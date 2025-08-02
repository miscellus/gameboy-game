#include <stdio.h>
#include <raylib.h>


int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    printf("Hello, World!\n");

    InitWindow(800, 600, "Hello, Raylib!");

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(RED);

        EndDrawing();
    }
    return 0;
}
