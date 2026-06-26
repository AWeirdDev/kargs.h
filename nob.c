#include <stddef.h>
#include <stdio.h>

#define NOB_IMPLEMENTATION
#include "vendor/nob.h"

#define BUILD_FOLDER "build/"

int main(int argc, char** argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    if (!nob_mkdir_if_not_exists(BUILD_FOLDER)) return 1;

    Nob_Cmd cmd = {0};

    nob_cc(&cmd);
    nob_cc_flags(&cmd);
    nob_cc_output(&cmd, BUILD_FOLDER "example");
    nob_cc_inputs(&cmd, "example.c");
    if (!nob_cmd_run(&cmd)) return 1;

    nob_cmd_append(&cmd, "./build/example");
    nob__cmd_append(&cmd, argc - 1, (const char**)(argv + 1));

    if (!nob_cmd_run(&cmd)) return 1;

    return 0;
}
