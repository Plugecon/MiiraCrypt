# MiiraCrypt

A lightweight command-line file encryption tool for Windows, built in C using AES-256-CBC via the native Windows BCrypt API. No third-party dependencies.

---

## Features

- **AES-256-CBC** encryption — industry-standard symmetric encryption
- **PBKDF2** key derivation (100 000 iterations) — your password is never used directly as a key
- **Random salt + IV** — every encrypted file is unique, even with the same password
- **No verification header** — encrypted files contain no metadata identifying them as encrypted, supporting plausible deniability
- **Zero dependencies** — uses Windows BCrypt API built into the OS
- Works on any file type — executables, documents, archives, images

---

## Usage

```
micrpt crypt   <input> <output> <password>
micrpt decrypt <input> <output> <password>
```

**Encrypt a file:**
```
micrpt crypt   1.exe 2.exe mypassword123
```

**Decrypt it back:**
```
micrpt decrypt 2.exe 3.exe mypassword123
```

Password must be at least 4 characters.

---

## How it works

```
Password + Random Salt  →  PBKDF2 (100 000 iterations)  →  256-bit key
256-bit key + Random IV  →  AES-256-CBC  →  Encrypted file
```

Output file structure:

```
[ 16 bytes salt ][ 16 bytes IV ][ encrypted data ]
```

Salt and IV are randomly generated for each encryption. Encrypting the same file twice with the same password produces different output — making frequency analysis impossible.

Since there are no magic bytes or headers in the output, the encrypted file is indistinguishable from random data. This provides **plausible deniability** — it is impossible to prove a file is encrypted without the correct password.

---

## Building

Requires Visual Studio with C/C++ workload.

1. Clone the repository
2. Open `MiiraCrypt-main.slnx`
3. Build with `Ctrl+Shift+B`

Links against `bcrypt.lib` which ships with Windows SDK — no extra installs needed.

---

## Adding to PATH

1. Copy `micrpt.exe` to a folder
2. Add that folder to your system PATH
3. Open a new terminal and run `micrpt` from anywhere

---

## Project structure

```
MiiraCrypt/
├── main.cpp       — CLI argument parsing and entry point
├── aes.cpp        — AES-256-CBC encrypt / decrypt via BCrypt
├── aes.h
├── fileio.cpp     — file read / write helpers
└── fileio.h
```

---

## Security notes

- Passwords shorter than 4 characters are rejected
- PBKDF2 with 100 000 iterations significantly slows down brute-force attacks
- No magic bytes or headers — output is indistinguishable from random data
- Intended for educational purposes and personal use

---

## License

MIT
