#![windows_subsystem = "windows"]

use eframe::egui;
use std::fs;
use aes_gcm::{
    aead::{Aead, KeyInit, rand_core::RngCore},
    Aes256Gcm, Nonce, Key
};
use argon2::{
    Argon2,
    password_hash::{PasswordHasher, SaltString},
};
use rand::rngs::OsRng;

// Структура локализации
struct Localization {
    title: &'static str,
    main_menu: &'static str,
    engine: &'static str,
    lang_label: &'static str,
    file_label: &'static str,
    browse_btn: &'static str,
    pass_label: &'static str,
    encrypt_radio: &'static str,
    decrypt_radio: &'static str,
    action_btn: &'static str,
    stats_label: &'static str,
    log_label: &'static str,
}

fn get_localization(lang: &str) -> Localization {
    if lang.starts_with("ru") {
        Localization {
            title: "MiiraCrypt",
            main_menu: "Главное меню",
            engine: "AES-256-GCM + Argon2id",
            lang_label: "Язык / Language:",
            file_label: "Файл:",
            browse_btn: "Обзор...",
            pass_label: "Пароль:",
            encrypt_radio: "Зашифровать",
            decrypt_radio: "Расшифровать",
            action_btn: "Запустить шифрование",
            stats_label: "Статистика сессии: Обработано файлов:",
            log_label: "Журнал операций в реальном времени:",
        }
    } else if lang.starts_with("de") {
        Localization {
            title: "MiiraCrypt",
            main_menu: "Hauptmenü",
            engine: "AES-256-GCM + Argon2id",
            lang_label: "Sprache:",
            file_label: "Datei:",
            browse_btn: "Durchsuchen...",
            pass_label: "Passwort:",
            encrypt_radio: "Verschlüsseln",
            decrypt_radio: "Entschlüsseln",
            action_btn: "Verschlüsselung starten",
            stats_label: "Sitzungsstatistik: Verarbeitete Dateien:",
            log_label: "Echtzeit-Protokoll:",
        }
    } else if lang.starts_with("fr") {
        Localization {
            title: "MiiraCrypt",
            main_menu: "Menu principal",
            engine: "AES-256-GCM + Argon2id",
            lang_label: "Langue :",
            file_label: "Fichier :",
            browse_btn: "Parcourir...",
            pass_label: "Mot de passe :",
            encrypt_radio: "Chiffrer",
            decrypt_radio: "Déchiffrer",
            action_btn: "Lancer le chiffrement",
            stats_label: "Statistiques : Fichiers traités :",
            log_label: "Journal des opérations :",
        }
    } else if lang.starts_with("es") {
        Localization {
            title: "MiiraCrypt",
            main_menu: "Menú principal",
            engine: "AES-256-GCM + Argon2id",
            lang_label: "Idioma:",
            file_label: "Archivo:",
            browse_btn: "Explorar...",
            pass_label: "Contraseña:",
            encrypt_radio: "Cifrar",
            decrypt_radio: "Descifrar",
            action_btn: "Iniciar cifrado",
            stats_label: "Estadísticas: Archivos procesados:",
            log_label: "Registro de operaciones:",
        }
    } else {
        Localization {
            title: "MiiraCrypt",
            main_menu: "Main Menu",
            engine: "AES-256-GCM + Argon2id",
            lang_label: "Language:",
            file_label: "File:",
            browse_btn: "Browse...",
            pass_label: "Password:",
            encrypt_radio: "Encrypt",
            decrypt_radio: "Decrypt",
            action_btn: "Start Operation",
            stats_label: "Session Stats: Files processed:",
            log_label: "Real-time operation log:",
        }
    }
}

fn detect_system_language() -> String {
    std::env::var("LANG").unwrap_or_else(|_| "ru".to_string())
}

struct MyApp {
    current_lang: String,
    loc: Localization,
    file_path: String,
    password: String,
    is_encrypt: bool,
    status_log: String,
    processed_count: usize,
}

impl Default for MyApp {
    fn default() -> Self {
        let sys_lang = detect_system_language();
        let lang_code = if sys_lang.starts_with("de") {
            "de"
        } else if sys_lang.starts_with("fr") {
            "fr"
        } else if sys_lang.starts_with("es") {
            "es"
        } else if sys_lang.starts_with("en") {
            "en"
        } else {
            "ru"
        };

        Self {
            current_lang: lang_code.to_string(),
            loc: get_localization(lang_code),
            file_path: String::new(),
            password: String::new(),
            is_encrypt: true,
            status_log: "Приложение запущено. Готово к работе.".to_string(),
            processed_count: 0,
        }
    }
}

// Функция шифрования файла через Argon2id + AES-256-GCM
fn encrypt_file(file_path: &str, password: &str) -> Result<(), String> {
    let data = fs::read(file_path).map_err(|e| format!("Ошибка чтения файла: {}", e))?;
    
    let salt = SaltString::generate(&mut OsRng);
    let argon2 = Argon2::default();
    
    let mut key_bytes = [0u8; 32];
    let password_hash = argon2
        .hash_password(password.as_bytes(), &salt)
        .map_err(|e| format!("Ошибка хэширования пароля: {}", e))?;
    
    let hash_str = password_hash.hash.ok_or("Ошибка генерации ключа")?;
    let raw_hash = hash_str.as_bytes();
    let len = std::cmp::min(raw_hash.len(), 32);
    key_bytes[..len].copy_from_slice(&raw_hash[..len]);

    let key = Key::<Aes256Gcm>::from_slice(&key_bytes);
    let cipher = Aes256Gcm::new(key);
    
    let mut nonce_bytes = [0u8; 12];
    OsRng.fill_bytes(&mut nonce_bytes);
    let nonce = Nonce::from_slice(&nonce_bytes);

    let ciphertext = cipher
        .encrypt(nonce, data.as_ref())
        .map_err(|_| "Ошибка шифрования AES-256-GCM")?;

    let mut output_data = Vec::new();
    let salt_bytes = salt.as_str().as_bytes();
    output_data.push(salt_bytes.len() as u8);
    output_data.extend_from_slice(salt_bytes);
    output_data.extend_from_slice(&nonce_bytes);
    output_data.extend_from_slice(&ciphertext);

    let out_path = format!("{}.miira", file_path);
    fs::write(&out_path, output_data).map_err(|e| format!("Ошибка сохранения файла: {}", e))?;
    
    Ok(())
}

// Функция расшифровки файла
fn decrypt_file(file_path: &str, password: &str) -> Result<(), String> {
    let file_data = fs::read(file_path).map_err(|e| format!("Ошибка чтения файла: {}", e))?;
    if file_data.len() < 13 {
        return Err("Файл поврежден или имеет неверный формат".into());
    }

    let salt_len = file_data[0] as usize;
    if file_data.len() < 1 + salt_len + 12 {
        return Err("Неверный формат зашифрованного файла".into());
    }

    let salt_str = std::str::from_utf8(&file_data[1..1 + salt_len])
        .map_err(|_| "Ошибка чтения соли")?;
    let salt = SaltString::from_b64(salt_str)
        .map_err(|_| "Ошибка парсинга соли")?;

    let nonce_bytes = &file_data[1 + salt_len..1 + salt_len + 12];
    let ciphertext = &file_data[1 + salt_len + 12..];

    let argon2 = Argon2::default();
    let mut key_bytes = [0u8; 32];
    let password_hash = argon2
        .hash_password(password.as_bytes(), &salt)
        .map_err(|e| format!("Ошибка проверки пароля: {}", e))?;

    let hash_str = password_hash.hash.ok_or("Ошибка ключа")?;
    let raw_hash = hash_str.as_bytes();
    let len = std::cmp::min(raw_hash.len(), 32);
    key_bytes[..len].copy_from_slice(&raw_hash[..len]);

    let key = Key::<Aes256Gcm>::from_slice(&key_bytes);
    let cipher = Aes256Gcm::new(key);
    let nonce = Nonce::from_slice(nonce_bytes);

    let plaintext = cipher
        .decrypt(nonce, ciphertext)
        .map_err(|_| "Неверный пароль или поврежденный файл!")?;

    let out_path = if file_path.ends_with(".miira") {
        file_path.strip_suffix(".miira").unwrap().to_string()
    } else {
        format!("{}.decrypted", file_path)
    };

    fs::write(&out_path, plaintext).map_err(|e| format!("Ошибка записи файла: {}", e))?;
    Ok(())
}

impl eframe::App for MyApp {
    fn update(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {
        egui::CentralPanel::default().show(ctx, |ui| {
            ui.horizontal(|ui| {
                ui.heading(self.loc.main_menu);
                ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
                    ui.label(self.loc.lang_label);
                    
                    let selected_name = match self.current_lang.as_str() {
                        "ru" => "Русский",
                        "de" => "Deutsch",
                        "fr" => "Français",
                        "es" => "Español",
                        _    => "English",
                    };

                    egui::ComboBox::from_id_salt("lang_selector")
                        .selected_text(selected_name)
                        .show_ui(ui, |ui| {
                            let mut change_lang = |code: &str, name: &str| {
                                if ui.selectable_value(&mut self.current_lang, code.to_string(), name).clicked() {
                                    self.loc = get_localization(code);
                                }
                            };
                            change_lang("ru", "Русский");
                            change_lang("en", "English");
                            change_lang("de", "Deutsch");
                            change_lang("fr", "Français");
                            change_lang("es", "Español");
                        });
                });
            });

            ui.label(self.loc.engine);
            ui.add_space(15.0);

            ui.horizontal(|ui| {
                ui.label(self.loc.file_label);
                ui.text_edit_singleline(&mut self.file_path);
                if ui.button(self.loc.browse_btn).clicked() {
                    if let Some(path) = rfd::FileDialog::new().pick_file() {
                        self.file_path = path.display().to_string();
                    }
                }
            });

            ui.horizontal(|ui| {
                ui.label(self.loc.pass_label);
                ui.add(egui::TextEdit::singleline(&mut self.password).password(true));
            });

            ui.horizontal(|ui| {
                ui.radio_value(&mut self.is_encrypt, true, self.loc.encrypt_radio);
                ui.radio_value(&mut self.is_encrypt, false, self.loc.decrypt_radio);
            });

            ui.add_space(10.0);
            if ui.button(self.loc.action_btn).clicked() {
                if self.file_path.is_empty() {
                    self.status_log = "Ошибка: Выберите файл для обработки!".into();
                } else if self.password.is_empty() {
                    self.status_log = "Ошибка: Введите пароль!".into();
                } else {
                    let result = if self.is_encrypt {
                        encrypt_file(&self.file_path, &self.password)
                    } else {
                        decrypt_file(&self.file_path, &self.password)
                    };

                    match result {
                        Ok(_) => {
                            self.processed_count += 1;
                            self.status_log = format!("Успешно! Операция завершена для: {}", self.file_path);
                        }
                        Err(err) => {
                            self.status_log = format!("Ошибка: {}", err);
                        }
                    }
                }
            }

            ui.add_space(20.0);
            ui.separator();
            ui.label(format!("{} {}", self.loc.stats_label, self.processed_count));
            ui.label(self.loc.log_label);
            ui.colored_label(egui::Color32::LIGHT_BLUE, &self.status_log);
        });
    }
}

fn main() -> Result<(), eframe::Error> {
    let loc = get_localization("ru");
    let options = eframe::NativeOptions {
        viewport: egui::ViewportBuilder::default()
            .with_inner_size([600.0, 420.0])
            .with_title(loc.title),
        ..Default::default()
    };

    eframe::run_native(
        "MiiraCrypt",
        options,
        Box::new(|_| Ok(Box::new(MyApp::default()))),
    )
}