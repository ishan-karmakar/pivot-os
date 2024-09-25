use std::{fs, path::Path, process::Command};

use regex::Regex;

const ASM_FILES: [&str; 1] = [
    "asm/intr.asm"
];

// const GLUE_FILES: [&str; 0] = [
// ];

fn main() {
    if !Path::new("build").exists() {
        fs::create_dir("build").unwrap();
    }
    compile_asm();
    // compile_glue();
    println!("cargo:rustc-link-search=build/");
    println!("cargo:rerun-if-changed=linker.ld");
}

fn compile_asm() {
    let out_files = generate_out_files(&ASM_FILES);
    for (fin, fout) in ASM_FILES.iter().zip(out_files.iter()) {
        assert!(Command::new("nasm").args(["-felf64", fin, "-o"]).arg(fout).spawn().unwrap().wait().unwrap().success());
        println!("cargo:rerun-if-changed={}", fin);
    }
    assert!(Command::new("ar").arg("rcs").arg("build/libasm.a").args(out_files).spawn().unwrap().wait().unwrap().success());
    println!("cargo:rustc-link-lib=static=asm");
}

// fn compile_glue() {
//     let out_files = generate_out_files(&GLUE_FILES);
//     for (fin, fout) in GLUE_FILES.iter().zip(out_files.iter()) {
//         assert!(Command::new("gcc").args(["-ffreestanding", "-mno-sse", "-mno-sse2", "-mno-red-zone", "-mno-mmx", "-Ibuddy_alloc/", "-c"]).arg(fin).arg("-o").arg(fout).spawn().unwrap().wait().unwrap().success());
//         println!("cargo:rerun-if-changed={}", fin);
//     }
//     assert!(Command::new("ar").arg("rcs").arg("build/libglue.a").args(out_files).spawn().unwrap().wait().unwrap().success());
//     println!("cargo:rustc-link-lib=static=glue");
// }

fn generate_out_files(in_files: &[&str]) -> Vec<String> {
    let re = Regex::new(r"^[a-z]+/([a-z]+\.[a-z]+)").unwrap();
    let replace = "build/${1}.o";
    in_files.iter().map(|f| re.replace(f, replace).to_string()).collect::<Vec<String>>()
}