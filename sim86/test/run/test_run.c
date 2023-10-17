bool do_test_run(const char *asm_path) {
    bool ret = true;

    NomStringBuilder txt_path = {0};
    nom_sb_append_str(&txt_path, asm_path);
    txt_path.len -= 4;
    nom_sb_append_str(&txt_path, ".txt");
    nom_sb_append_null(&txt_path);

    NomCmd cmd = {0};

    if(!nom_cmd_run(&cmd, "nasm", asm_path, "-o", "test_run.out")) nom_return_defer(false);

    cmd.out_path = "test_run_trace.txt";
    if(!nom_cmd_run(&cmd, "./sim86", "trace", "test_run.out")) {
        printf("Error while running `%s`\n", asm_path);
        nom_return_defer(false);
    }

    if(!nom_cmd_run(&cmd, "diff", txt_path.items, "test_run_trace.txt")) nom_return_defer(false);

defer:
    if(!ret) {
        printf("File `%s` run trace doesn't match `%s`\n", asm_path, txt_path.items);
    }
    nom_sb_free(&txt_path);
    nom_cmd_free(&cmd);
    return ret;
}

bool walkable_do_test_run(const char *path, NomFileType type, NomFileStats *ftw, va_list args) {
    if(!(type == NOM_FILE_REG && ftw->path_len >= 4 && strcmp(path + ftw->path_len - 4, ".asm") == 0)) {
        return true;
    }

    return do_test_run(path);
}

int test_run(int argc, const char **argv) {
    printf("\n");
    bool success = true;

    if(argc > 0) {
        for(int i = 0; i < argc; i++) {
            success = do_test_run(argv[i]) && success;
        }
    } else {
        // If no files provided, run for all asm files in test directory
        success = nom_files_read_dir("test/run", walkable_do_test_run);
    }

    nom_delete("test_run_trace.txt");
    nom_delete("test_run.out");

    if(success) {
        printf("All files ran correctly\n\n");
    }

    return success ? 0 : 1;
}
