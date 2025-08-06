#include <assert.h>
#include <stdio.h>
#include <raylib.h>
#include <raymath.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "../../nob.h"

#define WE_COLOR_GRAY_LIGHT ((Color){192, 192, 192, 255})
#define ZOOM_MIN 1.0f
#define ZOOM_MAX 60.0f
#define ZOOM_SHOW_PIXELS 10.0f
#define ZOOM_SHOW_TILES 2.5f
#define ZOOM_SHOW_TILE_INDEXES ZOOM_SHOW_TILES

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

typedef union TileGbFormat
{
    // Tiles in the game boy are stored line by line, using 2 bytes per line.
    // For each line, the first byte specifies the least significant bit of the
    // color ID of each pixel, and the second byte specifies the most
    // significant bit. In both bytes, bit 7 represents the leftmost pixel, and
    // bit 0 the rightmost.
    uint8_t u8[16];
    uint64_t u64[2];
} TileGbFormat;

typedef struct TileGfx
{
    Color pixels[8*8];
    uint8_t indexes[8*8]; // NOTE(jkk): In range 0-3
    Texture2D texture;
} TileGfx;

typedef struct TileSet
{
    // Dynamic array of tiles
    TileGfx *items;
    size_t count;
    size_t capacity;
} TileSet;

typedef struct Tile
{
    uint32_t index;
    bool solid;
} Tile;

typedef struct Level
{
    Tile *tiles;
    size_t width;
    size_t height;
} Level;

typedef enum Editor_Mode
{
    EDITOR_MODE_DRAW_TILES,
    EDITOR_MODE_DRAW_PIXELS,
} Editor_Mode;

typedef struct Editor
{
    Camera2D camera;
    Vector2 pointer_prev;
    uint8_t color_index; // NOTE(jkk): In range 0-3
    Editor_Mode mode;
    bool hide_grid;
    bool show_tile_indexes;
    Level level;
    TileSet tile_set;
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

static TileGfx create_tile_gfx(uint8_t color_index)
{
    assert(color_index < 4);

    TileGfx gfx = {0};
    for (int i = 0; i < 8*8; ++i)
    {
        gfx.pixels[i] = palette_gbp[color_index];
        gfx.indexes[i] = color_index;
    }

    gfx.texture = LoadTextureFromImage((Image){
        .data = (void *)&gfx.pixels,
        .width = 8,
        .height = 8,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
    });
    UpdateTexture(gfx.texture, (void *)&gfx.pixels);

    return gfx;
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    printf("Hello, World!\n");

    InitWindow(960, 576, "Hello, Raylib!");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(60);

    dpi = GetWindowScaleDPI();

    Editor *editor = calloc(1, sizeof(*editor));
    editor->camera.offset = (Vector2){0, 0};
    editor->camera.target = (Vector2){0, 0};
    editor->camera.rotation = 0;
    editor->camera.zoom = 5.0f;
    editor->mode = EDITOR_MODE_DRAW_PIXELS;

    editor->level.width = 128;
    editor->level.height = 64;
    editor->level.tiles = calloc(editor->level.width * editor->level.height, sizeof(*editor->level.tiles));

    // Create initial tile 0

    TileGfx tile0 = create_tile_gfx(COLOR_GB_LIGHT);
    da_append(&editor->tile_set, tile0);

    while (!WindowShouldClose())
    {
        Vector2 mouse_pos_screen = GetMousePosition();
        Vector2 mouse_delta = Vector2Subtract(mouse_pos_screen, editor->pointer_prev);
        editor->pointer_prev = mouse_pos_screen;
        Vector2 mouse_pos_world = GetScreenToWorld2D(mouse_pos_screen, editor->camera);
        float mouse_scroll = GetMouseWheelMoveV().y;

        if (IsKeyPressed(KEY_G))  editor->hide_grid = !editor->hide_grid;
        if (IsKeyPressed(KEY_I))  editor->show_tile_indexes = !editor->show_tile_indexes;

        if (IsKeyPressed(KEY_ONE)) editor->color_index = COLOR_GB_DARK;
        if (IsKeyPressed(KEY_TWO)) editor->color_index = COLOR_GB_MID_DARK;
        if (IsKeyPressed(KEY_THREE)) editor->color_index = COLOR_GB_MID_LIGHT;
        if (IsKeyPressed(KEY_FOUR)) editor->color_index = COLOR_GB_LIGHT;

        if (!IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && !IsKeyDown(KEY_SPACE))
        {
            mouse_delta = (Vector2){0};
        }
        ViewUpdate(&editor->camera, mouse_scroll, mouse_pos_screen, ZOOM_MIN, ZOOM_MAX, mouse_delta);

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
                if (tile_ix >= 0 && tile_ix < editor->level.width &&
                    tile_iy >= 0 && tile_iy < editor->level.height)
                {
                    tile = &editor->level.tiles[tile_iy * editor->level.width + tile_ix];

                    assert(px_x >= 0 && px_x < 8 && px_y >= 0 && px_y < 8);
                    uint8_t color_index = editor->color_index;

                    TileGfx *gfx = &editor->tile_set.items[tile->index];
                    gfx->pixels[px_y * 8 + px_x] = palette_gbp[color_index];
                    gfx->indexes[px_y * 8 + px_x] = color_index;

                    UpdateTexture(gfx->texture, (void *)&gfx->pixels);
                }
            }
        }


        BeginDrawing();
        BeginMode2D(editor->camera);
        ClearBackground(palette_gbp[COLOR_GB_OFF]);
        for (int y = 0; y < editor->level.height; ++y)
        {
            for (int x = 0; x < editor->level.width; ++x)
            {
                Texture2D texture = editor->tile_set.items[editor->level.tiles[y * editor->level.width + x].index].texture;
                DrawTexture(texture, x*8, y*8, WHITE);
            }
        }


        EndMode2D();
        /////////////////////////////
        //                         //
        // END OF CAMERA DRAW MODE //
        //                         //
        /////////////////////////////

        if (editor->mode == EDITOR_MODE_DRAW_PIXELS && editor->camera.zoom > ZOOM_SHOW_PIXELS)
        {
            Vector2 pixel_rect_min_world = (Vector2) {floorf(mouse_pos_world.x), floorf(mouse_pos_world.y)};
            Vector2 pixel_rect_min = GetWorldToScreen2D(pixel_rect_min_world, editor->camera);

            Color draw_color = palette_gbp[editor->color_index];
            Rectangle pixel_rect = {
                .x = pixel_rect_min.x,
                .y = pixel_rect_min.y,
                .width = editor->camera.zoom,
                .height = editor->camera.zoom,
            };
            DrawRectangleRec(pixel_rect, draw_color);
            DrawRectangleLinesEx(pixel_rect, 3, BLACK);
        }

        if (!editor->hide_grid && editor->camera.zoom > ZOOM_SHOW_TILES)
        {
            Vector2 grid_start = GetWorldToScreen2D((Vector2){0,0}, editor->camera);
            Vector2 grid_end = GetWorldToScreen2D((Vector2){editor->level.width * 8.0f, editor->level.height * 8.0f}, editor->camera);

            Color line_color = (Color){0, 0, 0, 48};

            // for (float y = grid_start.y; y < grid_end.y; y += screen_pixels_per_world_pixel)
            for (int y = 0; y < editor->level.height * 8; ++y)
            {
                float yf = grid_start.y + y * editor->camera.zoom;
                Vector2 start_pos = {grid_start.x, yf};
                Vector2 end_pos = {grid_end.x, yf};
                bool on_tile_boundary = (y & 0x7) == 0;
                if (on_tile_boundary || (y & 7) && editor->camera.zoom > ZOOM_SHOW_PIXELS) {
                    float line_width = on_tile_boundary ? 3.0f : 1.0f;
                    DrawLineEx(start_pos, end_pos, line_width, line_color);
                }
            }

            for (int x = 0; x < editor->level.width * 8; ++x)
            {
                float xf = grid_start.x + x * editor->camera.zoom;
                Vector2 start_pos = {xf, grid_start.y};
                Vector2 end_pos = {xf, grid_end.y};
                bool on_tile_boundary = (x & 0x7) == 0;
                if (on_tile_boundary || (x & 7) && editor->camera.zoom > ZOOM_SHOW_PIXELS) {
                    float line_width = on_tile_boundary ? 3.0f : 1.0f;
                    DrawLineEx(start_pos, end_pos, line_width, line_color);
                }
            }
        }

        if (editor->show_tile_indexes && editor->camera.zoom > ZOOM_SHOW_TILE_INDEXES)
        {
            Vector2 grid_start = GetWorldToScreen2D((Vector2){0,0}, editor->camera);

            Font font = GetFontDefault();
            for (int y = 0; y < editor->level.height; ++y)
            {
                float yf = grid_start.y + y * 8.0f * editor->camera.zoom;

                for (int x = 0; x < editor->level.width; ++x)
                {
                    float xf = grid_start.x + x * 8.0f * editor->camera.zoom;
                    int tile_index = editor->level.tiles[y * editor->level.width + x].index;
                    Vector2 pos = { xf + editor->camera.zoom * 0.5f, yf + editor->camera.zoom * 0.5f};
                    DrawTextEx(font, TextFormat("%d", tile_index), pos, 24, 1.0f, BLACK);
                }
            }
        }


        DrawText(TextFormat("mouse diff: (%04.2f, %04.2f)", mouse_delta.x, mouse_delta.y), 10, 10, 24, BLACK);
        EndDrawing();

    }
    return 0;
}
