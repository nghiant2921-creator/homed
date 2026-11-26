#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

/* ============================================================================
 * 1. CẤU HÌNH KIẾN TRÚC MẠNG & BỘ NHỚ TĨNH (ZERO HEAP ALLOCATION)
 * ============================================================================ */
#define WINDOW_SIZE 10    // Cửa sổ trượt 10 giây (tần số lấy mẫu 1 Hz)
#define IN_FEATURES 8     // 8 đặc trưng tổng hợp từ nhịp tim & tư thế
#define HIDDEN_NEURONS 16 // Số nơ-ron tầng ẩn
#define OUT_CLASSES 4     // 4 phân lớp trạng thái lâm sàng
#define BIAS_COL 1        // Cột nơ-ron giả định luôn bằng 1.0f

#define IN_DIM_BIAS (IN_FEATURES + BIAS_COL)        // 9
#define HIDDEN_DIM_BIAS (HIDDEN_NEURONS + BIAS_COL) // 17

typedef enum
{
    STATE_NORMAL = 0,                // 0: Nghỉ ngơi / Vận động ổn định
    STATE_NOCTURNAL_AGITATION = 1,   // 1: Bồn chồn / Nhịp tim vọt cao ban đêm (đau/lo âu)
    STATE_POSTURAL_INSTABILITY = 2,  // 2: Suy kiệt / Rối loạn huyết động khi đổi tư thế
    STATE_CRITICAL_FALL_COLLAPSE = 3 // 3: Té ngã hoặc trụy mạch cấp tính (Cảnh báo khẩn)
} TriageClass;

static const char *CLASS_NAMES[OUT_CLASSES] = {
    "NORMAL_REST_OR_GAIT",
    "NOCTURNAL_HR_SPIKE_AGITATION",
    "POSTURAL_INSTABILITY_FATIGUE",
    "CRITICAL_FALL_OR_COLLAPSE"};

/* ============================================================================
 * 2. THƯ VIỆN MA TRẬN GỌN NHẸ (MATT-STYLE STATIC MATRIX)
 * ============================================================================ */
typedef struct
{
    int rows;
    int cols;
    float *data;
} Mat;

static inline void mat_mul(Mat *dst, const Mat *a, const Mat *b)
{
    for (int r = 0; r < a->rows; ++r)
    {
        for (int c = 0; c < b->cols; ++c)
        {
            float sum = 0.0f;
            for (int k = 0; k < a->cols; ++k)
            {
                sum += a->data[r * a->cols + k] * b->data[k * b->cols + c];
            }
            dst->data[r * dst->cols + c] = sum;
        }
    }
}

static inline float relu_f(float x)
{
    return x > 0.0f ? x : 0.0f;
}

static void softmax(float *logits, float *probs, int len)
{
    float max_val = logits[0];
    for (int i = 1; i < len; ++i)
    {
        if (logits[i] > max_val)
            max_val = logits[i];
    }
    float sum_exp = 0.0f;
    for (int i = 0; i < len; ++i)
    {
        probs[i] = expf(logits[i] - max_val);
        sum_exp += probs[i];
    }
    for (int i = 0; i < len; ++i)
    {
        probs[i] /= (sum_exp + 1e-9f);
    }
}

/* ============================================================================
 * 3. TRỌNG SỐ MÔ HÌNH ĐÃ HUẤN LUYỆN (NẠP TĨNH VÀO FLASH/RODATA)
 * Hàng cuối cùng của mỗi ma trận chính là vector Bias của tầng đó.
 * (Trong thực tế, tệp header này được sinh tự động từ Python/PyTorch)
 * ============================================================================ */
static const float W0_DATA[IN_DIM_BIAS * HIDDEN_NEURONS] = {
    // 8 hàng đầu: trọng số kết nối 8 đặc trưng -> 16 nơ-ron ẩn
    -0.4f, 0.8f, 0.2f, 0.9f, -0.3f, 0.1f, 0.5f, -0.2f, 0.7f, -0.5f, 0.3f, 0.6f, -0.1f, 0.4f, -0.6f, 0.2f,     // hr_norm
    -0.5f, 1.1f, 0.1f, 0.8f, -0.2f, 0.3f, 0.9f, -0.4f, 0.6f, -0.3f, 0.2f, 0.7f, -0.2f, 0.5f, -0.4f, 0.3f,     // hr_delta
    0.3f, -0.6f, 0.4f, -0.5f, 0.2f, -0.3f, -0.4f, 0.5f, -0.2f, 0.4f, -0.1f, -0.3f, 0.3f, -0.2f, 0.4f, -0.1f,  // hrv_rmssd
    -0.6f, 0.3f, 0.5f, 1.2f, -0.1f, 0.4f, 0.8f, -0.3f, 0.5f, -0.2f, 0.6f, 0.9f, -0.4f, 0.3f, -0.5f, 0.4f,     // spo2_drop
    -0.3f, 0.2f, 0.7f, 1.3f, -0.4f, 0.8f, 0.4f, -0.2f, 0.3f, -0.5f, 0.9f, 1.1f, -0.3f, 0.2f, -0.4f, 0.6f,     // torso_pitch
    0.5f, -0.1f, -0.8f, -1.4f, 0.3f, -0.7f, -0.3f, 0.4f, -0.2f, 0.6f, -0.8f, -1.2f, 0.2f, -0.3f, 0.5f, -0.5f, // hip_height
    -0.4f, 0.9f, 0.6f, 1.1f, -0.2f, 0.5f, 0.7f, -0.3f, 0.8f, -0.4f, 0.4f, 0.6f, -0.1f, 0.5f, -0.3f, 0.4f,     // pose_vel
    0.2f, -0.5f, 0.4f, 0.9f, 0.1f, 0.6f, -0.4f, 0.3f, -0.5f, 0.2f, 0.7f, 0.8f, 0.2f, -0.3f, 0.4f, 0.2f,       // stillness
                                                                                                              // Hàng thứ 9 (index 8): Bias của tầng ẩn
    0.2f, -0.4f, -0.2f, -0.8f, 0.1f, -0.3f, -0.3f, 0.2f, -0.2f, 0.1f, -0.4f, -0.6f, 0.1f, -0.2f, 0.2f, -0.2f};

static const float W1_DATA[HIDDEN_DIM_BIAS * OUT_CLASSES] = {
    // 16 hàng đầu: trọng số tầng ẩn -> 4 lớp đầu ra
    0.9f, -0.4f, -0.3f, -0.6f,
    -0.5f, 1.2f, 0.2f, 0.3f,
    -0.3f, 0.1f, 1.1f, 0.4f,
    -0.8f, 0.3f, 0.5f, 1.6f,
    0.6f, -0.2f, -0.2f, -0.4f,
    -0.4f, 0.2f, 0.9f, 0.7f,
    -0.5f, 1.0f, 0.3f, 0.5f,
    0.7f, -0.3f, -0.2f, -0.5f,
    -0.4f, 0.9f, 0.2f, 0.4f,
    0.6f, -0.3f, -0.4f, -0.5f,
    -0.3f, 0.2f, 0.8f, 0.9f,
    -0.7f, 0.4f, 0.6f, 1.4f,
    0.5f, -0.2f, -0.1f, -0.3f,
    -0.3f, 0.6f, 0.2f, 0.3f,
    0.6f, -0.4f, -0.3f, -0.5f,
    -0.2f, 0.3f, 0.5f, 0.6f,
    // Hàng thứ 17 (index 16): Bias của tầng đầu ra
    0.5f, -0.2f, -0.2f, -0.5f};

/* ============================================================================
 * 4. BỘ ĐỆM VÒNG (RING BUFFER) & TRÍCH XUẤT ĐẶC TRƯNG CHUỖI THỜI GIAN
 * ============================================================================ */
typedef struct
{
    float hr_bpm;          // Nhịp tim từ đồng hồ (BPM)
    float spo2_pct;        // SpO2 (%)
    float torso_angle_deg; // Góc nghiêng thân trên (0° = đứng thẳng, 90° = nằm ngang)
    float hip_height_norm; // Chiều cao hông chuẩn hóa (1.0 = đứng, 0.2 = nằm sàn)
    float keypoint_vel;    // Vận tốc dịch chuyển khung xương (m/s)
} SensorFrame;

typedef struct
{
    SensorFrame buf[WINDOW_SIZE];
    int head;
    int count;
    float stillness_sec;
} VitalRingBuffer;

void ring_buffer_init(VitalRingBuffer *rb)
{
    memset(rb, 0, sizeof(VitalRingBuffer));
}

void ring_buffer_push(VitalRingBuffer *rb, SensorFrame frame)
{
    rb->buf[rb->head] = frame;
    rb->head = (rb->head + 1) % WINDOW_SIZE;
    if (rb->count < WINDOW_SIZE)
        rb->count++;

    if (frame.keypoint_vel < 0.05f)
    {
        rb->stillness_sec += 1.0f;
    }
    else
    {
        rb->stillness_sec = 0.0f;
    }
}

/* Chuyển đổi chuỗi 10 giây thô thành 8 đặc trưng chuẩn hóa */
bool extract_features(const VitalRingBuffer *rb, float out_features[IN_FEATURES])
{
    if (rb->count < WINDOW_SIZE)
        return false; // Chưa đủ dữ liệu cửa sổ

    int oldest_idx = rb->head; // Phần tử cũ nhất trong vòng đệm đầy
    int newest_idx = (rb->head + WINDOW_SIZE - 1) % WINDOW_SIZE;

    float hr_sum = 0.0f, rr_diff_sq_sum = 0.0f, max_vel = 0.0f;
    float prev_rr = 60000.0f / fmaxf(rb->buf[oldest_idx].hr_bpm, 30.0f);

    for (int i = 0; i < WINDOW_SIZE; ++i)
    {
        int idx = (oldest_idx + i) % WINDOW_SIZE;
        SensorFrame f = rb->buf[idx];
        hr_sum += f.hr_bpm;

        if (f.keypoint_vel > max_vel)
            max_vel = f.keypoint_vel;

        if (i > 0)
        {
            float curr_rr = 60000.0f / fmaxf(f.hr_bpm, 30.0f);
            float diff = curr_rr - prev_rr;
            rr_diff_sq_sum += diff * diff;
            prev_rr = curr_rr;
        }
    }

    SensorFrame latest = rb->buf[newest_idx];
    SensorFrame oldest = rb->buf[oldest_idx];

    float hr_mean = hr_sum / WINDOW_SIZE;
    float hr_delta = latest.hr_bpm - oldest.hr_bpm;
    float rmssd = sqrtf(rr_diff_sq_sum / (WINDOW_SIZE - 1));

    // Chuẩn hóa về dải giá trị ổn định cho mạng nơ-ron
    out_features[0] = (hr_mean - 75.0f) / 30.0f;                     // 1. Nhịp tim trung bình
    out_features[1] = hr_delta / 25.0f;                              // 2. Độ dốc tăng nhịp tim (Spike)
    out_features[2] = (rmssd - 30.0f) / 25.0f;                       // 3. Biến thiên nhịp tim (RMSSD)
    out_features[3] = fmaxf(0.0f, (97.0f - latest.spo2_pct) / 5.0f); // 4. Mức sụt giảm SpO2
    out_features[4] = latest.torso_angle_deg / 90.0f;                // 5. Góc nghiêng thân
    out_features[5] = latest.hip_height_norm;                        // 6. Tỷ lệ độ cao hông
    out_features[6] = max_vel / 2.0f;                                // 7. Vận tốc đột ngột cực đại
    out_features[7] = fminf(rb->stillness_sec / 30.0f, 1.0f);        // 8. Thời gian bất động sau biến cố

    return true;
}

/* ============================================================================
 * 5. ENGINE SUY LUẬN TĨNH (FORWARD PASS)
 * ============================================================================ */
typedef struct
{
    float a0_buf[IN_DIM_BIAS];
    float z0_buf[HIDDEN_NEURONS];
    float a1_buf[HIDDEN_DIM_BIAS];
    float z1_buf[OUT_CLASSES];
    float probs[OUT_CLASSES];
} InferenceContext;

TriageClass run_edge_inference(InferenceContext *ctx, const float features[IN_FEATURES])
{
    // 1. Nạp đầu vào và gắn cố định nơ-ron Bias = 1.0f ở cuối
    memcpy(ctx->a0_buf, features, IN_FEATURES * sizeof(float));
    ctx->a0_buf[IN_FEATURES] = 1.0f;

    Mat A0 = {1, IN_DIM_BIAS, ctx->a0_buf};
    Mat W0 = {IN_DIM_BIAS, HIDDEN_NEURONS, (float *)W0_DATA};
    Mat Z0 = {1, HIDDEN_NEURONS, ctx->z0_buf};

    // 2. Tầng ẩn: Z0 = A0 * W0 -> A1 = ReLU(Z0) + [1.0f]
    mat_mul(&Z0, &A0, &W0);
    for (int i = 0; i < HIDDEN_NEURONS; ++i)
    {
        ctx->a1_buf[i] = relu_f(ctx->z0_buf[i]);
    }
    ctx->a1_buf[HIDDEN_NEURONS] = 1.0f;

    Mat A1 = {1, HIDDEN_DIM_BIAS, ctx->a1_buf};
    Mat W1 = {HIDDEN_DIM_BIAS, OUT_CLASSES, (float *)W1_DATA};
    Mat Z1 = {1, OUT_CLASSES, ctx->z1_buf};

    // 3. Tầng đầu ra: Z1 = A1 * W1 -> Softmax
    mat_mul(&Z1, &A1, &W1);
    softmax(ctx->z1_buf, ctx->probs, OUT_CLASSES);

    // 4. Chọn lớp có xác suất cao nhất
    int best_cls = 0;
    for (int i = 1; i < OUT_CLASSES; ++i)
    {
        if (ctx->probs[i] > ctx->probs[best_cls])
        {
            best_cls = i;
        }
    }
    return (TriageClass)best_cls;
}

/* ============================================================================
 * 6. CHƯƠNG TRÌNH CHÍNH & MÔ PHỎNG KỊCH BẢN LÂM SÀNG
 * ============================================================================ */
void emit_json_event(int tick, TriageClass cls, const InferenceContext *ctx, SensorFrame f)
{
    bool trigger_hitl = (cls == STATE_CRITICAL_FALL_COLLAPSE ||
                         (cls == STATE_NOCTURNAL_AGITATION && ctx->probs[cls] > 0.70f));

    printf("{\"tick\":%d,\"hr\":%.0f,\"spo2\":%.1f,\"torso_deg\":%.1f,"
           "\"prediction\":\"%s\",\"confidence\":%.3f,\"trigger_vlm_hitl\":%s}\n",
           tick, f.hr_bpm, f.spo2_pct, f.torso_angle_deg,
           CLASS_NAMES[cls], ctx->probs[cls],
           trigger_hitl ? "true" : "false");
    fflush(stdout);
}

int main(void)
{
    VitalRingBuffer rb;
    InferenceContext ctx;
    float features[IN_FEATURES];

    ring_buffer_init(&rb);

    // Giả lập 20 giây dữ liệu bệnh nhân:
    // Giây 0..9: Ngủ yên ổn định (HR ~68, SpO2 97%, nằm nghiêng 85°)
    // Giây 10..14: Cơn đau/kích động ban đêm (HR vọt lên 135 BPM, xoay trở mạnh)
    // Giây 15..19: Bệnh nhân cố bước xuống giường và té ngã (Hip drop 0.15, SpO2 tụt 91%)
    for (int t = 0; t < 20; ++t)
    {
        SensorFrame frame;
        if (t < 10)
        {
            frame = (SensorFrame){68.0f + (t % 2), 97.5f, 85.0f, 0.25f, 0.01f};
        }
        else if (t < 15)
        {
            frame = (SensorFrame){115.0f + (t - 10) * 6.0f, 95.0f, 45.0f, 0.45f, 0.85f};
        }
        else
        {
            frame = (SensorFrame){138.0f, 91.0f, 88.0f, 0.12f, (t == 15) ? 1.9f : 0.0f};
        }

        ring_buffer_push(&rb, frame);

        if (extract_features(&rb, features))
        {
            TriageClass pred = run_edge_inference(&ctx, features);
            emit_json_event(t, pred, &ctx, frame);
        }
    }

    return 0;
}
