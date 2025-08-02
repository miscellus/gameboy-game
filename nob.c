#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

static Cmd cmd;

#define PATH_RGBDS "tools\\rgbds\\"
#define EXE_BGB "tools\\bgb\\bgb.exe"
#define PATH_BUILD "build\\"

#define WORLD_EDIT_OUT_PATH PATH_BUILD "world_edit.exe"

#define RAYLIB_INCLUDE "tools\\world_edit\\raylib\\include"
#define RAYLIB_LIB "tools\\world_edit\\raylib\\lib"

bool build_world_edit(void)
{

    cmd_append(&cmd, "cl", "-nologo", "-Od", "-Zi", "-std:c11", "-W4", "-WX", "-FC");
    cmd_append(&cmd, "gdi32.lib", "msvcrt.lib", "raylib.lib", "winmm.lib", "user32.lib", "shell32.lib");
    cmd_append(&cmd, "-Fe:" PATH_BUILD "world_edit.exe");
    cmd_append(&cmd, "tools\\world_edit\\world_edit.c");
    cmd_append(&cmd, "-I" RAYLIB_INCLUDE);

    cmd_append(&cmd, "/link");
    cmd_append(&cmd, "/libpath:" RAYLIB_LIB);
    cmd_append(&cmd, "/NODEFAULTLIB:libcmt");

    if (!cmd_run_sync_and_reset(&cmd)) return false;
    return true;
}

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    if (!mkdir_if_not_exists(PATH_BUILD)) return 1;

    cmd_append(&cmd, PATH_RGBDS "rgbasm.exe", "-o", PATH_BUILD "main.obj", "main.s");
    if (!cmd_run_sync_and_reset(&cmd)) return 1;

    cmd_append(&cmd, PATH_RGBDS "rgblink.exe", "-m", PATH_BUILD "main.map", "-n", PATH_BUILD "main.sym", "-o", PATH_BUILD "main.gb", PATH_BUILD "main.obj");
    if (!cmd_run_sync_and_reset(&cmd)) return 1;

    cmd_append(&cmd, PATH_RGBDS "rgbfix.exe", "-p", "0", "-v", PATH_BUILD "main.gb");
    if (!cmd_run_sync_and_reset(&cmd)) return 1;

    bool run = false;
    bool gb = false;
    bool world_edit = false;

    while (argc > 1)
    {
        shift(argv, argc);
        if (strcmp("run", *argv) == 0)
        {
            run = true;
        }
        else if (strcmp("gb", *argv) == 0)
        {
            gb = true;
        }
        else if (strcmp("we", *argv) == 0)
        {
            world_edit = true;
        }
        else
        {
            fprintf(stderr, "Unknown argument: `%s`\n", *argv);
        }
    }

    if (run)
    {
        if (world_edit)
        {
            cmd_append(&cmd, PATH_BUILD "world_edit.exe");
            if (!cmd_run_sync_and_reset(&cmd)) return 1;
        }
        else
        {
            cmd_append(&cmd, EXE_BGB, PATH_BUILD "main.gb");
            if (!cmd_run_sync_and_reset(&cmd)) return 1;
        }
    }

    log(INFO, "Building World Edit\n");
    if (!build_world_edit()) return 1;

    printf("Build successfully");
    return 0;
}
