use std::{fs, path::Path, process::Command};

const ASM_FILES: [&str; 1] = [
    "asm/intr.asm"
];

fn main() {
    if !Path::new("build").exists() {
        fs::create_dir("build").unwrap();
    }
    let out_asm: Vec<String> = ASM_FILES.iter().map(|asm| asm.replace("asm/", "build/") + ".o").collect();
    for (fin, fout) in ASM_FILES.iter().zip(out_asm.iter()) {
        assert!(Command::new("nasm").args(["-felf64", fin, "-o"]).arg(fout).spawn().unwrap().wait().unwrap().success());
        println!("cargo:rerun-if-changed={}", fin);
    }
    assert!(Command::new("ar").arg("rcs").arg("build/libasm.a").args(out_asm).spawn().unwrap().wait().unwrap().success());
    println!("cargo:rustc-link-search=build/");
    println!("cargo:rustc-link-lib=static=asm");
    println!("cargo:rerun-if-changed=linker.ld");
}