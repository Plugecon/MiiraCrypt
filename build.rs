fn main() {
    let mut build = cc::Build::new();
    
    build.cpp(true)
         .std("c++17")
         .file("cpp/aes.cpp")
         .file("cpp/fileio.cpp");

    if std::env::var("CARGO_CFG_TARGET_OS").unwrap() != "windows" {
        println!("cargo:rustc-link-lib=crypto"); // Подключаем OpenSSL на Linux
    }

    build.compile("crypto_engine");
}