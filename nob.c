#define NOB_IMPLEMENTATION

#include "nob.h"

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd, "cc", "-O2", "-Wall", "-Wextra", "-Wformat", "-Wformat-security", "-fstack-protector-strong",
                   "-D_FORTIFY_SOURCE=2", "-pipe", "-std=c17", "-MMD", "-MP", "-pthread", "-o", "out/exception-filter",
                   "src/main.c", "src/http.c", "src/tp.c");

    if (!nob_cmd_run_sync(cmd)) return 1;
    return 0;
}
