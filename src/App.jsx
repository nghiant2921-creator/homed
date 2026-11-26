import doctorDashboard from '../vital-signs-system/doctor_dashboard.html?raw'
import './App.css'

function App() {
  return (
    <div className="doctor-app">
      <header className="doctor-app__toolbar">
        <p className="doctor-app__brand">
          Vital Signs <span aria-hidden="true">·</span> Giao diện bác sĩ
        </p>
        <a
          className="doctor-app__gemini-link"
          href="https://ai.google.dev/gemini-api/docs"
          target="_blank"
          rel="noopener noreferrer"
        >
          Tài liệu Gemini API <span aria-hidden="true">↗</span>
        </a>
      </header>
      <iframe
        className="doctor-app__dashboard"
        title="Giao diện theo dõi sinh hiệu cho bác sĩ"
        srcDoc={doctorDashboard}
      />
    </div>
  )
}

export default App
