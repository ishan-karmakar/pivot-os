use std::{fs, path::Path};

const ASM_FILES: [&str; 1] = [
    "asm/intr.asm"
];

fn main() {
    if !Path::new("build").exists() {
        fs::create_dir("build").unwrap();
    }
    nasm_rs::compile_library("libasm.a", &ASM_FILES).unwrap();
    println!("cargo:rustc-link-lib=static=asm");
    println!("cargo:rerun-if-changed=linker.ld");
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