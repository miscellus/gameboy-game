#include <assert.h>
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

typedef enum Palette_Index
{
    COLOR_GB_DARK = 0,
    COLOR_GB_MID_DARK = 1,
    COLOR_GB_MID_LIGHT = 2,
    COLOR_GB_LIGHT = 3,
    COLOR_GB_OFF = 4,
} Palette_Index;

static Color palette_gbp[] =
{
    [COLOR_GB_DARK] = {90, 99, 92, 255},
    [COLOR_GB_MID_DARK] = {114, 126, 100, 255},
    [COLOR_GB_MID_LIGHT] = {150, 166, 124, 255},
    [COLOR_GB_LIGHT] = {173, 191, 146, 255},
    [COLOR_GB_OFF] = {194, 207, 168, 255},
};


typedef struct TileGfx
{
    Color pixels[8*8];
    uint8_t indexes[8*8]; // NOTE(jkk): In range 0-3
    Texture2D texture;
} TileGfx;

typedef struct Tile
{
    TileGfx *gfx;
    bool solid;
} Tile;

typedef struct Level
{
    Tile *tiles;
    size_t width;
    size_t height;
} Level;

typedef struct Editor
{
    Camera2D camera;
    Vector2 pointer_prev;
} Editor;

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

void ViewUpdate(Camera2D *camera, float zoom_input, Vector2 zoom_target, float zoom_min, float zoom_max, Vector2 delta_offset)
{
    if (zoom_input)
    {
        float zoom_factor = 0.1f * zoom_input;
        CameraSetZoomTarget(camera, zoom_target);
        CameraZoomByFactor(camera, zoom_factor, zoom_min, zoom_max);
    }

    if (delta_offset.x != 0 && delta_offset.y != 0) {
        camera->offset = Vector2Add(camera->offset, delta_offset);
    }
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

    TileGfx test_gfx = {0};
    test_gfx.texture = LoadTextureFromImage((Image){
        .data = (void *)&test_gfx.pixels,
        .width = 8,
        .height = 8,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
    });

    for (int i = 0; i < 8*8; ++i) test_gfx.pixels[i] = palette_gbp[COLOR_GB_LIGHT];

    for (size_t y = 0; y < level.height; ++y)
    {
        for (size_t x = 0; x < level.width; ++x)
        {
            level.tiles[y * level.width + x].gfx = &test_gfx;
        }
    }

    Editor editor = {0};
    editor.camera.offset = (Vector2){0, 0};
    editor.camera.target = (Vector2){0, 0};
    editor.camera.rotation = 0;
    editor.camera.zoom = 5.0f;

    while (!WindowShouldClose())
    {
        Vector2 mouse_pos_screen = GetMousePosition();
        Vector2 mouse_delta = Vector2Subtract(mouse_pos_screen, editor.pointer_prev);
        editor.pointer_prev = mouse_pos_screen;
        Vector2 mouse_pos_world = GetScreenToWorld2D(mouse_pos_screen, editor.camera);
        float mouse_scroll = GetMouseWheelMoveV().y;

        if (!IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && !IsKeyDown(KEY_SPACE))
        {
            mouse_delta = (Vector2){0};
        }
        ViewUpdate(&editor.camera, mouse_scroll, mouse_pos_screen, ZOOM_MIN, ZOOM_MAX, mouse_delta);

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            // GetTileAtPoint
            {
                float tile_x = mouse_pos_world.x / 8.0f;
                float tile_y = mouse_pos_world.y / 8.0f;
                int tile_ix = (int)tile_x;
                int tile_iy = (int)tile_y;

                int px_x = (int)(8 * (tile_x - tile_ix));
                int px_y = (int)(8 * (tile_y - tile_iy));

                Tile *tile = NULL;
                if (tile_ix >= 0 && tile_ix < level.width &&
                    tile_iy >= 0 && tile_iy < level.height)
                {
                    tile = &level.tiles[tile_iy * level.width + tile_ix];

                    assert(px_x >= 0 && px_x < 8 && px_y >= 0 && px_y < 8);
                    uint8_t color_index = COLOR_GB_DARK;
                    tile->gfx->pixels[px_y * 8 + px_x] = palette_gbp[color_index];
                    tile->gfx->indexes[px_y * 8 + px_x] = color_index;

                    UpdateTexture(tile->gfx->texture, (void *)&tile->gfx->pixels);
                }
            }
        }


        BeginDrawing();
        BeginMode2D(editor.camera);
        ClearBackground(WE_COLOR_GRAY_LIGHT);
        for (int y = 0; y < level.height; ++y)
        {
            for (int x = 0; x < level.width; ++x)
            {
                Texture2D texture = level.tiles[y * level.width + x].gfx->texture;
                DrawTexture(texture, x*8, y*8, WHITE);
            }
        }

        Vector2 pixel_rect_min = (Vector2) {floorf(mouse_pos_world.x), floorf(mouse_pos_world.y)};
        DrawRectangleV(pixel_rect_min, (Vector2){1, 1}, (Color){255, 255, 0, 192});

        EndMode2D();

        if (editor.camera.zoom > ZOOM_SHOW_TILES)
        {
            Vector2 grid_start = GetWorldToScreen2D((Vector2){0,0}, editor.camera);
            Vector2 grid_end = GetWorldToScreen2D((Vector2){level.width * 8.0f, level.height * 8.0f}, editor.camera);

            Color line_color = (Color){0, 0, 0, 48};

            // for (float y = grid_start.y; y < grid_end.y; y += screen_pixels_per_world_pixel)
            for (int y = 0; y < level.height * 8; ++y)
            {
                float yf = grid_start.y + y * editor.camera.zoom;
                Vector2 start_pos = {grid_start.x, yf};
                Vector2 end_pos = {grid_end.x, yf};
                bool on_tile_boundary = (y & 0x7) == 0;
                if (on_tile_boundary || (y & 7) && editor.camera.zoom > ZOOM_SHOW_PIXELS) {
                    float line_width = on_tile_boundary ? 3.0f : 1.0f;
                    DrawLineEx(start_pos, end_pos, line_width, line_color);
                }
            }

            for (int x = 0; x < level.width * 8; ++x)
            {
                float xf = grid_start.x + x * editor.camera.zoom;
                Vector2 start_pos = {xf, grid_start.y};
                Vector2 end_pos = {xf, grid_end.y};
                bool on_tile_boundary = (x & 0x7) == 0;
                if (on_tile_boundary || (x & 7) && editor.camera.zoom > ZOOM_SHOW_PIXELS) {
                    float line_width = on_tile_boundary ? 3.0f : 1.0f;
                    DrawLineEx(start_pos, end_pos, line_width, line_color);
                }
            }
        }
        DrawText(TextFormat("mouse diff: (%04.2f, %04.2f)", mouse_delta.x, mouse_delta.y), 10, 10, 24, BLACK);
        EndDrawing();

    }
    return 0;
}
