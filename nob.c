#include <stdio.h>
#include <string.h>

#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

static Cmd cmd;

#define PATH_RGBDS "tools/rgbds/"
#define EXE_BGB "tools/bgb/bgb.exe"
#define PATH_BUILD "build/"

int main(int argc, char **argv)
{
    const char *arg;

    NOB_GO_REBUILD_URSELF(argc, argv);

    if (!mkdir_if_not_exists(PATH_BUILD)) return 1;

    cmd_append(&cmd, PATH_RGBDS "rgbasm.exe", "-o", PATH_BUILD "main.obj", "main.s");
    if (!cmd_run_sync_and_reset(&cmd)) return 1;

    cmd_append(&cmd, PATH_RGBDS "rgblink.exe", "-m", PATH_BUILD "main.map", "-n", PATH_BUILD "main.sym", "-o", PATH_BUILD "main.gb", PATH_BUILD "main.obj");
    if (!cmd_run_sync_and_reset(&cmd)) return 1;

    cmd_append(&cmd, PATH_RGBDS "rgbfix.exe", "-p", "0", "-v", PATH_BUILD "main.gb");
    if (!cmd_run_sync_and_reset(&cmd)) return 1;

    if (argc > 1)
    {
        shift(argv, argc);
        if (strcmp("run", *argv) == 0)
        {
            cmd_append(&cmd, EXE_BGB, PATH_BUILD "main.gb");
            if (!cmd_run_sync_and_reset(&cmd)) return 1;
        }
    }

    printf("Build successfully");
    return 0;
}
