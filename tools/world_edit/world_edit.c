#include <stdio.h>
#include <raylib.h>
#include <raymath.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define WE_COLOR_GRAY_LIGHT ((Color){192, 192, 192, 255})
#define ZOOM_MIN 1.0f
#define ZOOM_MAX 60.0f
#define ZOOM_SHOW_PIXELS 10.0f
#define ZOOM_SHOW_TILES 1.0f

#define COLOR_GB_OFF ((Color){194, 207, 168, 255})
#define COLOR_GB_LIGHT ((Color){173, 191, 146, 255})
#define COLOR_GB_MID_LIGHT ((Color){150, 166, 124, 255})
#define COLOR_GB_MID_DARK ((Color){114, 126, 100, 255})
#define COLOR_GB_DARK ((Color){90, 99, 92, 255})

typedef struct Tile
{
    Texture2D *texture;
    uint8_t color_indexes[8*8]; // NOTE(jkk): In range 0-3
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
    SetTargetFPS(60);

    dpi = GetWindowScaleDPI();

    Level level = {0};
    level.width = 128;
    level.height = 64;
    level.tiles = calloc(level.width * level.height, sizeof(*level.tiles));

    Color gb_colors_0[] =
    {
        COLOR_GB_OFF,
        COLOR_GB_LIGHT,
        COLOR_GB_MID_LIGHT,
        COLOR_GB_MID_DARK,
        COLOR_GB_DARK,
    };

    Image test_image = GenImageColor(8, 8, gb_colors_0[1]);
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
        Vector2 mouse_pos_world = GetScreenToWorld2D(mouse_pos_screen, camera);
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

        // if (camera.zoom > ZOOM_SHOW_LINES_THRESHOLD)
        // {
        //     Vector2 world_view_min = (Vector2){0, 0};
        //     Vector2 world_view_max = (Vector2){level.width * 8.0f, level.height * 8.0f};
        //     Color line_color = (Color){0, 0, 0, 48};
        //
        //     for (float y = world_view_min.y; y < world_view_max.y; y += 1.0f)
        //     {
        //         Vector2 start_pos = {world_view_min.x, y};
        //         Vector2 end_pos = {world_view_max.x, y};
        //         DrawLineEx(start_pos, end_pos, 0.1f, line_color);
        //     }
        //
        //     for (float x = world_view_min.x; x < world_view_max.x; x += 1.0f)
        //     {
        //         Vector2 start_pos = {x, world_view_min.y};
        //         Vector2 end_pos = {x, world_view_max.y};
        //         DrawLineEx(start_pos, end_pos, 0.1f, line_color);
        //     }
        // }

        Vector2 pixel_rect_min = (Vector2) {floorf(mouse_pos_world.x), floorf(mouse_pos_world.y)};
        DrawRectangleV(pixel_rect_min, (Vector2){1, 1}, (Color){255, 255, 0, 192});

        EndMode2D();

        if (camera.zoom > ZOOM_SHOW_TILES)
        {
            Vector2 grid_start = GetWorldToScreen2D((Vector2){0,0}, camera);
            Vector2 grid_end = GetWorldToScreen2D((Vector2){level.width * 8.0f, level.height * 8.0f}, camera);

            Color line_color = (Color){0, 0, 0, 48};

            // for (float y = grid_start.y; y < grid_end.y; y += screen_pixels_per_world_pixel)
            for (int y = 0; y < level.height * 8; ++y)
            {
                float yf = grid_start.y + y * camera.zoom;
                Vector2 start_pos = {grid_start.x, yf};
                Vector2 end_pos = {grid_end.x, yf};
                bool on_tile_boundary = (y & 0x7) == 0;
                if (on_tile_boundary || (y & 7) && camera.zoom > ZOOM_SHOW_PIXELS) {
                    float line_width = on_tile_boundary ? 3.0f : 1.0f;
                    DrawLineEx(start_pos, end_pos, line_width, line_color);
                }
            }

            for (int x = 0; x < level.width * 8; ++x)
            {
                float xf = grid_start.x + x * camera.zoom;
                Vector2 start_pos = {xf, grid_start.y};
                Vector2 end_pos = {xf, grid_end.y};
                bool on_tile_boundary = (x & 0x7) == 0;
                if (on_tile_boundary || (x & 7) && camera.zoom > ZOOM_SHOW_PIXELS) {
                    float line_width = on_tile_boundary ? 3.0f : 1.0f;
                    DrawLineEx(start_pos, end_pos, line_width, line_color);
                }
            }
        }        EndDrawing();
    }
    return 0;
}
