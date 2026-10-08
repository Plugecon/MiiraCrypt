What new:
1. Removing C++ from the main program code as a method to improve efficiency. I removed C++, leaving only Rust in the main code of the program. This way, the two languages won’t interfere with each other, and compilation and development will be easier, and the system load will be reduced.

2. I created an installer using InnoSetup to simplify the system installation. Now you can install the program in just a couple of clicks, without any extra hassle, without having to dig through the hard drive and create shortcuts to the program.

3. I changed the encryption algorithm from AES-256-CBC to AES-256-GCM and Argon2id.
