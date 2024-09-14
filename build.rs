use std::process::Command;

const ASM_FILES: [&str; 1] = [
    "src/intr.asm"
];

fn main() {
    Command::new("make").arg("build/libasm.a").output().unwrap();
    for asm in ASM_FILES {
        println!("cargo:rerun-if-changed={}", asm);
    }
    println!("cargo:rustc-link-search=build/");
    println!("cargo:rustc-link-lib=static=asm");
    println!("cargo:rerun-if-changed=linker.ld");
}