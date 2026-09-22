use std::env;
use std::path::PathBuf;

fn main() {
    println!("cargo:rerun-if-changed=jxrlib/");
    println!("cargo:rerun-if-changed=src/fakelibc/");

    let src = &[
        // SRC_SYS
        "jxrlib/image/sys/adapthuff.c",
        "jxrlib/image/sys/image.c",
        "jxrlib/image/sys/strcodec.c",
        "jxrlib/image/sys/strPredQuant.c",
        "jxrlib/image/sys/strTransform.c",
        "jxrlib/image/sys/perfTimerANSI.c",
        // SRC_DEC
        "jxrlib/image/decode/decode.c",
        "jxrlib/image/decode/postprocess.c",
        "jxrlib/image/decode/segdec.c",
        "jxrlib/image/decode/strdec.c",
        "jxrlib/image/decode/strInvTransform.c",
        "jxrlib/image/decode/strPredQuantDec.c",
        "jxrlib/image/decode/JXRTranscode.c",
        // SRC_ENC
        "jxrlib/image/encode/encode.c",
        "jxrlib/image/encode/segenc.c",
        "jxrlib/image/encode/strenc.c",
        "jxrlib/image/encode/strFwdTransform.c",
        "jxrlib/image/encode/strPredQuantEnc.c",
        //
        // glue lib
        "jxrlib/jxrgluelib/JXRGlue.c",
        "jxrlib/jxrgluelib/JXRGlueJxr.c",
        "jxrlib/jxrgluelib/JXRGluePFC.c",
        "jxrlib/jxrgluelib/JXRMeta.c",
        // "test" lib (contains tif encoding)
        // Most of these are unused, but the Windows linker errors
        // if we don't include them.
        "jxrlib/jxrtestlib/JXRTest.c",
        "jxrlib/jxrtestlib/JXRTestBmp.c",
        "jxrlib/jxrtestlib/JXRTestHdr.c",
        "jxrlib/jxrtestlib/JXRTestPnm.c",
        "jxrlib/jxrtestlib/JXRTestTif.c",
        "jxrlib/jxrtestlib/JXRTestYUV.c",
    ];
    let mut build = cc::Build::new();
    build
        .files(src)
        .include("jxrlib")
        .include("jxrlib/common/include")
        .include("jxrlib/image/sys")
        .include("jxrlib/jxrgluelib")
        .include("jxrlib/jxrtestlib")
        .define("__ANSI__", None)
        .define("DISABLE_PERF_MEASUREMENT", None)
        // quiet the build on mac with clang
        .flag_if_supported("-Wno-constant-conversion")
        .flag_if_supported("-Wno-unused-const-variable")
        .flag_if_supported("-Wno-deprecated-declarations")
        .flag_if_supported("-Wno-comment")
        .flag_if_supported("-Wno-unused-value")
        .flag_if_supported("-Wno-unused-function")
        .flag_if_supported("-Wno-unknown-pragmas")
        .flag_if_supported("-Wno-extra-tokens")
        .flag_if_supported("-Wno-missing-field-initializers")
        .flag_if_supported("-Wno-shift-negative-value")
        .flag_if_supported("-Wno-dangling-else")
        .flag_if_supported("-Wno-sign-compare")
        // quiet the build on linux with gcc
        .flag_if_supported("-Wno-strict-aliasing")
        .flag_if_supported("-Wno-implicit-fallthrough")
        .flag_if_supported("-Wno-old-style-declaration")
        .flag_if_supported("-Wno-endif-labels")
        .flag_if_supported("-Wno-parentheses")
        .flag_if_supported("-Wno-misleading-indentation")
        .flag_if_supported("-Wno-unused-but-set-variable")
        .flag_if_supported("-Wno-incompatible-pointer-types")
        .opt_level(2);

    let target = std::env::var("TARGET").unwrap();
    if target == "wasm32-unknown-unknown" {
        // relying on our fake libc fragment
        build
            //.define("MALLOC_PREFIX", "vp6_custom_")
            .flag("-isystem")
            .flag("src/fakelibc")
            .file("src/fakelibc/impl.c")
            .file("src/fakelibc/qsort.c");
    }

    build
        // Suppress all warnings - comment this out in local development
        .flag("-w")
        .compile("jpegxr");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    let mut clang_args = vec![
        "-D__ANSI__",
        "-DDISABLE_PERF_MEASUREMENT",
        "-Ijxrlib/jxrgluelib",
        "-Ijxrlib/common/include",
        "-Ijxrlib/image/sys",
    ];

    if target == "wasm32-unknown-unknown" {
        // We need to manually specify the visibility
        // See https://github.com/rust-lang/rust-bindgen/issues/1941
        clang_args.extend(&["-isystem", "src/fakelibc"]);
    }

    bindgen::Builder::default()
        .header("jxrlib/jxrgluelib/JXRGlue.h")
        .header("jxrlib/jxrtestlib/JXRTest.h")
        .allowlist_function("^(WMP|PK|PixelFormatLookup|GetPixelFormatFromHash|GetImageEncodeIID|GetImageDecodeIID|FreeDescMetadata|Ruffle).*")
        .allowlist_var("^(WMP|PK|LOOKUP|GUID_PK|IID).*")
        .allowlist_type("^(WMP|PK|ERR|BITDEPTH|BD_|BITDEPTH_BITS|COLORFORMAT).*")
        .clang_args(clang_args)
        .derive_eq(true)
        .no_partialeq("^(tagPKStream|tagPKFactory|tagPKCodecFactory|tagPKFormatConverter)$")
        .size_t_is_usize(true)
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        .generate()
        .expect("Error building libjpegxr bindings")
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}
