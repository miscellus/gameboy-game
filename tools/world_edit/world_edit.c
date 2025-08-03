#include <stdio.h>
#include <raylib.h>
#include <raymath.h>
#include <stdlib.h>
#include <string.h>

#define WE_COLOR_GRAY_LIGHT ((Color){192, 192, 192, 255})
#define ZOOM_MIN 0.05f
#define ZOOM_MAX 50.0f
#define ZOOM_SHOW_LINES_THRESHOLD 10.0f

typedef struct Tile
{
    Texture2D *texture;
    bool solid;
} Tile;

typedef struct Level
{
    Tile *tiles;
    size_t width;
    size_t height;
} Level;

static Vector2 dpi;

void CameraZoomByFactor(Camera2D *camera, float zoom_factor, float zoom_min, float zoom_max)
{
	camera->zoom += zoom_factor * camera->zoom;
	camera->zoom = Clamp(camera->zoom, zoom_min, zoom_max);
}

void CameraSetZoomTarget(Camera2D *camera, Vector2 target)
{
	Vector2 world_target = GetScreenToWorld2D(target, *camera);
	camera->offset = target;
	camera->target = world_target;
}

void CameraUpdateZoom(Camera2D *camera, float zoom_input, Vector2 zoom_target, float zoom_min, float zoom_max)
{
    if (zoom_input == 0) return;

    float zoom_factor = 0.1f * zoom_input;
    CameraSetZoomTarget(camera, zoom_target);
    CameraZoomByFactor(camera, zoom_factor, zoom_min, zoom_max);
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    printf("Hello, World!\n");

    InitWindow(800, 600, "Hello, Raylib!");
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    dpi = GetWindowScaleDPI();

    Level level = {0};
    level.width = 128;
    level.height = 64;
    level.tiles = calloc(level.width * level.height, sizeof(*level.tiles));

    Image test_image = GenImageGradientRadial(8, 8, 1.0f, WHITE, BLACK);
    Texture2D test_texture = LoadTextureFromImage(test_image);

    for (size_t y = 0; y < level.height; ++y)
    {
        for (size_t x = 0; x < level.width; ++x)
        {
            level.tiles[y * level.width + x].texture = &test_texture;
        }
    }

    Camera2D camera = {0};
    camera.offset = (Vector2){0, 0};
    camera.target = (Vector2){0, 0};
    camera.rotation = 0;
    camera.zoom = 5.0f;

    while (!WindowShouldClose())
    {
        Vector2 mouse_pos_screen = GetMousePosition();
        float mouse_scroll = GetMouseWheelMoveV().y;

        CameraUpdateZoom(&camera, mouse_scroll, mouse_pos_screen, ZOOM_MIN, ZOOM_MAX);



        BeginDrawing();
        BeginMode2D(camera);
        ClearBackground(WE_COLOR_GRAY_LIGHT);
        for (int y = 0; y < level.height; ++y)
        {
            for (int x = 0; x < level.width; ++x)
            {
                Texture2D texture = *level.tiles[y * level.width + x].texture;
                DrawTexture(texture, x*8, y*8, WHITE);
            }
        }

        if (camera.zoom > ZOOM_SHOW_LINES_THRESHOLD)
        {
            Vector2 world_view_min = (Vector2){0, 0};
            Vector2 world_view_max = (Vector2){level.width * 8.0f, level.height * 8.0f};
            Color line_color = (Color){0, 0, 0, 48};

            for (float y = world_view_min.y; y < world_view_max.y; y += 1.0f)
            {
                Vector2 start_pos = {world_view_min.x, y};
                Vector2 end_pos = {world_view_max.x, y};
                DrawLineV(start_pos, end_pos, line_color);
            }

            for (float x = world_view_min.x; x < world_view_max.x; x += 1.0f)
            {
                Vector2 start_pos = {x, world_view_min.y};
                Vector2 end_pos = {x, world_view_max.y};
                DrawLineV(start_pos, end_pos, line_color);
            }
        }

        EndMode2D();

        DrawRectangle((int)mouse_pos_screen.x - 10, (int)mouse_pos_screen.y - 10, 20, 20, BLACK);

        DrawText(TextFormat("DPI: (%g, %g)", dpi.x, dpi.y), 20, 20, (int)(24 * dpi.y), RED);
        EndDrawing();
    }
    return 0;
}
