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

typedef struct World_Position
{
    uint32_t tile_x;
    uint32_t tile_y;
    uint8_t pixel_x;
    uint8_t pixel_y;
} World_Position;

typedef union GB_Tile_Data
{
    // NOTE(jkk): 128 bits
    //
    // Tiles in the game boy are stored line by line, using 2 bytes per line.
    // For each line, the first byte specifies the least significant bit of the
    // color ID of each pixel, and the second byte specifies the most
    // significant bit. In both bytes, bit 7 represents the leftmost pixel, and
    // bit 0 the rightmost.
    uint8_t u8[16];
    uint64_t u64[2];
} GB_Tile_Data;

typedef struct Tile
{
    uint8_t color_indexes[8*8]; // NOTE(jkk): In range 0-3
    Texture2D texture;
} Tile;

typedef struct Tiles
{
    // Dynamic array of tiles
    Tile *items;
    size_t count;
    size_t capacity;
} Tiles;

typedef struct World_Tile
{
    uint32_t index;
    bool solid;
} World_Tile;

typedef struct Level
{
    World_Tile *tiles;
    size_t width;
    size_t height;
} Level;

typedef enum Editor_Mode
{
    EDITOR_MODE_DRAW_TILES,
    EDITOR_MODE_DRAW_PIXELS,
} Editor_Mode;

typedef struct App
{
    Camera2D camera;
    Vector2 pointer_prev;
    uint8_t color_index; // NOTE(jkk): In range 0-3
    Editor_Mode mode;
    bool hide_grid;
    bool show_tile_indexes;
    Level level;
    Tiles tile_set;
} App;

App *APP;

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

static Tile CreateTile(uint8_t color_index)
{
    assert(color_index < 4);

    Tile tile = {0};
    Color pixels[8*8];

    for (int i = 0; i < 8*8; ++i)
    {
        tile.color_indexes[i] = color_index;
        pixels[i] = palette_gbp[color_index];
    }

    tile.texture = LoadTextureFromImage((Image){
        .data = (void *)&pixels,
        .width = 8,
        .height = 8,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
    });
    UpdateTexture(tile.texture, (void *)&pixels);

    return tile;
}

World_Position GetWorldPosition(Vector2 point)
{
    float tile_x = point.x / 8.0f;
    float tile_y = point.y / 8.0f;
    World_Position result = {0};
    result.tile_x = (uint32_t)floorf(tile_x);
    result.tile_y = (uint32_t)floorf(tile_y);
    result.pixel_x = (uint8_t)(8 * (tile_x - result.tile_x));
    result.pixel_y = (uint8_t)(8 * (tile_y - result.tile_y));

    return result;
}

World_Tile *GetWorldTile(uint32_t tile_x, uint32_t tile_y)
{
    Level *level = &APP->level;
    World_Tile *world_tile = NULL;
    if (tile_x >= 0 && tile_x < level->width &&
        tile_y >= 0 && tile_y < level->height)
    {
        world_tile = &level->tiles[tile_y * level->width + tile_x];
    }

    return world_tile;
}

Tile *GetTile(World_Tile *world_tile)
{
    assert(world_tile->index < APP->tile_set.count);
    return &APP->tile_set.items[world_tile->index];
}

void SetTilePixel(Tile *tile, uint8_t pixel_x, uint8_t pixel_y, uint8_t color_index)
{
    assert(pixel_x >= 0 && pixel_x < 8 && pixel_y >= 0 && pixel_y < 8);

    tile->color_indexes[pixel_y * 8 + pixel_x] = color_index;
    Color pixel = palette_gbp[color_index];
    UpdateTextureRec(tile->texture, (Rectangle){(float)pixel_x, (float)pixel_y, 1.0f, 1.0f}, &pixel);
}

void InitApp(void)
{
    APP = calloc(1, sizeof(*APP));
    APP->camera.offset = (Vector2){0, 0};
    APP->camera.target = (Vector2){0, 0};
    APP->camera.rotation = 0;
    APP->camera.zoom = 5.0f;
    APP->mode = EDITOR_MODE_DRAW_PIXELS;

    APP->level.width = 128;
    APP->level.height = 64;
    APP->level.tiles = calloc(APP->level.width * APP->level.height, sizeof(*APP->level.tiles));

    // Create initial tile 0
    Tile tile0 = CreateTile(COLOR_GB_LIGHT);
    da_append(&APP->tile_set, tile0);
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    printf("Hello, World!\n");

    InitWindow(960, 576, "Hello, Raylib!");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(60);

    InitApp();

    while (!WindowShouldClose())
    {
        Vector2 mouse_pos_screen = GetMousePosition();
        Vector2 mouse_delta = Vector2Subtract(mouse_pos_screen, APP->pointer_prev);
        APP->pointer_prev = mouse_pos_screen;
        float mouse_scroll = GetMouseWheelMoveV().y;

        if (!IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && !IsKeyDown(KEY_SPACE))
        {
            mouse_delta = (Vector2){0};
        }
        ViewUpdate(&APP->camera, mouse_scroll, mouse_pos_screen, ZOOM_MIN, ZOOM_MAX, mouse_delta);

        if (IsKeyPressed(KEY_G))  APP->hide_grid = !APP->hide_grid;
        if (IsKeyPressed(KEY_I))  APP->show_tile_indexes = !APP->show_tile_indexes;

        if (IsKeyPressed(KEY_ONE)) APP->color_index = COLOR_GB_DARK;
        if (IsKeyPressed(KEY_TWO)) APP->color_index = COLOR_GB_MID_DARK;
        if (IsKeyPressed(KEY_THREE)) APP->color_index = COLOR_GB_MID_LIGHT;
        if (IsKeyPressed(KEY_FOUR)) APP->color_index = COLOR_GB_LIGHT;

        Vector2 mouse_pos_world = GetScreenToWorld2D(mouse_pos_screen, APP->camera);

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            World_Position pos = GetWorldPosition(mouse_pos_world);
            World_Tile *world_tile = GetWorldTile(pos.tile_x, pos.tile_y);
            Tile *tile = GetTile(world_tile);
            SetTilePixel(tile, pos.pixel_x, pos.pixel_y, APP->color_index);
        }

        ///////////////////////////
        //                       //
        //        DRAWING        //
        //                       //
        ///////////////////////////

        BeginDrawing();

        // Draw tiles
        BeginMode2D(APP->camera);
        {
            ClearBackground(palette_gbp[COLOR_GB_OFF]);
            for (int y = 0; y < APP->level.height; ++y)
            {
                for (int x = 0; x < APP->level.width; ++x)
                {
                    Texture2D texture = APP->tile_set.items[APP->level.tiles[y * APP->level.width + x].index].texture;
                    DrawTexture(texture, x*8, y*8, WHITE);
                }
            }
        }
        EndMode2D();

        // Draw hovered pixel outline
        if (APP->mode == EDITOR_MODE_DRAW_PIXELS && APP->camera.zoom > ZOOM_SHOW_PIXELS)
        {
            Vector2 pixel_rect_min_world = (Vector2) {floorf(mouse_pos_world.x), floorf(mouse_pos_world.y)};
            Vector2 pixel_rect_min = GetWorldToScreen2D(pixel_rect_min_world, APP->camera);

            Color draw_color = palette_gbp[APP->color_index];
            Rectangle pixel_rect = {
                .x = pixel_rect_min.x,
                .y = pixel_rect_min.y,
                .width = APP->camera.zoom,
                .height = APP->camera.zoom,
            };
            DrawRectangleRec(pixel_rect, draw_color);
            DrawRectangleLinesEx(pixel_rect, 3, BLACK);
        }

        // Draw tile grid
        if (!APP->hide_grid && APP->camera.zoom > ZOOM_SHOW_TILES)
        {
            Vector2 grid_start = GetWorldToScreen2D((Vector2){0,0}, APP->camera);
            Vector2 grid_end = GetWorldToScreen2D((Vector2){APP->level.width * 8.0f, APP->level.height * 8.0f}, APP->camera);

            Color line_color = (Color){0, 0, 0, 48};

            for (int y = 0; y < APP->level.height * 8; ++y)
            {
                float yf = grid_start.y + y * APP->camera.zoom;
                Vector2 start_pos = {grid_start.x, yf};
                Vector2 end_pos = {grid_end.x, yf};
                bool on_tile_boundary = (y & 0x7) == 0;
                if (on_tile_boundary || (y & 7) && APP->camera.zoom > ZOOM_SHOW_PIXELS) {
                    float line_width = on_tile_boundary ? 3.0f : 1.0f;
                    DrawLineEx(start_pos, end_pos, line_width, line_color);
                }
            }

            for (int x = 0; x < APP->level.width * 8; ++x)
            {
                float xf = grid_start.x + x * APP->camera.zoom;
                Vector2 start_pos = {xf, grid_start.y};
                Vector2 end_pos = {xf, grid_end.y};
                bool on_tile_boundary = (x & 0x7) == 0;
                if (on_tile_boundary || (x & 7) && APP->camera.zoom > ZOOM_SHOW_PIXELS) {
                    float line_width = on_tile_boundary ? 3.0f : 1.0f;
                    DrawLineEx(start_pos, end_pos, line_width, line_color);
                }
            }
        }

        // Draw tile indexes
        if (APP->show_tile_indexes && APP->camera.zoom > ZOOM_SHOW_TILE_INDEXES)
        {
            Vector2 grid_start = GetWorldToScreen2D((Vector2){0,0}, APP->camera);

            Font font = GetFontDefault();
            for (int y = 0; y < APP->level.height; ++y)
            {
                float yf = grid_start.y + y * 8.0f * APP->camera.zoom;

                for (int x = 0; x < APP->level.width; ++x)
                {
                    float xf = grid_start.x + x * 8.0f * APP->camera.zoom;
                    int tile_index = APP->level.tiles[y * APP->level.width + x].index;
                    Vector2 pos = { xf + APP->camera.zoom * 0.5f, yf + APP->camera.zoom * 0.5f};
                    DrawTextEx(font, TextFormat("%d", tile_index), pos, 24, 1.0f, BLACK);
                }
            }
        }
        
        // Draw tile set
        for (int i = 0; i < APP->tile_set.count; ++i)
        {
            Texture texture = APP->tile_set.items[i].texture;

            Vector2 pos = {(float)(i % 16), (float)(i - (i % 16))};
            pos = Vector2Add(Vector2Scale(pos, 8.0f), (Vector2){64.0f, 64.0f});

            DrawTextureV(texture, pos, WHITE);
        }

        EndDrawing();

    }
    return 0;
}
