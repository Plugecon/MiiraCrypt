## What's New in Version 3.0

### 1. Pure Rust Architecture (Goodbye C++)
We have completely removed C++ from the core program code, transitioning to a **100% pure Rust** implementation (`eframe` / `egui`). 
* **Benefits:** Elimination of language friction, streamlined compilation, significantly reduced system resource consumption, and much easier code maintenance.

### 2. Professional InnoSetup Installer
Say manual file copying and shortcut creation goodbye. Version 3.0 introduces a custom **InnoSetup installer (`.iss`)**.
* **Benefits:** Install the application in just a couple of clicks, choose your custom installation directory, and automatically set up Start Menu/Desktop shortcuts alongside `.miira` file extension association.

### 3. Advanced Cryptography (AES-256-GCM + Argon2id)
The core encryption engine has been upgraded from legacy AES-256-CBC to state-of-the-art authenticated encryption.
* **Details:** Utilizes **AES-256-GCM** combined with memory-hard **Argon2id** key derivation (using a unique random salt and Nonce for every file). Encrypted files are securely packaged into the `.miira` format.

---

## Key Features

* **Military-Grade Security:** AES-256-GCM encryption paired with Argon2id password hashing.
* **Clean & Modern UI:** Built with `egui` featuring a sleek dark theme and zero promotional branding.
* **Multi-Language Support:** Full localization for **English, Russian, German, French, and Spanish** with manual dropdown switching and auto-detection.
* **Seamless File Handling:** Integrated native file dialogs (`rfd`) for quick file selection.
* **Real-time Logs:** Live session status tracking and built-in processed file counters.

---

## Tech Stack

* **Language:** Rust (`eframe`, `egui`)
* **Cryptography:** `aes-gcm`, `argon2`, `rand`
* **File Dialogs:** `rfd`
* **Installer:** InnoSetup
