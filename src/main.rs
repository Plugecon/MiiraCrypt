#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

use eframe::egui;
use std::ffi::CString;
use std::fs;
use std::path::PathBuf;

// ============================================================================
// 1. C++ FFI БИНДИНГИ (Функции из ваших aes.h / aes.cpp)
// ============================================================================
extern "C" {
    fn aes_encrypt(
        input: *const u8,
        input_len: usize,
        password: *const i8,
        output: *mut *mut u8,
        output_len: *mut usize,
    ) -> i32;

    fn aes_decrypt(
        input: *const u8,
        input_len: usize,
        password: *const i8,
        output: *mut *mut u8,
        output_len: *mut usize,
    ) -> i32;

    fn free(ptr: *mut std::ffi::c_void);
}

// ============================================================================
// 2. СИСТЕМА ЛОКАЛИЗАЦИИ (МУЛЬТИЯЗЫЧНОСТЬ: EN / RU / DE / ES)
// ============================================================================
#[derive(PartialEq, Clone, Copy)]
pub enum Language {
    English,
    Russian,
    German,
    Spanish,
}

pub struct AppStrings {
    pub title: &'static str,
    pub subtitle: &'static str,
    pub file_label: &'static str,
    pub select_btn: &'static str,
    pub no_file: &'static str,
    pub mask_label: &'static str,
    pub mask_hint: &'static str,
    pub password_label: &'static str,
    pub encrypt_btn: &'static str,
    pub decrypt_btn: &'static str,
    pub default_status: &'static str,
    pub err_no_file: &'static str,
    pub err_short_pass: &'static str,
    pub err_read: &'static str,
    pub err_empty_file: &'static str,
    pub err_write: &'static str,
    pub err_crypto: &'static str,
    pub success_encrypt: &'static str,
    pub success_decrypt: &'static str,
}

impl Language {
    pub fn strings(&self) -> AppStrings {
        match self {
            Language::English => AppStrings {
                title: "🔒 micrpt - AES-256 Vault",
                subtitle: "File Encryption & Camouflage Tool",
                file_label: "File:",
                select_btn: "📁 Browse...",
                no_file: "No file selected",
                mask_label: "Camouflage as:",
                mask_hint: "(e.g. .exe, .sys, .dll)",
                password_label: "Password:  ",
                encrypt_btn: "🔐 Encrypt",
                decrypt_btn: "🔓 Decrypt",
                default_status: "Select a file and enter a password",
                err_no_file: "Error: Please select a file first!",
                err_short_pass: "Error: Password must be at least 4 characters!",
                err_read: "Error: Failed to read file!",
                err_empty_file: "Error: File is empty!",
                err_write: "Error: Failed to write file to disk!",
                err_crypto: "Error! Wrong password or corrupted file.",
                success_encrypt: "Encrypted & camouflaged to:",
                success_decrypt: "Successfully decrypted to:",
            },
            Language::Russian => AppStrings {
                title: "🔒 micrpt - AES-256 Vault",
                subtitle: "Шифрование и маскировка файлов",
                file_label: "Файл:",
                select_btn: "📁 Выбрать...",
                no_file: "Файл не выбран",
                mask_label: "Маскировать в:",
                mask_hint: "(например .exe, .sys, .dll)",
                password_label: "Пароль:    ",
                encrypt_btn: "🔐 Зашифровать",
                decrypt_btn: "🔓 Расшифровать",
                default_status: "Выберите файл и введите пароль",
                err_no_file: "Ошибка: Сначала выберите файл!",
                err_short_pass: "Ошибка: Пароль должен быть минимум 4 символа!",
                err_read: "Ошибка: Не удалось прочитать файл!",
                err_empty_file: "Ошибка: Файл пуст!",
                err_write: "Ошибка записи файла на диск!",
                err_crypto: "Ошибка! Неверный пароль или файл поврежден.",
                success_encrypt: "Зашифровано и замаскировано в:",
                success_decrypt: "Успешно расшифровано в:",
            },
            Language::German => AppStrings {
                title: "🔒 micrpt - AES-256 Vault",
                subtitle: "Datei-Verschlüsselung & Tarnung",
                file_label: "Datei:",
                select_btn: "📁 Durchsuchen...",
                no_file: "Keine Datei ausgewählt",
                mask_label: "Tarnen als:",
                mask_hint: "(z.B. .exe, .sys, .dll)",
                password_label: "Passwort:  ",
                encrypt_btn: "🔐 Verschlüsseln",
                decrypt_btn: "🔓 Entschlüsseln",
                default_status: "Wählen Sie eine Datei und geben Sie ein Passwort ein",
                err_no_file: "Fehler: Bitte zuerst eine Datei auswählen!",
                err_short_pass: "Fehler: Passwort muss mindestens 4 Zeichen lang sein!",
                err_read: "Fehler: Datei konnte nicht gelesen werden!",
                err_empty_file: "Fehler: Datei ist leer!",
                err_write: "Fehler: Datei konnte nicht geschrieben werden!",
                err_crypto: "Fehler! Falsches Passwort oder beschädigte Datei.",
                success_encrypt: "Verschlüsselt und getarnt als:",
                success_decrypt: "Erfolgreich entschlüsselt in:",
            },
            Language::Spanish => AppStrings {
                title: "🔒 micrpt - AES-256 Vault",
                subtitle: "Cifrado y Camuflaje de Archivos",
                file_label: "Archivo:",
                select_btn: "📁 Buscar...",
                no_file: "Ningún archivo seleccionado",
                mask_label: "Camuflar como:",
                mask_hint: "(p. ej. .exe, .sys, .dll)",
                password_label: "Contraseña:",
                encrypt_btn: "🔐 Cifrar",
                decrypt_btn: "🔓 Descifrar",
                default_status: "Seleccione un archivo e ingrese una contraseña",
                err_no_file: "¡Error: Por favor seleccione un archivo primero!",
                err_short_pass: "¡Error: La contraseña debe tener al menos 4 caracteres!",
                err_read: "¡Error: No se pudo leer el archivo!",
                err_empty_file: "¡Error: El archivo está vacío!",
                err_write: "¡Error: No se pudo escribir el archivo en el disco!",
                err_crypto: "¡Error! Contraseña incorrecta o archivo dañado.",
                success_encrypt: "Cifrado y camuflado en:",
                success_decrypt: "Descifrado con éxito en:",
            },
        }
    }
}

// ============================================================================
// 3. СОСТОЯНИЕ ПРИЛОЖЕНИЯ (APP STATE)
// ============================================================================
struct CryptoApp {
    language: Language,
    selected_file: Option<PathBuf>,
    mask_extension: String,
    password: String,
    status_text: String,
    status_is_error: bool,
}

impl Default for CryptoApp {
    fn default() -> Self {
        let initial_lang = Language::English;
        Self {
            language: initial_lang,
            selected_file: None,
            mask_extension: ".sys".to_string(),
            password: String::new(),
            status_text: initial_lang.strings().default_status.to_string(),
            status_is_error: false,
        }
    }
}

// ============================================================================
// 4. ГРАФИЧЕСКИЙ ИНТЕРФЕЙС (EGUI UI)
// ============================================================================
impl eframe::App for CryptoApp {
    fn update(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {
        ctx.set_visuals(egui::Visuals::dark());
        let s = self.language.strings();

        egui::CentralPanel::default().show(ctx, |ui| {
            // --- ШАПКА: Заголовок + Выбор языка (EN / RU / DE / ES) ---
            ui.horizontal(|ui| {
                ui.heading(s.title);
                ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
                    egui::ComboBox::from_id_source("lang_selector")
                        .selected_text(match self.language {
                            Language::English => "🇬🇧 EN",
                            Language::Russian => "🇷🇺 RU",
                            Language::German => "🇩🇪 DE",
                            Language::Spanish => "🇪🇸 ES",
                        })
                        .show_ui(ui, |ui| {
                            if ui.selectable_value(&mut self.language, Language::English, "🇬🇧 English").clicked() {
                                self.status_text = self.language.strings().default_status.to_string();
                            }
                            if ui.selectable_value(&mut self.language, Language::Russian, "🇷🇺 Русский").clicked() {
                                self.status_text = self.language.strings().default_status.to_string();
                            }
                            if ui.selectable_value(&mut self.language, Language::German, "🇩🇪 Deutsch").clicked() {
                                self.status_text = self.language.strings().default_status.to_string();
                            }
                            if ui.selectable_value(&mut self.language, Language::Spanish, "🇪🇸 Español").clicked() {
                                self.status_text = self.language.strings().default_status.to_string();
                            }
                        });
                });
            });

            ui.label(s.subtitle);
            ui.separator();
            ui.add_space(8.0);

            // --- БЛОК 1: ВЫБОР ФАЙЛА ---
            ui.horizontal(|ui| {
                ui.label(s.file_label);
                if ui.button(s.select_btn).clicked() {
                    if let Some(path) = rfd::FileDialog::new().pick_file() {
                        self.selected_file = Some(path);
                    }
                }
            });

            if let Some(ref path) = self.selected_file {
                ui.label(egui::RichText::new(path.to_string_lossy()).small().color(egui::Color32::LIGHT_BLUE));
            } else {
                ui.label(egui::RichText::new(s.no_file).small().weak());
            }

            ui.add_space(10.0);

            // --- БЛОК 2: МАСКИРОВКА (РАСШИРЕНИЕ) ---
            ui.horizontal(|ui| {
                ui.label(s.mask_label);
                ui.text_edit_singleline(&mut self.mask_extension);
                ui.label(s.mask_hint);
            });

            ui.add_space(5.0);

            // --- БЛОК 3: ПАРОЛЬ ---
            ui.horizontal(|ui| {
                ui.label(s.password_label);
                ui.add(egui::TextEdit::singleline(&mut self.password).password(true));
            });

            ui.add_space(15.0);

            // --- БЛОК 4: КНОПКИ ДЕЙСТВИЯ ---
            ui.horizontal(|ui| {
                if ui.add_sized([130.0, 32.0], egui::Button::new(s.encrypt_btn)).clicked() {
                    self.run_crypto(true);
                }

                if ui.add_sized([130.0, 32.0], egui::Button::new(s.decrypt_btn)).clicked() {
                    self.run_crypto(false);
                }
            });

            ui.add_space(15.0);
            ui.separator();

            // --- БЛОК 5: СТАТУС ---
            let color = if self.status_is_error {
                egui::Color32::RED
            } else {
                egui::Color32::GREEN
            };
            ui.label(egui::RichText::new(&self.status_text).color(color).strong());
        });
    }
}

// ============================================================================
// 5. ЛОГИКА ОБРАБОТКИ И ВЫЗОВ C++ ДВИЖКА
// ============================================================================
impl CryptoApp {
    fn run_crypto(&mut self, is_encrypt: bool) {
        let s = self.language.strings();

        let file_path = match &self.selected_file {
            Some(p) => p,
            None => {
                self.show_error(s.err_no_file);
                return;
            }
        };

        if self.password.len() < 4 {
            self.show_error(s.err_short_pass);
            return;
        }

        // Читаем файл в память
        let input_bytes = match fs::read(file_path) {
            Ok(bytes) => bytes,
            Err(_) => {
                self.show_error(s.err_read);
                return;
            }
        };

        if input_bytes.is_empty() {
            self.show_error(s.err_empty_file);
            return;
        }

        let c_password = match CString::new(self.password.clone()) {
            Ok(c_str) => c_str,
            Err(_) => {
                self.show_error(s.err_crypto);
                return;
            }
        };

        let mut out_ptr: *mut u8 = std::ptr::null_mut();
        let mut out_len: usize = 0;

        // ВЫЗОВ C++ ФУНКЦИИ
        let ok = unsafe {
            if is_encrypt {
                aes_encrypt(
                    input_bytes.as_ptr(),
                    input_bytes.len(),
                    c_password.as_ptr(),
                    &mut out_ptr,
                    &mut out_len,
                )
            } else {
                aes_decrypt(
                    input_bytes.as_ptr(),
                    input_bytes.len(),
                    c_password.as_ptr(),
                    &mut out_ptr,
                    &mut out_len,
                )
            }
        };

        // ЕСЛИ УСПЕХ (C++ вернул 1)
        if ok == 1 && !out_ptr.is_null() {
            let output_bytes = unsafe { std::slice::from_raw_parts(out_ptr, out_len) };

            // Формируем путь для сохранения файла
            let out_path = if is_encrypt {
                let mut path_str = file_path.to_string_lossy().to_string();
                path_str.push_str(&self.mask_extension);
                PathBuf::from(path_str)
            } else {
                let path_str = file_path.to_string_lossy().to_string();
                if path_str.ends_with(&self.mask_extension) {
                    PathBuf::from(&path_str[..path_str.len() - self.mask_extension.len()])
                } else {
                    PathBuf::from(format!("{}.decrypted", path_str))
                }
            };

            // Записываем результат на диск
            if fs::write(&out_path, output_bytes).is_ok() {
                self.status_is_error = false;
                self.status_text = if is_encrypt {
                    format!("{} {:?}", s.success_encrypt, out_path.file_name().unwrap())
                } else {
                    format!("{} {:?}", s.success_decrypt, out_path.file_name().unwrap())
                };
            } else {
                self.show_error(s.err_write);
            }

            // Освобождаем память C++ (malloc)
            unsafe { free(out_ptr as *mut _) };
        } else {
            self.show_error(s.err_crypto);
        }
    }

    fn show_error(&mut self, msg: &str) {
        self.status_text = msg.to_string();
        self.status_is_error = true;
    }
}

// ============================================================================
// 6. ТОЧКА ВХОДА (MAIN)
// ============================================================================
fn main() -> eframe::Result<()> {
    let options = eframe::NativeOptions {
        viewport: egui::ViewportBuilder::default()
            .with_inner_size([490.0, 360.0])
            .with_resizable(false),
        ..Default::default()
    };

    eframe::run_native(
        "micrpt - Stealth Vault",
        options,
        Box::new(|_cc| Ok(Box::new(CryptoApp::default()))),
    )
}