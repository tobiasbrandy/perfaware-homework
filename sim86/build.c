#define NOM_ALLOW_FORCE_REBUILD_YOURSELF
#define NOM_REBUILD_YOURSELF_CC "clang"
#define NOM_REBUILD_YOURSELF_FLAGS                                                                  \
    "-Wall", "-Wextra", "-pedantic", "-Wshadow", "-Wformat=2",                                      \
    "-Wno-unused-parameter", "-Wno-unused-function", "-Wno-implicit-fallthrough",                   \
    "-Wno-format-nonliteral",                                                                       \
    "-O0", "-ggdb3", "-fsanitize=undefined", "-fno-omit-frame-pointer", "-fstack-protector",        \
    "-fsanitize=address,pointer-compare,pointer-subtract,leak"

#define NOM_IMPLEMENTATION
#include "nom/nom.h"

#include "test/decompile/test_decompile.c"
#include "test/run/test_run.c"

#include <string.h>

void set_flags(NomCompileConfig *compile_config) {
    NomCmdFlags flags = {0};

    nom_cmd_flags_append(&flags // INCLUDES
        , "-iquote"
        , compile_config->src_dir
    );
    nom_cmd_flags_append(&flags // CFLAGS
        , "-Wall"
        , "-Wextra"
        , "-pedantic"
        , "-Wshadow"
        , "-Wformat=2"
    );
    nom_cmd_flags_append(&flags // OFF_WARNINGS
        , "-Wno-unused-parameter"
        , "-Wno-unused-function"
        , "-Wno-implicit-fallthrough"
        , "-Wno-missing-field-initializers"
    );
    nom_cmd_flags_append(&flags // DEBUG_FLAGS
        , "-O0"
        , "-ggdb3"
        , "-fsanitize=undefined"
        , "-fno-omit-frame-pointer"
        , "-fstack-protector"
   );
    nom_cmd_flags_append(&flags // GDB_INCOMPATIBLE_FLAGS
        , "-fsanitize=address,pointer-compare,pointer-subtract,leak"
    );

    compile_config->flags = flags;
}

int test(int argc, const char **argv) {
    // Skip executable name and test commando
    argc -= 2;
    argv += 2;

    if(argc > 0) {
        const char *maybe_cmd = argv[0];
        if(strcmp(maybe_cmd, "decompile") == 0) {
            return test_decompile(argc - 1, argv + 1);

        } else if(strcmp(maybe_cmd, "run") == 0) {
            return test_run(argc - 1, argv + 1);
        }
    }

    // Run all
    int ret;
    if((ret = test_decompile(argc, argv))) return ret;
    if((ret = test_run(argc, argv))) return ret;
    return 0;
}

#define DEFAULT_CMD "compile"
int main(int argc, const char **argv) {
    nom_rebuild_yourself(argc, argv, __FILE__);

    const char *cmd = argc > 1 ? argv[1] : DEFAULT_CMD;

    nom_log_set_level(NOM_INFO);
    nom_log_set_stream(stdout);

    NomCompileConfig compile_config = {
        .cc         = "clang",
        .target     = "sim86",
        .src_dir    = "src",
        .obj_dir    = "obj",
    };
    set_flags(&compile_config);

    int ret = 0;

    if(strcmp(cmd, "compile") == 0) {
        if(!nom_compile(&compile_config)) ret = 1;

    } else if(strcmp(cmd, "test") == 0) {
        if(!nom_compile(&compile_config)) ret = 1;
        if(!ret) {
            ret = test(argc, argv);
        }

    } else if(strcmp(cmd, "db") == 0) {
        if(!nom_build_compilation_database(&compile_config)) ret = 1;

    } else if(strcmp(cmd, "clean") == 0) {
        if(!nom_clean(&compile_config)) ret = 1;

    } else {
        nom_log(NOM_ERROR, "command `%s` not recognized", cmd);
        ret = 1;
    }

    nom_darr_free(&compile_config.flags);

    return ret;
}
