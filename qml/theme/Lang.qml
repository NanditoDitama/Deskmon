pragma Singleton
import QtQuick

QtObject {
    id: i18n

    // Bahasa aktif saat ini. Default: "en" (English)
    property string currentLanguage: "en"

    readonly property bool isEnglish: currentLanguage === "en"
    readonly property bool isIndonesian: currentLanguage === "id"

    // Daftar bahasa yang didukung
    readonly property var availableLanguages: [
        { code: "en", name: "English", flag: "🇺🇸" },
        { code: "id", name: "Bahasa Indonesia", flag: "🇮🇩" }
    ]

    // Kamus terpusat teks multi-bahasa (Otomatis disinkronkan oleh scripts/sync_i18n.py)
    readonly property var strings: ({
        "en": {
            "activityMonitor": "Activity Monitor",
            "application": "Application",
            "authError": "Authentication Session Expired",
            "authErrorDesc": "Your session has expired. Please sign in again.",
            "cancel": "Cancel",
            "Check Update": "Check Update",
            "Check Update now": "Check Update now",
            "checkUpdate": "Check Update",
            "clockIn": "Clock In",
            "clockOut": "Clock Out",
            "confirm": "Confirm",
            "confirmSwitch": "Switch to Project?",
            "confirmSwitchDesc": "You have an active task. Do you want to switch to this task?",
            "connectionError": "Connection Error",
            "currentTask": "Current Task",
            "currentWindow": "Current Window",
            "Dark Mode": "Dark Mode",
            "darkMode": "Dark Mode",
            "delete": "Delete",
            "deskmon": "Deskmon",
            "dismiss": "Dismiss",
            "emptyCredentials": "Username and password cannot be empty",
            "english": "English",
            "filterDate": "Filter Date",
            "finishTask": "Finish Task",
            "idleDetected": "Idle Detected",
            "idleMessage": "No mouse or keyboard activity detected for a while.",
            "indonesian": "Bahasa Indonesia",
            "language": "Language",
            "Language": "Language",
            "Light Mode": "Light Mode",
            "lightMode": "Light Mode",
            "loginAgain": "Sign In Again",
            "loginFailed": "Login failed",
            "logout": "Logout",
            "Logout": "Logout",
            "mode": "Appearance Mode",
            "monitoredApps": "Monitored Apps",
            "My Profile": "My Profile",
            "needReview": "Mark as Need Review",
            "neutral": "Neutral",
            "noActiveTask": "No Active Task",
            "nonProductive": "Non-Productive",
            "password": "Password",
            "passwordPlaceholder": "Enter your password",
            "pauseTask": "Pause Task",
            "productive": "Productive",
            "productivityStatistics": "Productivity Statistics",
            "profile": "My Profile",
            "Profile": "Profile",
            "recordedClockIn": "Recorded entry time: ",
            "refresh": "Refresh",
            "Refresh": "Refresh",
            "resume": "Resume",
            "resumeTask": "Resume Task",
            "retry": "Retry",
            "save": "Save",
            "seeAll": "See All",
            "seeLess": "See Less",
            "selectDateRange": "Select Date Range",
            "settings": "Settings",
            "Settings": "Settings",
            "signIn": "Sign In",
            "signingIn": "Signing in...",
            "signInSubtitle": "Sign in to continue monitoring your tasks",
            "Switch to Dark Mode": "Switch to Dark Mode",
            "Switch to Light Mode": "Switch to Light Mode",
            "switchProject": "Switch Project",
            "switchToDarkMode": "Switch to Dark Mode",
            "switchToLightMode": "Switch to Light Mode",
            "target": "Target",
            "taskDetails": "Task Details",
            "timeAtWork": "Time At Work",
            "timeUp": "Time is Up",
            "timeUpFinished": "Your task time has expired.",
            "timeUpWarning": "Your task time has less than 10 minutes remaining!",
            "today": "Today",
            "topApps": "Top Applications",
            "topDomains": "Top Websites",
            "username": "Username",
            "usernamePlaceholder": "Enter your username or email",
            "waitingClockIn": "Waiting for clock-in data...",
            "welcomeBack": "Welcome Back",
            "windowTitle": "Title"
        },
        "id": {
            "activityMonitor": "Pemantau Aktivitas",
            "application": "Aplikasi",
            "authError": "Sesi Autentikasi Berakhir",
            "authErrorDesc": "Sesi Anda telah kedaluwarsa. Silakan masuk kembali.",
            "cancel": "Batal",
            "Check Update": "Periksa Pembaruan",
            "Check Update now": "Periksa Perbarui sekarang",
            "checkUpdate": "Cek Pembaruan",
            "clockIn": "Jam Masuk",
            "clockOut": "Jam Keluar",
            "confirm": "Konfirmasi",
            "confirmSwitch": "Beralih ke Proyek?",
            "confirmSwitchDesc": "Ada pekerjaan yang sedang berjalan. Apakah Anda yakin ingin beralih?",
            "connectionError": "Gangguan Koneksi",
            "currentTask": "Pekerjaan Saat Ini",
            "currentWindow": "Jendela Saat Ini",
            "Dark Mode": "Mode Gelap",
            "darkMode": "Mode Gelap",
            "delete": "Hapus",
            "deskmon": "Deskmon",
            "dismiss": "Abaikan",
            "emptyCredentials": "Username dan password tidak boleh kosong",
            "english": "English",
            "filterDate": "Pilih Tanggal",
            "finishTask": "Selesaikan Pekerjaan",
            "idleDetected": "Tidak Aktif Terdeteksi",
            "idleMessage": "Tidak ada aktivitas mouse atau keyboard dalam beberapa waktu.",
            "indonesian": "Bahasa Indonesia",
            "language": "Bahasa",
            "Language": "Bahasa",
            "Light Mode": "Mode Terang",
            "lightMode": "Mode Terang",
            "loginAgain": "Masuk Kembali",
            "loginFailed": "Gagal masuk",
            "logout": "Keluar",
            "Logout": "Keluar",
            "mode": "Mode Tampilan",
            "monitoredApps": "Aplikasi Terpantau",
            "My Profile": "Profil Saya",
            "needReview": "Tandai Perlu Review",
            "neutral": "Netral",
            "noActiveTask": "Tidak Ada Pekerjaan Aktif",
            "nonProductive": "Non-Produktif",
            "password": "Kata Sandi",
            "passwordPlaceholder": "Masukkan kata sandi",
            "pauseTask": "Jeda Pekerjaan",
            "productive": "Produktif",
            "productivityStatistics": "Statistik Produktivitas",
            "profile": "Profil Saya",
            "Profile": "Profil",
            "recordedClockIn": "Tercatat masuk pada jam: ",
            "refresh": "Segarkan",
            "Refresh": "Segarkan",
            "resume": "Lanjutkan",
            "resumeTask": "Lanjutkan Pekerjaan",
            "retry": "Coba Lagi",
            "save": "Simpan",
            "seeAll": "Lihat Semua",
            "seeLess": "Lebih Sedikit",
            "selectDateRange": "Rentang Tanggal",
            "settings": "Pengaturan",
            "Settings": "Pengaturan",
            "signIn": "Masuk",
            "signingIn": "Sedang masuk...",
            "signInSubtitle": "Masuk untuk melanjutkan pemantauan pekerjaan Anda",
            "Switch to Dark Mode": "Beralih ke Mode Gelap",
            "Switch to Light Mode": "Beralih ke Mode Terang",
            "switchProject": "Beralih Proyek",
            "switchToDarkMode": "Beralih ke Mode Gelap",
            "switchToLightMode": "Beralih ke Mode Terang",
            "target": "Target",
            "taskDetails": "Detail Pekerjaan",
            "timeAtWork": "Waktu Kerja",
            "timeUp": "Waktu Habis",
            "timeUpFinished": "Waktu pekerjaan Anda sudah habis.",
            "timeUpWarning": "Waktu pekerjaan Anda tersisa kurang dari 10 menit!",
            "today": "Hari Ini",
            "topApps": "Aplikasi Teratas",
            "topDomains": "Situs Teratas",
            "username": "Nama Pengguna",
            "usernamePlaceholder": "Masukkan username atau email",
            "waitingClockIn": "Menunggu data jam masuk...",
            "welcomeBack": "Selamat Datang Kembali",
            "windowTitle": "Judul"
        }
    })

    /**
     * Fungsi penerjemah: I18n.t("key", "Default Fallback")
     */
    function t(key, fallback) {
        var langDict = strings[currentLanguage];
        if (langDict && langDict[key] !== undefined) {
            return langDict[key];
        }
        var enDict = strings["en"];
        if (enDict && enDict[key] !== undefined) {
            return enDict[key];
        }
        return fallback !== undefined ? fallback : key;
    }

    /**
     * Alias singkat agar mudah diketik: Lang.tr("Logout")
     */
    function tr(key, fallback) {
        return t(key, fallback);
    }

    /**
     * Mengubah bahasa aktif ("en" atau "id")
     */
    function setLanguage(code) {
        if (strings[code]) {
            currentLanguage = code;
        }
    }
}
