bool do_test_decompile(const char *asm_path) {
    bool ret = true;

    NomCmd cmd = {0};

    if(!nom_cmd_run(&cmd, "nasm", asm_path, "-o", "test_asm_diff_nasm.out")) nom_return_defer(false);

    cmd.out_path = "test_asm_diff_sim86.asm";
    if(!nom_cmd_run(&cmd, "./sim86", "decompile", "test_asm_diff_nasm.out")) nom_return_defer(false);

    if(!nom_cmd_run(&cmd, "nasm", "test_asm_diff_sim86.asm", "-o", "test_asm_diff_sim86.out")) nom_return_defer(false);

    if(!nom_cmd_run(&cmd, "diff", "test_asm_diff_nasm.out", "test_asm_diff_sim86.out")) nom_return_defer(false);

defer:
    if(!ret) {
        printf("File `%s` could not be decompiled correctly\n", asm_path);
    }
    nom_cmd_free(&cmd);
    return ret;
}

bool walkable_do_test_compile(const char *path, NomFileType type, NomFileStats *ftw, va_list args) {
    if(!(type == NOM_FILE_REG && ftw->path_len >= 4 && strcmp(path + ftw->path_len - 4, ".asm") == 0)) {
        return true;
    }

    return do_test_decompile(path);
}

int test_decompile(int argc, const char **argv) {
    printf("\n");
    bool success = true;

    if(argc > 0) {
        for(int i = 0; i < argc; i++) {
            success = do_test_decompile(argv[i]) && success;
        }
    } else {
        // If no files provided, run for all asm files in test directory
        success = nom_files_read_dir("test/decompile", walkable_do_test_compile);
    }

    nom_delete("test_asm_diff_sim86.asm");
    nom_delete("test_asm_diff_nasm.out");
    nom_delete("test_asm_diff_sim86.out");

    if(success) {
        printf("All files decompiled correctly\n\n");
    }

    return success ? 0 : 1;
}
