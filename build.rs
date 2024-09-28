use std::{env::set_var, fs, path::Path};

fn main() {
    if !Path::new("build").exists() {
        fs::create_dir("build").unwrap();
    }
    nasm_rs::compile_library("libasm.a", &["asm/intr.asm"]).unwrap();
    set_var("CFLAGS", "-ffreestanding -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -fno-stack-protector -fno-exceptions");
    cc::Build::new()
        .file("flanterm/flanterm.c")
        .file("flanterm/backends/fb.c")
        .compile("glue");
    println!("cargo:rustc-link-lib=static=asm");
    println!("cargo:rerun-if-changed=linker.ld");
}