#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <errno.h>
#include <signal.h>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <afunix.h>
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifndef ssize_t
typedef intptr_t ssize_t;
#endif

#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif

#define open _open
#define read _read
#define write _write
#define close _close
#define unlink _unlink
#define fsync(fd) (FlushFileBuffers((HANDLE)_get_osfhandle(fd)) ? 0 : -1)
#define chmod(path, mode) (_chmod((path), (mode)))

typedef SOCKET NativeSocket;
#define INVALID_NATIVE_SOCKET INVALID_SOCKET
#define native_socket_close closesocket
#define socket_errno WSAGetLastError()
#define socket_poll WSAPoll
#define NATIVE_POLLIN POLLRDNORM

static inline void sleep_poll_reset(void)
{
}

static bool win32_socket_ready(void)
{
    static bool initialized = false;
    if (initialized)
        return true;

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return false;
    initialized = true;
    return true;
}

#else
#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>

typedef int NativeSocket;
#define INVALID_NATIVE_SOCKET (-1)
#define native_socket_close close
#define socket_errno errno
#define socket_poll poll
#define NATIVE_POLLIN POLLIN
#endif

/* ============================================================================
 * 1. CẤU HÌNH KIẾN TRÚC MẠNG & BỘ NHỚ TĨNH (ZERO HEAP ALLOCATION)
 * ============================================================================ */
#define WINDOW_SIZE 10    // Cửa sổ trượt 10 giây (tần số 1 Hz)
#define IN_FEATURES 8     // 8 đặc trưng động học & sinh tồn
#define HIDDEN_NEURONS 16 // Số nơ-ron tầng ẩn
#define OUT_CLASSES 4     // 4 trạng thái lâm sàng
#define BIAS_COL 1

#define IN_DIM_BIAS (IN_FEATURES + BIAS_COL)        // 9
#define HIDDEN_DIM_BIAS (HIDDEN_NEURONS + BIAS_COL) // 17

#define MAX_PACKET_LEN 256
#define MAX_JSON_LEN 640
#define MAX_PATH_LEN 256
#define SENSOR_TIMEOUT_SEC 5      // Cảnh báo mất tín hiệu cảm biến sau 5 giây
#define ALERT_COOLDOWN_TICK 15    // Giãn cách tối thiểu 15 chu kỳ giữa 2 lần gọi VLM
#define PROFILE_SAVE_INTERVAL 300 // Tự động lưu profile mỗi 300 giây nghỉ ngơi (5 phút)

#define PROFILE_MAGIC 0x56544250U // "VTBP" (Vital Triage Baseline Profile)
#define PROFILE_VERSION 1U
#define PROFILE_FLAG_SEEDED (1U << 0)

static volatile sig_atomic_t g_running = 1;

typedef enum
{
    STATE_NORMAL = 0,
    STATE_NOCTURNAL_AGITATION = 1,
    STATE_POSTURAL_INSTABILITY = 2,
    STATE_CRITICAL_FALL_COLLAPSE = 3
} TriageClass;

static const char *CLASS_NAMES[OUT_CLASSES] = {
    "NORMAL_REST_OR_GAIT",
    "NOCTURNAL_HR_SPIKE_AGITATION",
    "POSTURAL_INSTABILITY_FATIGUE",
    "CRITICAL_FALL_OR_COLLAPSE"};

/* ============================================================================
 * 2. CẤU TRÚC HỒ SƠ SINH LÝ NỀN CÁ NHÂN HÓA (56 BYTES - CỐ ĐỊNH CĂN CHỈNH)
 * ============================================================================ */
typedef struct
{
    uint32_t magic;           // 0x56544250 ("VTBP")
    uint16_t version;         // Phiên bản cấu trúc (1)
    uint16_t flags;           // Cờ trạng thái (VD: PROFILE_FLAG_SEEDED)
    uint32_t anon_subject_id; // Mã định danh ẩn danh (không chứa PII/PHI)
    float hr_baseline;        // Nhịp tim nghỉ nền cá nhân hóa (BPM)
    float hr_dev_scale;       // Thang độ nhạy biến thiên nhịp tim (BPM)
    float spo2_baseline;      // Ngưỡng SpO2 nền (%)
    float rmssd_baseline;     // Biến thiên nhịp tim nền (ms)
    uint32_t reserved;        // Đệm căn chỉnh 8-byte cho các trường uint64_t
    uint64_t resting_samples; // Tổng số giây nghỉ ngơi đã tích lũy
    uint64_t updated_unix_ts; // Dấu thời gian cập nhật cuối (UTC epoch)
    uint32_t crc32;           // Mã kiểm tra toàn vẹn dữ liệu
    uint32_t padding_end;     // Đệm cuối cấu trúc (tổng 56 bytes)
} PatientBaselineProfile;

static uint32_t compute_crc32(const void *data, size_t len)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFFU;
    for (size_t i = 0; i < len; ++i)
    {
        crc ^= bytes[i];
        for (int b = 0; b < 8; ++b)
        {
            uint32_t mask = -(crc & 1U);
            crc = (crc >> 1) ^ (0xEDB88320U & mask);
        }
    }
    return ~crc;
}

static void profile_set_defaults(PatientBaselineProfile *prof, uint32_t anon_id)
{
    memset(prof, 0, sizeof(PatientBaselineProfile));
    prof->magic = PROFILE_MAGIC;
    prof->version = PROFILE_VERSION;
    prof->anon_subject_id = anon_id;
    prof->hr_baseline = 75.0f;
    prof->hr_dev_scale = 25.0f;
    prof->spo2_baseline = 97.0f;
    prof->rmssd_baseline = 30.0f;
    prof->resting_samples = 0;
    prof->updated_unix_ts = (uint64_t)time(NULL);
}

static bool profile_load(PatientBaselineProfile *prof, const char *path)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return false;

    PatientBaselineProfile tmp;
    ssize_t n = read(fd, &tmp, sizeof(PatientBaselineProfile));
    close(fd);

    if (n != (ssize_t)sizeof(PatientBaselineProfile))
        return false;
    if (tmp.magic != PROFILE_MAGIC || tmp.version != PROFILE_VERSION)
        return false;

    uint32_t expected_crc = compute_crc32(&tmp, offsetof(PatientBaselineProfile, crc32));
    if (tmp.crc32 != expected_crc)
    {
        fprintf(stderr, "[WARN] Tep profile %s bi sai lech CRC32, khoi tao lai gia tri mac dinh.\n", path);
        return false;
    }

    // Kiểm tra biên sinh lý hợp lệ trước khi áp dụng
    if (tmp.hr_baseline < 40.0f || tmp.hr_baseline > 130.0f ||
        tmp.spo2_baseline < 85.0f || tmp.spo2_baseline > 100.0f)
    {
        return false;
    }

    memcpy(prof, &tmp, sizeof(PatientBaselineProfile));
    return true;
}

/* Ghi nguyên tử qua tệp tạm + fsync + rename để chống hỏng file khi mất điện */
static bool profile_save_atomic(PatientBaselineProfile *prof, const char *path)
{
    char tmp_path[MAX_PATH_LEN];
    int n = snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path);
    if (n < 0 || (size_t)n >= sizeof(tmp_path))
        return false;

    prof->updated_unix_ts = (uint64_t)time(NULL);
    prof->crc32 = compute_crc32(prof, offsetof(PatientBaselineProfile, crc32));

    int fd = open(tmp_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0)
    {
        perror("[WARN] open(profile.tmp)");
        return false;
    }

    ssize_t written = write(fd, prof, sizeof(PatientBaselineProfile));
    if (written != (ssize_t)sizeof(PatientBaselineProfile))
    {
        close(fd);
        unlink(tmp_path);
        return false;
    }

    fsync(fd);
    close(fd);

#ifdef _WIN32
    if (!MoveFileExA(tmp_path, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        fprintf(stderr, "[WARN] MoveFileEx(profile) failed: %lu\n", GetLastError());
        unlink(tmp_path);
        return false;
    }
#else
    if (rename(tmp_path, path) < 0)
    {
        perror("[WARN] rename(profile)");
        unlink(tmp_path);
        return false;
    }
#endif
    return true;
}

/* ============================================================================
 * 3. THƯ VIỆN MA TRẬN TĨNH & TRỌNG SỐ MÔ HÌNH
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

static const float W0_DATA[IN_DIM_BIAS * HIDDEN_NEURONS] = {
    -0.4f, 0.8f, 0.2f, 0.9f, -0.3f, 0.1f, 0.5f, -0.2f, 0.7f, -0.5f, 0.3f, 0.6f, -0.1f, 0.4f, -0.6f, 0.2f,
    -0.5f, 1.1f, 0.1f, 0.8f, -0.2f, 0.3f, 0.9f, -0.4f, 0.6f, -0.3f, 0.2f, 0.7f, -0.2f, 0.5f, -0.4f, 0.3f,
    0.3f, -0.6f, 0.4f, -0.5f, 0.2f, -0.3f, -0.4f, 0.5f, -0.2f, 0.4f, -0.1f, -0.3f, 0.3f, -0.2f, 0.4f, -0.1f,
    -0.6f, 0.3f, 0.5f, 1.2f, -0.1f, 0.4f, 0.8f, -0.3f, 0.5f, -0.2f, 0.6f, 0.9f, -0.4f, 0.3f, -0.5f, 0.4f,
    -0.3f, 0.2f, 0.7f, 1.3f, -0.4f, 0.8f, 0.4f, -0.2f, 0.3f, -0.5f, 0.9f, 1.1f, -0.3f, 0.2f, -0.4f, 0.6f,
    0.5f, -0.1f, -0.8f, -1.4f, 0.3f, -0.7f, -0.3f, 0.4f, -0.2f, 0.6f, -0.8f, -1.2f, 0.2f, -0.3f, 0.5f, -0.5f,
    -0.4f, 0.9f, 0.6f, 1.1f, -0.2f, 0.5f, 0.7f, -0.3f, 0.8f, -0.4f, 0.4f, 0.6f, -0.1f, 0.5f, -0.3f, 0.4f,
    0.2f, -0.5f, 0.4f, 0.9f, 0.1f, 0.6f, -0.4f, 0.3f, -0.5f, 0.2f, 0.7f, 0.8f, 0.2f, -0.3f, 0.4f, 0.2f,
    0.2f, -0.4f, -0.2f, -0.8f, 0.1f, -0.3f, -0.3f, 0.2f, -0.2f, 0.1f, -0.4f, -0.6f, 0.1f, -0.2f, 0.2f, -0.2f};

static const float W1_DATA[HIDDEN_DIM_BIAS * OUT_CLASSES] = {
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
    0.5f, -0.2f, -0.2f, -0.5f};

/* ============================================================================
 * 4. BỘ LỌC KALMAN 1D THÍCH ỨNG CHUYỂN ĐỘNG
 * ============================================================================ */
typedef struct
{
    float x;
    float p;
    float q;
    float r_base;
    float motion_gain;
    float min_valid;
    float max_valid;
    bool initialized;
} Kalman1D;

static void kalman1d_init(Kalman1D *kf, float q, float r_base, float motion_gain,
                          float min_valid, float max_valid)
{
    kf->x = 0.0f;
    kf->p = 4.0f;
    kf->q = q;
    kf->r_base = r_base;
    kf->motion_gain = motion_gain;
    kf->min_valid = min_valid;
    kf->max_valid = max_valid;
    kf->initialized = false;
}

static float kalman1d_update(Kalman1D *kf, float z, float motion_vel)
{
    if (isnan(z) || z < kf->min_valid || z > kf->max_valid)
    {
        return kf->initialized ? kf->x : z;
    }

    if (!kf->initialized)
    {
        kf->x = z;
        kf->p = kf->r_base;
        kf->initialized = true;
        return kf->x;
    }

    float p_pred = kf->p + kf->q;
    float r_eff = kf->r_base * (1.0f + kf->motion_gain * motion_vel * motion_vel);

    float innovation = z - kf->x;
    float sigma = sqrtf(p_pred + r_eff);
    float norm_err = fabsf(innovation) / (sigma + 1e-6f);
    if (norm_err > 3.0f)
    {
        float penalty = norm_err / 3.0f;
        r_eff *= (penalty * penalty);
    }

    float k_gain = p_pred / (p_pred + r_eff);
    kf->x = kf->x + k_gain * innovation;
    kf->p = (1.0f - k_gain) * p_pred;

    return kf->x;
}

/* ============================================================================
 * 5. BỘ ĐỆM VÒNG & TRÍCH XUẤT ĐẶC TRƯNG THEO NGƯỠNG NỀN CÁ NHÂN HÓA
 * ============================================================================ */
typedef struct
{
    float hr_bpm;
    float spo2_pct;
    float torso_angle_deg;
    float hip_height_norm;
    float keypoint_vel;
} SensorFrame;

typedef struct
{
    SensorFrame buf[WINDOW_SIZE];
    int head;
    int count;
    float stillness_sec;
} VitalRingBuffer;

static void ring_buffer_init(VitalRingBuffer *rb)
{
    memset(rb, 0, sizeof(VitalRingBuffer));
}

static void ring_buffer_push(VitalRingBuffer *rb, SensorFrame frame)
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

/* Chuẩn hóa đặc trưng dựa trên nhịp tim nghỉ nền & SpO2 nền riêng của bệnh nhân */
static bool extract_features(const VitalRingBuffer *rb,
                             const PatientBaselineProfile *prof,
                             float out_features[IN_FEATURES],
                             float *out_rmssd)
{
    if (rb->count < WINDOW_SIZE)
        return false;

    int oldest_idx = rb->head;
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
    if (out_rmssd)
        *out_rmssd = rmssd;

    float hr_scale = fmaxf(prof->hr_dev_scale, 15.0f);

    // Áp dụng trực tiếp ngưỡng nền cá nhân hóa từ PatientBaselineProfile
    out_features[0] = (hr_mean - prof->hr_baseline) / hr_scale;
    out_features[1] = hr_delta / 25.0f;
    out_features[2] = (rmssd - prof->rmssd_baseline) / 25.0f;
    out_features[3] = fmaxf(0.0f, (prof->spo2_baseline - latest.spo2_pct) / 5.0f);
    out_features[4] = latest.torso_angle_deg / 90.0f;
    out_features[5] = latest.hip_height_norm;
    out_features[6] = max_vel / 2.0f;
    out_features[7] = fminf(rb->stillness_sec / 30.0f, 1.0f);

    return true;
}

/* Cập nhật chỉ số nghỉ nền bằng EMA chậm chỉ khi bệnh nhân hoàn toàn yên tĩnh */
static bool update_resting_baseline(PatientBaselineProfile *prof,
                                    const VitalRingBuffer *rb,
                                    SensorFrame raw_frame,
                                    SensorFrame filtered_frame,
                                    float current_rmssd,
                                    TriageClass cls)
{
    if (cls != STATE_NORMAL)
        return false;
    if (raw_frame.keypoint_vel >= 0.03f || rb->stillness_sec < 10.0f)
        return false;
    if (raw_frame.hr_bpm < 42.0f || raw_frame.hr_bpm > 115.0f)
        return false;
    if (fabsf(filtered_frame.hr_bpm - prof->hr_baseline) > 18.0f)
        return false;

    // Giai đoạn hiệu chỉnh ban đầu (5 phút nghỉ đầu tiên) học nhanh hơn một chút (alpha=0.01)
    // Sau đó chuyển sang theo dõi trôi sinh lý dài hạn cực chậm (alpha=0.002)
    float alpha_hr = (prof->resting_samples < 300) ? 0.01f : 0.002f;
    float alpha_slow = 0.001f;

    prof->hr_baseline = (1.0f - alpha_hr) * prof->hr_baseline + alpha_hr * filtered_frame.hr_bpm;
    prof->rmssd_baseline = (1.0f - alpha_slow) * prof->rmssd_baseline + alpha_slow * current_rmssd;

    if (filtered_frame.spo2_pct >= 88.0f && filtered_frame.spo2_pct <= 100.0f)
    {
        prof->spo2_baseline = (1.0f - alpha_slow) * prof->spo2_baseline + alpha_slow * filtered_frame.spo2_pct;
    }

    prof->resting_samples++;
    return true;
}

/* ============================================================================
 * 6. ENGINE SUY LUẬN TĨNH & ĐIỀU PHỐI IPC HAI CHIỀU
 * ============================================================================ */
typedef struct
{
    float a0_buf[IN_DIM_BIAS];
    float z0_buf[HIDDEN_NEURONS];
    float a1_buf[HIDDEN_DIM_BIAS];
    float z1_buf[OUT_CLASSES];
    float probs[OUT_CLASSES];
} InferenceContext;

static TriageClass run_edge_inference(InferenceContext *ctx, const float features[IN_FEATURES])
{
    memcpy(ctx->a0_buf, features, IN_FEATURES * sizeof(float));
    ctx->a0_buf[IN_FEATURES] = 1.0f;

    Mat A0 = {1, IN_DIM_BIAS, ctx->a0_buf};
    Mat W0 = {IN_DIM_BIAS, HIDDEN_NEURONS, (float *)W0_DATA};
    Mat Z0 = {1, HIDDEN_NEURONS, ctx->z0_buf};

    mat_mul(&Z0, &A0, &W0);
    for (int i = 0; i < HIDDEN_NEURONS; ++i)
    {
        ctx->a1_buf[i] = relu_f(ctx->z0_buf[i]);
    }
    ctx->a1_buf[HIDDEN_NEURONS] = 1.0f;

    Mat A1 = {1, HIDDEN_DIM_BIAS, ctx->a1_buf};
    Mat W1 = {HIDDEN_DIM_BIAS, OUT_CLASSES, (float *)W1_DATA};
    Mat Z1 = {1, OUT_CLASSES, ctx->z1_buf};

    mat_mul(&Z1, &A1, &W1);
    softmax(ctx->z1_buf, ctx->probs, OUT_CLASSES);

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

typedef enum
{
    IPC_MODE_UDS = 0,
    IPC_MODE_FIFO = 1
} IpcMode;

typedef struct
{
    NativeSocket out_fd;
#ifdef _WIN32
    struct sockaddr_in dest_addr;
#else
    struct sockaddr_un dest_addr;
#endif
    uint64_t last_alert_tick;
    TriageClass last_alert_cls;
    bool has_alerted;
} AlertDispatcher;

static void handle_sigint(int sig)
{
    (void)sig;
    g_running = 0;
}

static void setup_signals(void)
{
#ifdef _WIN32
    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);
#else
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigint;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
#endif
}

static NativeSocket open_uds_server(const char *sock_path)
{
#ifdef _WIN32
    if (!win32_socket_ready())
    {
        fprintf(stderr, "[ERR] WSAStartup failed\n");
        return INVALID_NATIVE_SOCKET;
    }
#endif

    NativeSocket fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (fd == INVALID_NATIVE_SOCKET)
    {
        perror("[ERR] socket(IN_UDS)");
        return INVALID_NATIVE_SOCKET;
    }
    unlink(sock_path);

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, sock_path, sizeof(addr.sun_path) - 1);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("[ERR] bind(IN_UDS)");
        native_socket_close(fd);
        return INVALID_NATIVE_SOCKET;
    }
#ifndef _WIN32
    chmod(sock_path, 0660);
#endif
    return fd;
}

#ifdef _WIN32
static NativeSocket open_udp_server(const char *bind_address)
{
    if (!win32_socket_ready())
        return INVALID_NATIVE_SOCKET;

    unsigned short port = 9001;
    const char *colon = strrchr(bind_address, ':');
    if (colon)
        port = (unsigned short)strtoul(colon + 1, NULL, 10);

    NativeSocket fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd == INVALID_NATIVE_SOCKET)
    {
        fprintf(stderr, "[ERR] socket(IN_UDP) failed: %d\n", WSAGetLastError());
        return INVALID_NATIVE_SOCKET;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        fprintf(stderr, "[ERR] bind(IN_UDP) failed: %d\n", WSAGetLastError());
        closesocket(fd);
        return INVALID_NATIVE_SOCKET;
    }
    return fd;
}
#endif

static NativeSocket open_fifo_server(const char *fifo_path)
{
#ifdef _WIN32
    (void)fifo_path;
    fprintf(stderr, "[ERR] --fifo khong duoc ho tro khi chay native Windows. Hay dung --uds.\n");
    return INVALID_NATIVE_SOCKET;
#else
    if (mkfifo(fifo_path, 0660) < 0 && errno != EEXIST)
    {
        perror("[ERR] mkfifo");
        return INVALID_NATIVE_SOCKET;
    }
    int fd = open(fifo_path, O_RDWR | O_NONBLOCK);
    if (fd < 0)
    {
        perror("[ERR] open(FIFO)");
        return INVALID_NATIVE_SOCKET;
    }
    return fd;
#endif
}

static bool init_alert_dispatcher(AlertDispatcher *disp, const char *alert_sock_path)
{
#ifdef _WIN32
    if (!win32_socket_ready())
        return false;
    memset(disp, 0, sizeof(AlertDispatcher));
    disp->out_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (disp->out_fd == INVALID_NATIVE_SOCKET)
    {
        fprintf(stderr, "[ERR] socket(OUT_ALERT_UDP) failed: %d\n", WSAGetLastError());
        return false;
    }
    u_long nonblocking = 1;
    ioctlsocket(disp->out_fd, FIONBIO, &nonblocking);

    unsigned short port = 9002;
    const char *colon = strrchr(alert_sock_path, ':');
    if (colon)
        port = (unsigned short)strtoul(colon + 1, NULL, 10);

    memset(&disp->dest_addr, 0, sizeof(disp->dest_addr));
    disp->dest_addr.sin_family = AF_INET;
    disp->dest_addr.sin_port = htons(port);
    disp->dest_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    return true;
#else
    memset(disp, 0, sizeof(AlertDispatcher));
    disp->out_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (disp->out_fd == INVALID_NATIVE_SOCKET)
    {
        perror("[ERR] socket(OUT_ALERT_UDS)");
        return false;
    }
    int flags = fcntl(disp->out_fd, F_GETFL, 0);
    if (flags >= 0)
        fcntl(disp->out_fd, F_SETFL, flags | O_NONBLOCK);
    disp->dest_addr.sun_family = AF_UNIX;
    strncpy(disp->dest_addr.sun_path, alert_sock_path, sizeof(disp->dest_addr.sun_path) - 1);
    return true;
#endif
}

static void send_uds_datagram(const AlertDispatcher *disp, const char *json_msg, size_t len)
{
    if (disp->out_fd == INVALID_NATIVE_SOCKET || len == 0)
        return;
#ifdef _WIN32
    ssize_t sent = sendto(disp->out_fd, json_msg, (int)len, 0,
                          (const struct sockaddr *)&disp->dest_addr,
                          sizeof(disp->dest_addr));
#else
    ssize_t sent = sendto(disp->out_fd, json_msg, (int)len, 0,
                          (const struct sockaddr *)&disp->dest_addr,
                          sizeof(disp->dest_addr));
#endif
#ifdef _WIN32
    int err = WSAGetLastError();
    if (sent == SOCKET_ERROR &&
        err != WSAEWOULDBLOCK && err != WSAECONNREFUSED)
    {
        fprintf(stderr, "[WARN] sendto(ALERT_UDS) failed: %d\n", err);
    }
#else
    if (sent < 0 && errno != ENOENT && errno != ECONNREFUSED && errno != EAGAIN)
    {
        perror("[WARN] sendto(ALERT_UDS)");
    }
#endif
}

static bool parse_telemetry_line(const char *line, SensorFrame *out_frame)
{
    int matched = sscanf(line, "%f %f %f %f %f",
                         &out_frame->hr_bpm,
                         &out_frame->spo2_pct,
                         &out_frame->torso_angle_deg,
                         &out_frame->hip_height_norm,
                         &out_frame->keypoint_vel);
    if (matched != 5)
    {
        matched = sscanf(line, "%f,%f,%f,%f,%f",
                         &out_frame->hr_bpm,
                         &out_frame->spo2_pct,
                         &out_frame->torso_angle_deg,
                         &out_frame->hip_height_norm,
                         &out_frame->keypoint_vel);
    }
    return (matched == 5);
}

static void process_frame(uint64_t tick, Kalman1D *kf_hr, Kalman1D *kf_spo2,
                          VitalRingBuffer *rb, InferenceContext *ctx,
                          PatientBaselineProfile *prof, const char *profile_path,
                          uint32_t *unsaved_resting_ticks,
                          AlertDispatcher *disp, SensorFrame raw_frame)
{
    float features[IN_FEATURES];
    char json_buf[MAX_JSON_LEN];

    SensorFrame filtered_frame = raw_frame;
    filtered_frame.hr_bpm = kalman1d_update(kf_hr, raw_frame.hr_bpm, raw_frame.keypoint_vel);
    filtered_frame.spo2_pct = kalman1d_update(kf_spo2, raw_frame.spo2_pct, raw_frame.keypoint_vel);

    ring_buffer_push(rb, filtered_frame);

    float current_rmssd = prof->rmssd_baseline;
    if (!extract_features(rb, prof, features, &current_rmssd))
    {
        printf("{\"status\":\"WARMING_UP\",\"buffered_frames\":%d,\"required\":%d,"
               "\"hr_raw\":%.0f,\"hr\":%.1f,\"hr_base\":%.1f}\n",
               rb->count, WINDOW_SIZE, raw_frame.hr_bpm, filtered_frame.hr_bpm, prof->hr_baseline);
        fflush(stdout);
        return;
    }

    TriageClass cls = run_edge_inference(ctx, features);

    // Tự động hiệu chỉnh nhịp tim nền khi bệnh nhân nghỉ ngơi ổn định
    if (update_resting_baseline(prof, rb, raw_frame, filtered_frame, current_rmssd, cls))
    {
        (*unsaved_resting_ticks)++;
        if (*unsaved_resting_ticks >= PROFILE_SAVE_INTERVAL)
        {
            if (profile_save_atomic(prof, profile_path))
            {
                *unsaved_resting_ticks = 0;
            }
        }
    }

    bool is_critical = (cls == STATE_CRITICAL_FALL_COLLAPSE ||
                        (cls == STATE_NOCTURNAL_AGITATION && ctx->probs[cls] > 0.70f));

    bool should_dispatch_uds = false;
    if (is_critical)
    {
        bool escalated = (!disp->has_alerted || cls > disp->last_alert_cls);
        bool cooldown_passed = ((tick - disp->last_alert_tick) >= ALERT_COOLDOWN_TICK);
        if (escalated || cooldown_passed)
        {
            should_dispatch_uds = true;
            disp->last_alert_tick = tick;
            disp->last_alert_cls = cls;
            disp->has_alerted = true;
        }
    }
    else if (cls == STATE_NORMAL)
    {
        disp->last_alert_cls = STATE_NORMAL;
    }

    int len = snprintf(json_buf, sizeof(json_buf),
                       "{\"status\":\"OK\",\"tick\":%llu,\"hr_raw\":%.0f,\"hr\":%.1f,"
                       "\"hr_base\":%.1f,\"spo2\":%.1f,\"spo2_base\":%.1f,"
                       "\"torso_deg\":%.1f,\"hip_norm\":%.2f,\"stillness_sec\":%.0f,"
                       "\"prediction\":\"%s\",\"confidence\":%.3f,"
                       "\"trigger_vlm_hitl\":%s,\"uds_dispatched\":%s}\n",
                       (unsigned long long)tick,
                       raw_frame.hr_bpm, filtered_frame.hr_bpm, prof->hr_baseline,
                       filtered_frame.spo2_pct, prof->spo2_baseline,
                       filtered_frame.torso_angle_deg, filtered_frame.hip_height_norm,
                       rb->stillness_sec, CLASS_NAMES[cls], ctx->probs[cls],
                       is_critical ? "true" : "false",
                       should_dispatch_uds ? "true" : "false");

    if (len > 0)
    {
        size_t safe_len = (size_t)len < sizeof(json_buf) ? (size_t)len : sizeof(json_buf) - 1;
        fwrite(json_buf, 1, safe_len, stdout);
        fflush(stdout);

        if (should_dispatch_uds)
        {
            send_uds_datagram(disp, json_buf, safe_len);
        }
    }
}

int main(int argc, char **argv)
{
    IpcMode mode = IPC_MODE_UDS;
#ifdef _WIN32
    const char *in_path = "127.0.0.1:9001";
    const char *alert_path = "127.0.0.1:9002";
    const char *profile_path = "C:\\Temp\\patient_baseline.bin";
#else
    const char *in_path = "/tmp/vital_triage.sock";
    const char *alert_path = "/tmp/vital_alerts.sock";
    const char *profile_path = "/tmp/patient_baseline.bin";
#endif

    bool has_init_hr = false, has_init_spo2 = false;
    float init_hr = 75.0f, init_spo2 = 97.0f;
    uint32_t anon_id = 1001;

    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "--fifo") == 0 && i + 1 < argc)
        {
            mode = IPC_MODE_FIFO;
            in_path = argv[++i];
        }
        else if (strcmp(argv[i], "--uds") == 0 && i + 1 < argc)
        {
            mode = IPC_MODE_UDS;
            in_path = argv[++i];
        }
        else if (strcmp(argv[i], "--alert-uds") == 0 && i + 1 < argc)
        {
            alert_path = argv[++i];
        }
        else if (strcmp(argv[i], "--profile") == 0 && i + 1 < argc)
        {
            profile_path = argv[++i];
        }
        else if (strcmp(argv[i], "--subject-id") == 0 && i + 1 < argc)
        {
            anon_id = (uint32_t)strtoul(argv[++i], NULL, 10);
        }
        else if (strcmp(argv[i], "--init-hr") == 0 && i + 1 < argc)
        {
            init_hr = strtof(argv[++i], NULL);
            has_init_hr = true;
        }
        else if (strcmp(argv[i], "--init-spo2") == 0 && i + 1 < argc)
        {
            init_spo2 = strtof(argv[++i], NULL);
            has_init_spo2 = true;
        }
        else
        {
            fprintf(stderr,
#ifdef _WIN32
                    "Su dung: %s [--uds 127.0.0.1:9001 | --fifo khong-ho-tro] [--alert-uds 127.0.0.1:9002]\n"
                    "            [--profile C:\\Temp\\patient_baseline.bin] [--subject-id 1001] [--init-hr 60] [--init-spo2 95]\n",
#else
                    "Su dung: %s [--uds /in.sock | --fifo /in.pipe] [--alert-uds /alert.sock]\n"
                    "            [--profile /baseline.bin] [--subject-id 1001] [--init-hr 60] [--init-spo2 95]\n",
#endif
                    argv[0]);
            return EXIT_FAILURE;
        }
    }

    setup_signals();

    PatientBaselineProfile prof;
    if (profile_load(&prof, profile_path))
    {
        fprintf(stderr, "[INFO] Da nap ho so nen ca nhan hoa tu %s (ID=%u, HR_base=%.1f BPM, SpO2_base=%.1f%%, mau=%llu)\n",
                profile_path, prof.anon_subject_id, prof.hr_baseline, prof.spo2_baseline,
                (unsigned long long)prof.resting_samples);
    }
    else
    {
        profile_set_defaults(&prof, anon_id);
        fprintf(stderr, "[INFO] Khoi tao ho so nen mac dinh moi tai %s (ID=%u)\n",
                profile_path, prof.anon_subject_id);
    }

    // Cho phép bác sĩ áp đặt ngưỡng khởi điểm lâm sàng nếu truyền qua CLI
    if (has_init_hr && init_hr >= 40.0f && init_hr <= 130.0f)
    {
        prof.hr_baseline = init_hr;
        prof.flags |= PROFILE_FLAG_SEEDED;
    }
    if (has_init_spo2 && init_spo2 >= 85.0f && init_spo2 <= 100.0f)
    {
        prof.spo2_baseline = init_spo2;
        prof.flags |= PROFILE_FLAG_SEEDED;
    }
    profile_save_atomic(&prof, profile_path);

    NativeSocket in_fd =
#ifdef _WIN32
        (mode == IPC_MODE_FIFO) ? open_fifo_server(in_path) : open_udp_server(in_path);
#else
        (mode == IPC_MODE_UDS) ? open_uds_server(in_path) : open_fifo_server(in_path);
#endif
    if (in_fd == INVALID_NATIVE_SOCKET)
        return EXIT_FAILURE;

    AlertDispatcher disp;
    if (!init_alert_dispatcher(&disp, alert_path))
    {
        native_socket_close(in_fd);
        unlink(in_path);
        return EXIT_FAILURE;
    }

    fprintf(stderr, "[INFO] Edge Triage Daemon san sang:\n");
    fprintf(stderr, "       - Kenh vao (%s): %s\n", (mode == IPC_MODE_UDS) ? "UDS_DGRAM" : "FIFO", in_path);
    fprintf(stderr, "       - Kenh canh bao (UDS_DGRAM): %s\n", alert_path);
    fprintf(stderr, "       - Nguong nen hien tai: HR=%.1f BPM, SpO2=%.1f%%\n",
            prof.hr_baseline, prof.spo2_baseline);

    Kalman1D kf_hr, kf_spo2;
    kalman1d_init(&kf_hr, 1.5f, 6.0f, 15.0f, 25.0f, 240.0f);
    kalman1d_init(&kf_spo2, 0.15f, 1.5f, 10.0f, 50.0f, 100.0f);

    VitalRingBuffer rb;
    InferenceContext ctx;
    ring_buffer_init(&rb);

    #ifdef _WIN32
    WSAPOLLFD pfd = {.fd = in_fd, .events = POLLRDNORM, .revents = 0};
    #else
    struct pollfd pfd = {.fd = in_fd, .events = POLLIN, .revents = 0};
    #endif
    char rx_buf[MAX_PACKET_LEN];
    char fifo_accum[MAX_PACKET_LEN];
    char timeout_json[MAX_JSON_LEN];
    size_t fifo_pos = 0;

    uint64_t tick = 0;
    uint32_t unsaved_resting_ticks = 0;
    int idle_sec = 0;

    while (g_running)
    {
        int ret = socket_poll(&pfd, 1, 1000);
        if (ret < 0)
        {
#ifdef _WIN32
            fprintf(stderr, "[ERR] poll failed: %d\n", WSAGetLastError());
#else
            if (errno == EINTR)
                continue;
            perror("[ERR] poll");
#endif
            break;
        }

        if (ret == 0)
        {
            idle_sec++;
            if (idle_sec == SENSOR_TIMEOUT_SEC)
            {
                int tlen = snprintf(timeout_json, sizeof(timeout_json),
                                    "{\"status\":\"SENSOR_STREAM_TIMEOUT\",\"idle_sec\":%d,"
                                    "\"trigger_vlm_hitl\":true,\"uds_dispatched\":true}\n",
                                    idle_sec);
                if (tlen > 0)
                {
                    size_t safe_tlen = (size_t)tlen < sizeof(timeout_json) ? (size_t)tlen : sizeof(timeout_json) - 1;
                    fwrite(timeout_json, 1, safe_tlen, stdout);
                    fflush(stdout);
                    send_uds_datagram(&disp, timeout_json, safe_tlen);
                }
            }
            continue;
        }

        if (pfd.revents & NATIVE_POLLIN)
        {
            idle_sec = 0;

            if (mode == IPC_MODE_UDS)
            {
                ssize_t n = recvfrom(in_fd, rx_buf, sizeof(rx_buf) - 1, 0, NULL, NULL);
                if (n > 0)
                {
                    rx_buf[n] = '\0';
                    SensorFrame frame;
                    if (parse_telemetry_line(rx_buf, &frame))
                    {
                        process_frame(++tick, &kf_hr, &kf_spo2, &rb, &ctx,
                                      &prof, profile_path, &unsaved_resting_ticks, &disp, frame);
                    }
                }
            }
            else
            {
                ssize_t n = read(in_fd, rx_buf, sizeof(rx_buf));
                for (ssize_t i = 0; i < n; ++i)
                {
                    char c = rx_buf[i];
                    if (c == '\n' || fifo_pos >= sizeof(fifo_accum) - 1)
                    {
                        fifo_accum[fifo_pos] = '\0';
                        if (fifo_pos > 0)
                        {
                            SensorFrame frame;
                            if (parse_telemetry_line(fifo_accum, &frame))
                            {
                                process_frame(++tick, &kf_hr, &kf_spo2, &rb, &ctx,
                                              &prof, profile_path, &unsaved_resting_ticks, &disp, frame);
                            }
                        }
                        fifo_pos = 0;
                    }
                    else if (c != '\r')
                    {
                        fifo_accum[fifo_pos++] = c;
                    }
                }
            }
        }
    }

    fprintf(stderr, "\n[INFO] Dang luu ho so nen ca nhan hoa (HR_base=%.1f BPM) va don dep socket...\n",
            prof.hr_baseline);
    profile_save_atomic(&prof, profile_path);
    native_socket_close(disp.out_fd);
    native_socket_close(in_fd);
#ifdef _WIN32
    WSACleanup();
#endif
    unlink(in_path);
    return EXIT_SUCCESS;
}
