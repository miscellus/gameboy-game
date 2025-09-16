//
// TODO: Add proper licenses and credit for used libs
//


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

#include "tinyfiledialogs.h"

// OPTIONS
//#define SORT_TILES

#define COLOR_PANEL_BORDER ((Color){160, 160, 160, 255})
#define COLOR_WINDOW_BACKGROUND ((Color){192, 192, 192, 255})
#define COLOR_WORLD_BACKGROUND ((Color){170, 170, 170, 255})
#define COLOR_TILESET_BACKGROUND ((Color){170, 170, 170, 255})
#define COLOR_SOLID_BRUSH ((Color){160, 100, 20, 44})

#define ZOOM_MIN 1.0f
#define ZOOM_MAX 60.0f
#define ZOOM_SHOW_PIXELS 10.0f
#define ZOOM_SHOW_TILES 2.5f
#define ZOOM_SHOW_TILE_INDEXES 7.0f
#define ZOOM_MAX_TILE_PICKER 15.0f
#define ZOOM_MIN_TILE_PICKER 3.0f

#define INVALID_TILE_INDEX ((uint32_t)-1)
#define INVALID_TILE_ATLAS_INDEX ((uint32_t)-1)

#define TILE_ATLAS_DIM (8 * 1024)
#define TILE_ATLAS_CAPACITY (TILE_ATLAS_DIM / 8 * TILE_ATLAS_DIM / 8)

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

typedef struct
{
    uint8_t *items;
    uint32_t count;
    uint32_t capacity;
} Bytes;

typedef struct World_Position
{
    uint32_t tile_x;
    uint32_t tile_y;
    uint8_t pixel_x;
    uint8_t pixel_y;
} World_Position;

typedef struct Tile_Line_GB
{
    // NOTE(jkk): 128 bits
    //
    // Tiles in the game boy are stored line by line, using 2 bytes per line.
    // For each line, the first byte specifies the least significant bit of the
    // color ID of each pixel, and the second byte specifies the most
    // significant bit. In both bytes, bit 7 represents the leftmost pixel, and
    // bit 0 the rightmost.
    uint8_t bit_planes[2];
} Tile_Line_GB;

typedef union Tile_Packed
{
    Tile_Line_GB lines[8];
    uint8_t u8s[16];
    uint64_t u64s[2];
} Tile_Packed;

typedef struct Tile_Color_Indexes
{
    uint8_t v[8*8]; // NOTE(jkk): In range 0-3
} Tile_Color_Indexes;

typedef struct Tile
{
    Tile_Color_Indexes color_indexes;
    uint32_t ref_count;
    uint32_t tile_atlas_index;
    bool texture_needs_update;
} Tile;

typedef struct Tile_Set
{
    // Dynamic array of tiles
    Tile *items;
    uint32_t count;
    uint32_t capacity;
} Tile_Set;

typedef struct Level_Tile
{
    uint32_t index;
    bool is_solid;
} Level_Tile;

typedef struct Level
{
    Level_Tile *tiles;
    uint32_t width;
    uint32_t height;
    uint32_t tile_set_index;
} Level;

typedef struct World
{
    Level level;
    Tile_Set tile_set;
} World;

typedef enum Action_Kind
{
    ACT_LEVEL_TILE_UPDATE,
    ACT_TILE_UPDATE,
    ACT_TILE_ADD_LAST,
    ACT_TILE_DELETE_LAST,
    ACT_TILE_INSERT,
    ACT_TILE_DELETE,
} Action_Kind;

typedef struct Action_Level_Tile
{
    Level_Tile *level_tile;
    Level_Tile level_tile_delta;
} Action_Level_Tile;

typedef struct Action_Tile
{
    uint32_t tile_index;
    Tile_Packed tile_delta;
} Action_Tile;

typedef struct Action
{
    Action_Kind kind;
    uint32_t transaction_id;
    Tile_Set *tile_set;
    union
    {
        Action_Level_Tile level_action;
        Action_Tile tile_action;
    } as;
} Action;

typedef struct History
{
    Action *items;
    uint32_t count;
    uint32_t capacity;
    uint32_t undo_count;
    uint32_t current_transaction_id;
} History;

typedef enum Editor_Mode
{
    MODE_DRAW_TILES,
    MODE_DRAW_PIXELS,
} Editor_Mode;

typedef struct Tile_Atlas
{
    uint32_t *free_indexes; // Free indexes
    uint32_t free_index_count;
    Texture texture;
} Tile_Atlas;

typedef struct App
{
    // View stuff
    Camera2D camera_world;

    float side_panel_width;
    float side_panel_width_min;
    float side_panel_zoom;
    float side_panel_scroll_offset;

    Vector2 mouse_previous;
    uint8_t current_color_index; // NOTE(jkk): In range 0-3
    uint32_t current_tile_index;

    // Mode stuff
    Editor_Mode mode;
    bool current_is_solid;
    bool hide_grid;
    bool show_tile_indexes;
    bool auto_new_tile;
    bool show_game_boy_screen;

    Level_Tile *hot_tile;
    Tile_Packed tile_snapshot;

    World world;

    Tile_Atlas tile_atlas;

    History history;

    const char *currently_open_world_file_path;
    uint16_t save_file_format_version;
    Bytes serialization_buffer;
} App;

App *APP;

typedef struct KeyModifiers
{
    bool alt;
    bool ctrl;
    bool shift;
} KeyModifiers;

static inline KeyModifiers GetKeyModifiers(void)
{
    KeyModifiers result = {0};
    result.alt = IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT);
    result.ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    result.shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    return result;
}

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

void ResetTileAtlas(Tile_Atlas *tile_atlas)
{
    tile_atlas->free_index_count = TILE_ATLAS_CAPACITY;
    for (uint32_t i = 0; i < TILE_ATLAS_CAPACITY; ++i)
    {
        tile_atlas->free_indexes[i] = i;
    }
}

void InitTileAtlas(Tile_Atlas *tile_atlas)
{
    tile_atlas->free_indexes = malloc(TILE_ATLAS_CAPACITY * sizeof(*tile_atlas->free_indexes));
    tile_atlas->texture = LoadTextureFromImage((Image){
        .width = TILE_ATLAS_DIM,
        .height = TILE_ATLAS_DIM,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    });
    ResetTileAtlas(tile_atlas);
}

uint32_t GetNewTileAtlasIndex(Tile_Atlas *tile_atlas)
{
    assert(tile_atlas->free_index_count <= TILE_ATLAS_CAPACITY);
    if (tile_atlas->free_index_count == 0)
    {
        return INVALID_TILE_ATLAS_INDEX;
    }

    uint32_t index = tile_atlas->free_indexes[--tile_atlas->free_index_count];
    assert(index < TILE_ATLAS_CAPACITY);
    return (uint32_t)index;
}

void FreeTileAtlasIndex(Tile_Atlas *tile_atlas, uint32_t index)
{
    assert(index < TILE_ATLAS_CAPACITY);
    tile_atlas->free_indexes[tile_atlas->free_index_count++] = index;
}

static Color* ConvertColorIndexesToPixels(Tile_Color_Indexes color_indexes)
{
    static Color pixels[8*8];

    for (int i = 0; i < 8*8; ++i)
    {
        uint8_t color_index = color_indexes.v[i];
        assert(color_index < 4);
        pixels[i] = palette_gbp[color_index];
    }

    return pixels;
}

static void FillTile(Tile *tile, uint8_t color_index)
{
    assert(color_index < 4);

    for (uint32_t i = 0; i < 8*8; ++i)
    {
        tile->color_indexes.v[i] = color_index;
    }

    tile->texture_needs_update = true;
}

static inline Tile_Packed PackTile(Tile_Color_Indexes color_indexes)
{
    Tile_Packed tile_pk = {0};

    for (uint32_t i = 0; i < 8*8; ++i)
    {
        uint8_t color_index = color_indexes.v[i];
        assert(color_index < 4);
        tile_pk.u8s[i / 4] |= color_index << (2*(i % 4));
    }

    return tile_pk;
}

static inline Tile_Packed XorPackedTile(Tile_Packed a, Tile_Packed b)
{
    a.u64s[0] ^= b.u64s[0];
    a.u64s[1] ^= b.u64s[1];
    return a;
}

Tile_Color_Indexes UnpackTile(Tile_Packed tile_pk)
{
    Tile_Color_Indexes color_indexes = {0};

    for (uint32_t i = 0; i < 8*8; ++i)
    {
        uint8_t packed = tile_pk.u8s[i / 4];
        color_indexes.v[i] = (packed >> (2*(i % 4))) & 3;
    }

    return color_indexes;
}

static inline Tile_Packed PackTileGB(Tile_Color_Indexes color_indexes)
{
    Tile_Packed tile_gb = {0};

    for (int y = 0; y < 8; ++y)
    {
        for (int x = 0; x < 8; ++x)
        {
            uint8_t pixel = color_indexes.v[8*y + x];
            if (pixel & 1) tile_gb.lines[y].bit_planes[0] |= (0x80 >> x);
            if (pixel & 2) tile_gb.lines[y].bit_planes[1] |= (0x80 >> x);
        }
    }

    return tile_gb;
}

Tile_Color_Indexes UnpackTileGB(Tile_Packed tile_gb)
{
    Tile_Color_Indexes color_indexes = {0};

    for (int y = 0; y < 8; ++y)
    {
        for (int x = 0; x < 8; ++x)
        {
            uint8_t pixel = 0;
            if (tile_gb.lines[y].bit_planes[0] & (0x80 >> x)) pixel |= 1;
            if (tile_gb.lines[y].bit_planes[1] & (0x80 >> x)) pixel |= 2;
            color_indexes.v[8 * y + x] = pixel;
        }
    }

    return color_indexes;
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

Level_Tile *LevelGetTile(Level level, uint32_t tile_x, uint32_t tile_y)
{
    Level_Tile *level_tile = NULL;
    if (tile_x < level.width && tile_y < level.height)
    {
        level_tile = &level.tiles[tile_y * level.width + tile_x];
    }

    return level_tile;
}

Tile *GetTile(Tile_Set *tile_set, uint32_t index)
{
    assert(index < tile_set->count);
    return &tile_set->items[index];
}

void SetTilePixel(Tile *tile, uint8_t pixel_x, uint8_t pixel_y, uint8_t color_index)
{
    assert(pixel_x < 8 && pixel_y < 8);
    assert(color_index < 4);

    tile->color_indexes.v[pixel_y * 8 + pixel_x] = color_index;
    tile->texture_needs_update = true;
}

Tile CreateTile(Tile_Atlas *atlas)
{
    Tile tile = {0};
    tile.tile_atlas_index = GetNewTileAtlasIndex(atlas);
    tile.texture_needs_update = true;
    return tile;
}

void CopyTilePixels(Tile *dst_tile, Tile *src_tile)
{
    memcpy(&dst_tile->color_indexes, &src_tile->color_indexes, sizeof(src_tile->color_indexes));
    dst_tile->texture_needs_update = true;
}

void DeleteLastTile(Tile_Set *tile_set, Tile_Atlas *tile_atlas)
{
    Tile deleted_tile = tile_set->items[--tile_set->count];
    FreeTileAtlasIndex(tile_atlas, deleted_tile.tile_atlas_index);
}

void InitWorld(World *world, uint32_t level_width, uint32_t level_height)
{
    world->level.width = level_width;
    world->level.height = level_height;
    size_t num_tiles = world->level.width * world->level.height;
    world->level.tiles = malloc(num_tiles*sizeof(*world->level.tiles));
    memset(world->level.tiles, 0xcd, num_tiles*sizeof(*world->level.tiles));

    world->tile_set.capacity = 0;
    world->tile_set.count = 0;
    world->tile_set.items = NULL;

    for (size_t i = 0; i < num_tiles; ++i)
    {
        world->level.tiles[i].index = 0;
        world->level.tiles[i].is_solid = 0;
    }

    Tile tile = CreateTile(&APP->tile_atlas);
    FillTile(&tile, COLOR_GB_LIGHT);
    tile.ref_count = (uint32_t)num_tiles;
    da_append(&world->tile_set, tile);
}

void InitApp(void)
{
    APP = malloc(sizeof(*APP));
    memset(APP, 0xcd, sizeof(*APP));

    APP->camera_world.offset = (Vector2){0};
    APP->camera_world.target = (Vector2){0};
    APP->camera_world.rotation = 0.0f;
    APP->camera_world.zoom = 5.0f;

    APP->side_panel_width_min = 300.0f;
    APP->side_panel_width = 200.0f;
    APP->side_panel_zoom = 4.0f;
    APP->side_panel_scroll_offset = 0.0f;

    APP->mouse_previous = (Vector2){0};
    APP->current_color_index = 0; // NOTE(jkk): In range 0-3
    APP->current_tile_index = 0;
    APP->current_is_solid = false;

    APP->mode = MODE_DRAW_PIXELS;
    APP->current_is_solid = false;
    APP->hide_grid = false;
    APP->show_game_boy_screen = false;
    APP->show_tile_indexes = false;
    APP->auto_new_tile = true;

    InitTileAtlas(&APP->tile_atlas);

    InitWorld(&APP->world, 128, 64);

    APP->history = (History){0};

    APP->hot_tile = NULL;

    APP->currently_open_world_file_path = NULL;
    APP->save_file_format_version = 0;
    APP->serialization_buffer = (Bytes){0};
}

Rectangle TileAtlasIndexToRect(uint32_t index)
{
    assert(index < TILE_ATLAS_CAPACITY);
    uint32_t tile_x = index % (TILE_ATLAS_DIM / 8);
    uint32_t tile_y = index / (TILE_ATLAS_DIM / 8);
    float x = tile_x * 8.0f;
    float y = tile_y * 8.0f;
    return (Rectangle){x, y, 8.0f, 8.0f};
}

Rectangle CutRectGetTop(Rectangle r, float s) { r.height = s; return r; }
Rectangle CutRectGetBottom(Rectangle r, float s) { r.y += s; r.height -= s; return r; }
Rectangle CutRectGetLeft(Rectangle r, float s) { r.width = s; return r; }
Rectangle CutRectGetRight(Rectangle r, float s) { r.x += s; r.width -= s; return r; }
Rectangle PadRect(Rectangle r, float s) { r.x += s; r.y += s; r.width -= 2*s; r.height -= 2*s; return r; }
Rectangle PadRectEx(Rectangle rect, float t, float r, float b, float l)
{
    rect.x += l;
    rect.y += t;
    rect.width -= l + r;
    rect.height -= t + b;
    return rect;
}

void DrawTileGrid(Level level)
{
    Vector2 grid_start = GetWorldToScreen2D((Vector2){0,0}, APP->camera_world);
    Vector2 grid_end = GetWorldToScreen2D((Vector2){level.width * 8.0f, level.height * 8.0f}, APP->camera_world);

    bool show_pixels = APP->camera_world.zoom > ZOOM_SHOW_PIXELS;

    for (uint32_t y = 0; y < level.height * 8; ++y)
    {
        float yf = grid_start.y + y * APP->camera_world.zoom;
        Vector2 start_pos = {grid_start.x, yf};
        Vector2 end_pos = {grid_end.x, yf};
        bool on_tile_boundary = !(y & 0x7);
        if (on_tile_boundary || show_pixels)
        {
            Color line_color = (Color){0, 0, 0, 48};
            float line_width = 1.0f;
            if (on_tile_boundary && show_pixels)
            {
                line_color.a = 60;
                line_width = 3.0f;
            }

            DrawLineEx(start_pos, end_pos, line_width, line_color);
        }
    }

    for (uint32_t x = 0; x < level.width * 8; ++x)
    {
        float xf = grid_start.x + x * APP->camera_world.zoom;
        Vector2 start_pos = {xf, grid_start.y};
        Vector2 end_pos = {xf, grid_end.y};
        bool on_tile_boundary = !(x & 0x7);
        if (on_tile_boundary || show_pixels)
        {
            Color line_color = (Color){0, 0, 0, 48};
            float line_width = 1.0f;
            if (on_tile_boundary && show_pixels)
            {
                line_color.a = 60;
                line_width = 3.0f;
            }

            DrawLineEx(start_pos, end_pos, line_width, line_color);
        }
    }
}

void DrawTileIndexes(Level level)
{
    Vector2 grid_start = GetWorldToScreen2D((Vector2){0,0}, APP->camera_world);

    Font font = GetFontDefault();
    for (uint32_t y = 0; y < level.height; ++y)
    {
        float yf = grid_start.y + y * 8.0f * APP->camera_world.zoom;

        for (uint32_t x = 0; x < level.width; ++x)
        {
            float xf = grid_start.x + x * 8.0f * APP->camera_world.zoom;
            uint32_t tile_index = level.tiles[y * level.width + x].index;
            Vector2 pos = { xf + APP->camera_world.zoom * 0.2f, yf + APP->camera_world.zoom * 0.2f};
            DrawTextEx(font, TextFormat("%d", tile_index), pos, 48, 1.0f, BLACK);
        }
    }
}

static inline uint32_t GetEditTileIndex(Tile_Set *tile_set)
{
    return tile_set->count - 1;
}

void DrawWorldView(Rectangle view, World world, Vector2 mouse_pos_screen)
{
    Level level = world.level;
    Tile_Set *tile_set = &world.tile_set;

    Vector2 mouse_pos_world = GetScreenToWorld2D(mouse_pos_screen, APP->camera_world);
    BeginScissorMode((int)view.x, (int)view.y, (int)view.width, (int)view.height);
    ClearBackground(COLOR_WORLD_BACKGROUND);

    // Draw tiles
    BeginMode2D(APP->camera_world);
    {
        for (uint32_t tile_y = 0; tile_y < level.height; ++tile_y)
        {
            for (uint32_t tile_x = 0; tile_x < level.width; ++tile_x)
            {
                Level_Tile *level_tile = &level.tiles[tile_y * level.width + tile_x];
                uint32_t tile_index = level_tile->index;
                if (APP->auto_new_tile && level_tile == APP->hot_tile)
                {
                    tile_index = GetEditTileIndex(tile_set);
                }
                assert(tile_index < tile_set->count);

                Tile *tile = GetTile(tile_set, tile_index);

                Rectangle tile_rect = {tile_x*8.0f, tile_y*8.0f, 8.0f, 8.0f};
                Rectangle rec = TileAtlasIndexToRect(tile->tile_atlas_index);
                DrawTexturePro(APP->tile_atlas.texture, rec, tile_rect, (Vector2){0}, 0, WHITE);
                if (level_tile->is_solid) DrawRectangleRec(tile_rect, COLOR_SOLID_BRUSH);
            }
        }

        // Uncomment for debugging the tile_atlas texture:
        // DrawTexture(APP->tile_atlas.texture, 0, 0, WHITE);

        if (APP->show_game_boy_screen)
        {
            float game_boy_screen_width = 160;
            float game_boy_screen_height = 144;
            Rectangle game_boy_screen_rect = {
                mouse_pos_world.x - game_boy_screen_width/2,
                mouse_pos_world.y - game_boy_screen_height/2,
                game_boy_screen_width,
                game_boy_screen_height,
            };
            DrawRectangleLinesEx(game_boy_screen_rect, 8, BLACK);
        }
    }
    EndMode2D();

    if (APP->mode == MODE_DRAW_PIXELS)
    {
        // Draw hovered pixel outline
        if (APP->camera_world.zoom > ZOOM_SHOW_PIXELS)
        {
            Vector2 pixel_rect_min_world = (Vector2) {floorf(mouse_pos_world.x), floorf(mouse_pos_world.y)};
            Vector2 pixel_rect_min = GetWorldToScreen2D(pixel_rect_min_world, APP->camera_world);

            Color draw_color = palette_gbp[APP->current_color_index];
            Rectangle pixel_rect = {
                .x = pixel_rect_min.x,
                .y = pixel_rect_min.y,
                .width = APP->camera_world.zoom,
                .height = APP->camera_world.zoom,
            };
            DrawRectangleRec(pixel_rect, draw_color);
            DrawRectangleLinesEx(pixel_rect, 3, BLACK);
        }
    }
    else if (APP->mode == MODE_DRAW_TILES)
    {
        Vector2 rect_min_world = (Vector2) {floorf(mouse_pos_world.x/8)*8, floorf(mouse_pos_world.y/8)*8};
        Vector2 rect_min = GetWorldToScreen2D(rect_min_world, APP->camera_world);
        Rectangle tile_rect = {rect_min.x, rect_min.y, 8*APP->camera_world.zoom, 8*APP->camera_world.zoom};

        if (APP->current_is_solid)
        {
            if (APP->current_is_solid) DrawRectangleRec(tile_rect, COLOR_SOLID_BRUSH);
            DrawRectangleLinesEx(tile_rect, 3, BLACK);
        }
    }

    if (!APP->hide_grid && APP->camera_world.zoom > ZOOM_SHOW_TILES)
    {
        DrawTileGrid(APP->world.level);
    }

    if (APP->show_tile_indexes && APP->camera_world.zoom > ZOOM_SHOW_TILE_INDEXES)
    {
        DrawTileIndexes(APP->world.level);
    }

    EndScissorMode();
}

static inline bool PackedTilesAreEqual(Tile_Packed a, Tile_Packed b)
{
    return a.u64s[0] == b.u64s[0] && a.u64s[1] == b.u64s[1];
}

typedef struct
{
    float tile_size;
    float gap;
    float gap_min;
    uint32_t tiles_per_row;
    uint32_t num_rows;
} Tile_Picker_Props;

Tile_Picker_Props GetTilePickerProps(Rectangle view, Tile_Set tile_set)
{
    float tile_size = APP->side_panel_zoom * 8.0f;
    if (tile_size > view.width)
    {
        tile_size = view.width;
    }

    float gap_min = 5;

    int tiles_per_row = (int)((view.width) / (tile_size + gap_min));
    if (tiles_per_row < 1) tiles_per_row = 1;
    float space_remaining = view.width - tiles_per_row * tile_size;

    float gap = space_remaining / (tiles_per_row - 1 + 2);


    Tile_Picker_Props p = {0};
    p.tile_size = tile_size;
    p.gap = gap;
    p.gap_min = gap_min;
    p.tiles_per_row = tiles_per_row;
    p.num_rows = (tile_set.count + tiles_per_row - 1) / tiles_per_row;
    return p;
}

Rectangle GetSidePanelTileRect(Rectangle view, int tile_index, Tile_Picker_Props props)
{
    int tile_x = tile_index % props.tiles_per_row;
    int tile_y = tile_index / props.tiles_per_row;
    float advance_x = props.tile_size + props.gap;
    float advance_y = props.tile_size + props.gap_min;

    return (Rectangle){
        view.x + props.gap + tile_x * advance_x,
        view.y + props.gap_min + tile_y * advance_y + APP->side_panel_scroll_offset,
        props.tile_size,
        props.tile_size
    };
}

int32_t SidePanelGetHoveredTileIndex(Rectangle view, Vector2 point, Tile_Picker_Props p)
{
    //////////////
    // X

    // make point relative to scrollable region
    float d = (point.x - p.gap - view.x) / (p.tile_size + p.gap);
    if (d < 0) return -1;

    // Return -1 if we are in the gap
    if ((d - (int)d) > p.tile_size / (float)(p.tile_size + p.gap)) return -1;

    if (d >= p.tiles_per_row) return -1;

    uint32_t tile_x = (uint32_t)d;

    //////////////
    // Y

    d = (point.y - p.gap_min - view.y - APP->side_panel_scroll_offset) / (p.tile_size + p.gap_min);
    if (d < 0) return -1;

    // Return -1 if we are in the gap
    if ((d - (int)d) > p.tile_size / (float)(p.tile_size + p.gap_min)) return -1;

    if (d > p.num_rows) return -1;

    uint32_t tile_y = (uint32_t)d;

    return tile_x + p.tiles_per_row * tile_y;
}

void DrawSidePanel(Rectangle view, Tile_Set tile_set)
{
    BeginScissorMode((int)view.x, (int)view.y, (int)view.width, (int)view.height);

    ClearBackground(COLOR_TILESET_BACKGROUND);

    Tile_Picker_Props props = GetTilePickerProps(view, tile_set);

    // Draw tile set

    Vector2 mouse = GetMousePosition();
    uint32_t hovered_tile_index = (uint32_t)SidePanelGetHoveredTileIndex(view, mouse, props);

    for (uint32_t i = 0; i < (uint32_t)tile_set.count; ++i)
    {
        Tile *tile = &tile_set.items[i];
        // Texture2D texture = GetTexture(tile->texture_index);

        Rectangle tile_rect = GetSidePanelTileRect(view, i, props);
        Color tint = tile->ref_count ? WHITE : (Color){255, 100, 100, 255};
        if (i == hovered_tile_index) tint = BLUE;

        Rectangle rec = TileAtlasIndexToRect(tile->tile_atlas_index);

        DrawTexturePro(APP->tile_atlas.texture, rec, tile_rect, (Vector2){0}, 0, tint);

        if (i == APP->current_tile_index)
        {
            float border = props.gap_min;
            Rectangle border_rect = PadRect(tile_rect, -border);
            DrawRectangleLinesEx(border_rect, border, BLACK);
        }
    }
    // DrawText(TextFormat("%0.2f", tile_size), (int)(view.x + 20), (int)(view.y + 20), 50, RED);

    EndScissorMode();
}

uint32_t FindTileMatch(Tile_Set tile_set, Tile *src_tile, uint32_t skip_index)
{
    Tile_Packed src_tile_pk = PackTile(src_tile->color_indexes);

    for (uint32_t i = 0; i < tile_set.count; ++i)
    {
        if (i == skip_index) continue;

        Tile *tile = &tile_set.items[i];
        Tile_Packed tile_pk = PackTile(tile->color_indexes);
        if (PackedTilesAreEqual(src_tile_pk, tile_pk))
        {
            return i;
        }
    }
    return INVALID_TILE_INDEX;
}

Action ActLevelTileUpdate(Level_Tile *level_tile, Level_Tile level_tile_delta, Tile_Set *tile_set)
{
    Action act = {0};
    act.kind = ACT_LEVEL_TILE_UPDATE;
    act.tile_set = tile_set;
    act.as.level_action.level_tile = level_tile;
    act.as.level_action.level_tile_delta = level_tile_delta;
    return act;
}

Action ActTileAdd(Tile_Set *tile_set, Tile_Packed tile_delta)
{
    Action act = {0};
    act.kind = ACT_TILE_ADD_LAST;
    act.tile_set = tile_set;
    act.as.tile_action.tile_delta = tile_delta;
    return act;
}

Action ActTileDeleteLast(Tile_Set *tile_set, Tile_Packed tile_delta)
{
    Action act = {0};
    act.kind = ACT_TILE_DELETE_LAST;
    act.tile_set = tile_set;
    act.as.tile_action.tile_delta = tile_delta;
    return act;
}

Action ActTileUpdate(Tile_Set *tile_set, uint32_t tile_index, Tile_Packed tile_delta)
{
    Action act = {0};
    act.kind = ACT_TILE_UPDATE;
    act.tile_set = tile_set;
    act.as.tile_action.tile_index = tile_index;
    act.as.tile_action.tile_delta = tile_delta;
    return act;
}

void Record(Action action)
{
    action.transaction_id = APP->history.current_transaction_id;
    assert(APP->history.undo_count <= APP->history.count);
    APP->history.count -= APP->history.undo_count;
    APP->history.undo_count = 0;
    da_append(&APP->history, action);
}

void EndRecordTransaction(void)
{
    ++APP->history.current_transaction_id;
}

void DeleteTile_(uint32_t tile_index, Tile_Set *tile_set)
{
    assert(tile_index < tile_set->count);

    if (tile_set->count == 1) return;

    World *world = &APP->world;

    Tile *tile = GetTile(tile_set, tile_index);
    FreeTileAtlasIndex(&APP->tile_atlas, tile->tile_atlas_index);

    // Delete tile in tile_set
    uint32_t last_index = tile_set->count - 1;
    if (tile_index < last_index)
    {
        void *dst = &tile_set->items[tile_index];
        void *src = &tile_set->items[tile_index + 1];
        size_t size = (last_index - tile_index) * sizeof(*tile_set->items);
        memmove(dst, src, size);
    }
    --tile_set->count;

    // If we delete the last tile in the set, make sure we update level tiles
    // using it to the previous index
    if (tile_index == last_index) --tile_index;

    // Update references
    for (uint32_t i = 0; i < world->level.width*world->level.height; ++i)
    {
        Level_Tile *level_tile = &world->level.tiles[i];
        if (level_tile->index > tile_index)
        {
            if (level_tile->index < tile_set->count) --tile_set->items[level_tile->index].ref_count;
            --level_tile->index;
            ++tile_set->items[level_tile->index].ref_count;
        }
    }
}

void InsertTile_(uint32_t tile_index, Tile new_tile, Tile_Set *tile_set)
{
    assert(tile_index <= tile_set->count); // allow append at count

    World *world = &APP->world;

    // Ensure capacity for one more tile
    da_reserve(tile_set, tile_set->count + 1);

    uint32_t last_index = tile_set->count;
    // If inserting not at the end, shift existing tiles up to make room
    if (tile_index < last_index)
    {
        void *dst = &tile_set->items[tile_index + 1];
        void *src = &tile_set->items[tile_index];
        size_t size = (last_index - tile_index) * sizeof(*tile_set->items);
        memmove(dst, src, size);
    }

    // Place the new tile into tile_set
    tile_set->items[tile_index] = new_tile;

    ++tile_set->count;

    // Update references in levels: any tile indices >= tile_index (before insertion)
    // must be incremented to point to the same logical tile after insertion.
    // We also need to adjust ref_counts accordingly.
    for (uint32_t i = 0; i < world->level.width * world->level.height; ++i)
    {
        Level_Tile *level_tile = &world->level.tiles[i];

        // Before insertion, valid indices were [0 .. last_index-1].
        // After insertion, any level_tile->index >= tile_index should become index+1.
        if (level_tile->index >= tile_index)
        {
            // Decrement ref_count of the tile that currently sits at level_tile->index
            // (it will move to index+1). Only adjust if the index was valid in the old range.
            if (level_tile->index < last_index) --tile_set->items[level_tile->index].ref_count;

            ++level_tile->index;

            // Increment ref_count for the tile at the new index (after shift).
            if (level_tile->index < tile_set->count) ++tile_set->items[level_tile->index].ref_count;
        }
    }
}

void PerformAction(Action act, bool undo)
{
    switch (act.kind)
    {
        case ACT_LEVEL_TILE_UPDATE:
        {
            Tile *tile_old = GetTile(act.tile_set, act.as.level_action.level_tile->index);
            act.as.level_action.level_tile->index ^= act.as.level_action.level_tile_delta.index;
            act.as.level_action.level_tile->is_solid ^= act.as.level_action.level_tile_delta.is_solid;
            Tile *tile_new = GetTile(act.tile_set, act.as.level_action.level_tile->index);

            tile_old->ref_count -= 1;
            tile_new->ref_count += 1;
        } break;

        case ACT_TILE_UPDATE:
        {
            Tile *tile = GetTile(act.tile_set, act.as.tile_action.tile_index);
            Tile_Packed old_packed = PackTile(tile->color_indexes);
            Tile_Packed new_packed = XorPackedTile(old_packed, act.as.tile_action.tile_delta);
            tile->color_indexes = UnpackTile(new_packed);
            tile->texture_needs_update = true;
        } break;

        case ACT_TILE_ADD_LAST:
        {
            if (undo) goto TileDeleteLast;
TileAdd:
            Tile tile = CreateTile(&APP->tile_atlas);
            tile.color_indexes = UnpackTile(act.as.tile_action.tile_delta);
            da_append(act.tile_set, tile);
        } break;

        case ACT_TILE_DELETE_LAST:
        {
            if (undo) goto TileAdd;
TileDeleteLast:
            Tile *tile = GetTile(act.tile_set, act.tile_set->count - 1);
            FreeTileAtlasIndex(&APP->tile_atlas, tile->tile_atlas_index);
            act.tile_set->count -= 1;
        } break;


        case ACT_TILE_INSERT:
        {
            if (undo) goto TileDelete;
TileInsert:
            Tile tile = {0};
            InsertTile_(act.as.tile_action.tile_index, tile, act.tile_set);
            TODO("Actually get the real tile data");
        } break;

        case ACT_TILE_DELETE:
        {
            if (undo) goto TileInsert;
TileDelete:
            DeleteTile_(act.as.tile_action.tile_index, act.tile_set);
        } break;

        default:
            TODO("HistoryRedo: Unimplemented Action_Kind");
            break;
    }
}

static inline void Undo(Action act) { PerformAction(act, true); }
static inline void Do(Action act) { PerformAction(act, false); }

void HistoryUndo(void)
{
    // Remember: we are going from NEW to OLD, because it is undo.
    // Also remember: Because this is undo, we need to delete a tile on
    // TILE_ADD and add a tile on TILE_DELETE, not confusing at all (^;

    History *history = &APP->history;
    if (history->undo_count == history->count) return;
    assert(history->undo_count < history->count);

    Action act = history->items[history->count - 1 - history->undo_count];

    uint32_t transaction_id = act.transaction_id;

    while (act.transaction_id == transaction_id)
    {
        printf("UNDO: %d\n", act.transaction_id);
        fflush(stdout);

        Undo(act);

        history->undo_count += 1;
        if (history->undo_count == history->count) return;
        act = history->items[history->count - 1 - history->undo_count];
    }
}

void HistoryRedo(void)
{
    // Remember: we are going from OLD to NEW, because it is redo

    History *history = &APP->history;
    assert(history->undo_count <= history->count);
    if (history->undo_count == 0) return;

    Action act = history->items[history->count - history->undo_count];

    uint32_t transaction_id = act.transaction_id;

    while (act.transaction_id == transaction_id)
    {
        printf("REDO: %d\n", act.transaction_id);
        fflush(stdout);

        Do(act);

        history->undo_count -= 1;
        if (history->undo_count == 0) return;
        act = history->items[history->count - history->undo_count];
    }
}

static inline void RecordAndDo(Action act)
{
    Record(act);
    Do(act);
}

void ModeDrawPixelsAutoNewTile(Vector2 mouse_pos_world, Level *level, Tile_Set *tile_set)
{
    World_Position pos = GetWorldPosition(mouse_pos_world);
    Level_Tile *level_tile = LevelGetTile(*level, pos.tile_x, pos.tile_y);
    if (!level_tile) return;

    World_Position pos_prev = GetWorldPosition(GetScreenToWorld2D(APP->mouse_previous, APP->camera_world));
    bool has_entered_new_tile = (pos_prev.tile_x != pos.tile_x || pos_prev.tile_y != pos.tile_y);

    uint32_t edit_tile_index = GetEditTileIndex(tile_set);

    bool mouse_released = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);

    if (APP->hot_tile && (has_entered_new_tile || mouse_released))
    {
        Tile *edit_tile = GetTile(tile_set, edit_tile_index);
        Tile *old_tile = GetTile(tile_set, APP->hot_tile->index);

        assert(edit_tile_index != APP->hot_tile->index);
        assert(edit_tile->ref_count == 0);
        assert(old_tile->ref_count > 0);
        assert(edit_tile_index == tile_set->count - 1);

        // Deduplicate last auto tile
        uint32_t matched_tile_index = FindTileMatch(*tile_set, edit_tile, edit_tile_index);

        if (matched_tile_index != INVALID_TILE_INDEX)
        {
            // We found a duplicate tile.
            Level_Tile delta = *APP->hot_tile;
            delta.index ^= matched_tile_index;
            RecordAndDo(ActLevelTileUpdate(APP->hot_tile, delta, tile_set));

            // Remove temporary edit tile
            Do(ActTileDeleteLast(tile_set, PackTile(edit_tile->color_indexes)));
        }
        else if (old_tile->ref_count == 1)
        {
            // We can reuse the old tile
            Tile_Packed delta = XorPackedTile(PackTile(old_tile->color_indexes), PackTile(edit_tile->color_indexes));
            RecordAndDo(ActTileUpdate(tile_set, APP->hot_tile->index, delta));

            // Remove temporary edit tile
            Do(ActTileDeleteLast(tile_set, PackTile(edit_tile->color_indexes)));
        }
        else
        {
            // We have a unique tile and we can't just overwrite the old tile
            // because it is used elsewhere.
            assert(old_tile->ref_count > 1);

            Record(ActTileAdd(tile_set, PackTile(edit_tile->color_indexes)));

            Level_Tile delta = *APP->hot_tile;
            delta.index ^= edit_tile_index;
            RecordAndDo(ActLevelTileUpdate(APP->hot_tile, delta, tile_set));
        }

        if (mouse_released) EndRecordTransaction();

        APP->hot_tile = NULL;
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
    {
        if (has_entered_new_tile || IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            APP->hot_tile = level_tile;

            // Figure out if we can just modify the existing tile
            // or if we need to AUTOMATICALLY create a NEW TILE.
            Tile *old_tile = GetTile(tile_set, level_tile->index);
            assert(old_tile->ref_count > 0 && "The ref count should be > 0 since we got the tile from the level");

            Tile_Packed tile_data = PackTile(old_tile->color_indexes);

            Do(ActTileAdd(tile_set, tile_data));

            edit_tile_index = GetEditTileIndex(tile_set);
        }

        Tile *tile = GetTile(tile_set, edit_tile_index);

        SetTilePixel(tile, pos.pixel_x, pos.pixel_y, APP->current_color_index);
    }
}

void ModeDrawPixels(Vector2 mouse_pos_world, Level *level, Tile_Set *tile_set)
{
    World_Position pos = GetWorldPosition(mouse_pos_world);
    Level_Tile *level_tile = LevelGetTile(*level, pos.tile_x, pos.tile_y);
    if (!level_tile) return;

    World_Position pos_prev = GetWorldPosition(GetScreenToWorld2D(APP->mouse_previous, APP->camera_world));
    bool has_entered_new_tile = (pos_prev.tile_x != pos.tile_x || pos_prev.tile_y != pos.tile_y);

    bool mouse_released = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);

    if (APP->hot_tile && (has_entered_new_tile || mouse_released))
    {
        Tile *old_tile = GetTile(tile_set, APP->hot_tile->index);
        Tile_Packed delta = XorPackedTile(APP->tile_snapshot, PackTile(old_tile->color_indexes));
        Record(ActTileUpdate(tile_set, APP->hot_tile->index, delta));

        if (mouse_released) EndRecordTransaction();

        APP->hot_tile = NULL;
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
    {
        Tile *tile = GetTile(tile_set, level_tile->index);

        if (has_entered_new_tile || IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            APP->hot_tile = level_tile;
            APP->tile_snapshot = PackTile(tile->color_indexes);
        }

        SetTilePixel(tile, pos.pixel_x, pos.pixel_y, APP->current_color_index);
    }
}

void UpdateWorldView(Vector2 mouse_pos_screen, Vector2 mouse_delta, float mouse_scroll, Level *level, Tile_Set *tile_set)
{
    Vector2 mouse_pos_world = GetScreenToWorld2D(mouse_pos_screen, APP->camera_world);
    if (!IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && !IsKeyDown(KEY_SPACE))
    {
        mouse_delta = (Vector2){0};
    }
    ViewUpdate(&APP->camera_world, mouse_scroll, mouse_pos_screen, ZOOM_MIN, ZOOM_MAX, mouse_delta);

    KeyModifiers modifiers = GetKeyModifiers();

    if (IsKeyPressed(KEY_G) && !modifiers.shift) APP->hide_grid ^= 1;
    if (IsKeyPressed(KEY_G) && modifiers.shift) APP->show_game_boy_screen ^= 1;

    if (IsKeyPressed(KEY_I))  APP->show_tile_indexes = !APP->show_tile_indexes;


    if (APP->mode == MODE_DRAW_PIXELS)
    {
        if (IsKeyPressed(KEY_ONE)) APP->current_color_index = COLOR_GB_DARK;
        if (IsKeyPressed(KEY_TWO)) APP->current_color_index = COLOR_GB_MID_DARK;
        if (IsKeyPressed(KEY_THREE)) APP->current_color_index = COLOR_GB_MID_LIGHT;
        if (IsKeyPressed(KEY_FOUR)) APP->current_color_index = COLOR_GB_LIGHT;

        if (APP->auto_new_tile)
        {
            ModeDrawPixelsAutoNewTile(mouse_pos_world, level, tile_set);
        }
        else
        {
            ModeDrawPixels(mouse_pos_world, level, tile_set);
        }
    }
    else if (APP->mode == MODE_DRAW_TILES)
    {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            World_Position pos = GetWorldPosition(mouse_pos_world);
            Level_Tile *level_tile = LevelGetTile(*level, pos.tile_x, pos.tile_y);
            if (level_tile && (level_tile->index != APP->current_tile_index || level_tile->is_solid != APP->current_is_solid))
            {
                Level_Tile level_tile_delta = *level_tile;
                level_tile_delta.is_solid ^= APP->current_is_solid;
                level_tile_delta.index ^= APP->current_tile_index;
                RecordAndDo(ActLevelTileUpdate(level_tile, level_tile_delta, tile_set));
            }
        }
        else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        {
            EndRecordTransaction();
        }
    }
    else
    {
        TODO("Unimplemented app mode");
    }

}

void UpdateTileTextures(Tile_Set *tile_set)
{
    for (uint32_t i = 0; i < tile_set->count; ++i)
    {
        Tile *tile = GetTile(tile_set, i);
        if (!tile->texture_needs_update) continue;

        tile->texture_needs_update = false;

        Rectangle rec = TileAtlasIndexToRect(tile->tile_atlas_index);

        Color *pixels = ConvertColorIndexesToPixels(tile->color_indexes);
        UpdateTextureRec(APP->tile_atlas.texture, rec, (const void *)pixels);
    }
}

void UpdateSidePanelView(Rectangle view, float scroll_input, Tile_Set tile_set)
{
    bool click = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    bool delete_pressed = IsKeyPressed(KEY_DELETE);

    if (!scroll_input && !click && !delete_pressed) return;

    Tile_Picker_Props p = GetTilePickerProps(view, tile_set);

    KeyModifiers modifiers = GetKeyModifiers();

    if (scroll_input)
    {
        if (modifiers.ctrl)
        {
            float zoom_factor = 0.1f * scroll_input;
            APP->side_panel_zoom += zoom_factor * APP->side_panel_zoom;
            APP->side_panel_zoom = Clamp(APP->side_panel_zoom, ZOOM_MIN_TILE_PICKER, ZOOM_MAX_TILE_PICKER);
        }
        else
        {
            APP->side_panel_scroll_offset += scroll_input * 8 * APP->side_panel_zoom;
        }

        float height_not_in_view = (p.num_rows + 1) * (p.tile_size + p.gap_min) - view.height;
        APP->side_panel_scroll_offset = Clamp(APP->side_panel_scroll_offset, -height_not_in_view, 0);
    }

    if (click)
    {
        int32_t tile_index = SidePanelGetHoveredTileIndex(view, GetMousePosition(), p);
        if (tile_index >= 0)
        {
            APP->current_tile_index = tile_index;
        }
    }

    if (delete_pressed)
    {
        if (modifiers.shift)
        {
            // DeleteTile_(APP->current_tile_index, &APP->world.tile_set, &APP->world);
            // if (APP->current_tile_index > APP->world.tile_set.count - 1)
            // {
            //     APP->current_tile_index = APP->world.tile_set.count - 1;
            // }

            TODO("DeleteTile history");
        }
    }
}

void DrawBrushPreview(Rectangle world_view, Tile_Set tile_set)
{
    UNUSED(tile_set);
    float size = 100.0f;
    Rectangle rect =
    {
        world_view.x + 10,
        world_view.y + world_view.height - size - 10,
        size,
        size,
    };

    const char *legend = NULL;
    if (APP->mode == MODE_DRAW_TILES)
    {
        legend = "TILE";
        if (APP->current_is_solid) legend = "TILE (SOLID)";

        // Texture texture = GetTexture(tile_set.items[APP->current_tile_index].texture_index);
        // DrawTexturePro(texture, (Rectangle){0,0,8,8}, rect, (Vector2){0}, 0, WHITE);
    }
    else if (APP->mode == MODE_DRAW_PIXELS)
    {
        legend = "PIXEL";
        if (APP->auto_new_tile) legend = "PIXEL (AUTO TILE)";
        DrawRectangleRec(rect, palette_gbp[APP->current_color_index]);
    }

    DrawRectangleLinesEx(rect, 3, BLACK);

    Font font = GetFontDefault();
    float font_size = 20.0f;
    float spacing = 2.0f;
    Vector2 text_dim = MeasureTextEx(font, legend, font_size, spacing);

    Rectangle legend_rect = {rect.x + rect.width, rect.y + rect.height, text_dim.x + 10.0f, text_dim.y + 10.0f};
    legend_rect.y -= legend_rect.height;
    DrawRectangleRec(legend_rect, (Color){0,0,0,255});
    Vector2 pos = {legend_rect.x + 5.0f, legend_rect.y + 5.0f};
    DrawTextEx(font, legend, pos, font_size, spacing, WHITE);
}

void SerializeU32(Bytes *b, uint32_t v)
{
    da_append(b, (uint8_t)(v >>  0));
    da_append(b, (uint8_t)(v >>  8));
    da_append(b, (uint8_t)(v >> 16));
    da_append(b, (uint8_t)(v >> 24));
}

void SerializeU16(Bytes *b, uint16_t v)
{
    da_append(b, (uint8_t)(v >> 0));
    da_append(b, (uint8_t)(v >> 8));
}

#define SerializeChunkId(b, s) do \
{ \
    da_append(b, (s)[0]); \
    da_append(b, (s)[1]); \
    da_append(b, (s)[2]); \
    da_append(b, (s)[3]); \
} while(0)

void SerializeTile(Bytes *b, Tile *tile)
{
    Tile_Packed tile_gb = PackTileGB(tile->color_indexes);

    for (uint32_t i = 0; i < 8; ++i)
    {
        da_append(b, tile_gb.lines[i].bit_planes[0]);
        da_append(b, tile_gb.lines[i].bit_planes[1]);
    }
}

void SerializeTileSet(Bytes *b, Tile_Set tile_set)
{
    SerializeChunkId(b, "TLST");

    uint32_t chunk_len_loc = b->count;
    SerializeU32(b, 0); // Replace later with actual chunk length

    assert(tile_set.count <= 65535);
    SerializeU16(b, (uint16_t)tile_set.count);

    for (uint32_t i = 0; i < tile_set.count; ++i)
    {
        SerializeTile(b, &tile_set.items[i]);
    }

    // Backfill chunk length field
    *(uint32_t *)&b->items[chunk_len_loc] = b->count - chunk_len_loc;
}

void SerializeLevel(Bytes *b, Level level, uint16_t tile_set_index)
{
    SerializeChunkId(b, "LEVL");

    uint32_t chunk_len_loc = b->count;
    SerializeU32(b, 0); // Replace later with actual chunk length

    assert(level.width <= 65536);
    SerializeU16(b, (uint16_t)level.width);

    assert(level.height <= 65536);
    SerializeU16(b, (uint16_t)level.height);

    SerializeU16(b, tile_set_index);

    // da_reserve(b, b->count + level.width*level.height * sizeof(uint16_t));

    for (uint32_t i = 0; i < level.width*level.height; ++i)
    {
        assert(level.tiles[i].index < 65536);
        SerializeU16(b, (uint16_t)level.tiles[i].index);
    }

    assert(level.width*level.height % 8 == 0);

    // da_reserve(b, b->count + level.width*level.height);
    for (uint32_t i = 0; i < level.width*level.height; i += 8)
    {
        uint8_t solid_bits =
            !!level.tiles[i + 0].is_solid << 7 |
            !!level.tiles[i + 1].is_solid << 6 |
            !!level.tiles[i + 2].is_solid << 5 |
            !!level.tiles[i + 3].is_solid << 4 |
            !!level.tiles[i + 4].is_solid << 3 |
            !!level.tiles[i + 5].is_solid << 2 |
            !!level.tiles[i + 6].is_solid << 1 |
            !!level.tiles[i + 7].is_solid << 0;

        da_append(b, solid_bits);
    }

    // Backfill chunk length field
    *(uint32_t *)&b->items[chunk_len_loc] = b->count - chunk_len_loc;
}

void SerializeWorld(Bytes *b, Level level, Tile_Set tile_set)
{
    // File format header
    SerializeChunkId(b, "\xffWLD");
    SerializeU16(b, APP->save_file_format_version);

    // Write tile sets
    // TODO support more than one tile set
    SerializeTileSet(b, tile_set);

    // Write levels
    // TODO support more than one level
    SerializeLevel(b, level, 0);
}

void SetOpenFilePath(const char *currently_open_world_file_path)
{
    APP->currently_open_world_file_path = currently_open_world_file_path;
    size_t tmp = temp_save();
    SetWindowTitle(temp_sprintf("Game Boy World Editor (%.100s)", currently_open_world_file_path));
    temp_rewind(tmp);
}

static const char *world_file_pattern = "*.wld";

void SaveWorld(bool save_as, Level level, Tile_Set tile_set)
{
    const char *save_file_path = APP->currently_open_world_file_path;

    if (save_as || !APP->currently_open_world_file_path)
    {
        save_file_path = tinyfd_saveFileDialog("Save World", NULL, 1, &world_file_pattern, NULL);
        if (save_file_path == NULL || save_file_path[0] == '\0') return;
    }

    FILE *file = fopen(save_file_path, "wb");
    if (!file)
    {
        size_t tmp = temp_save();
        char *message = temp_sprintf("Could not open %.100s for writing", save_file_path);
        tinyfd_messageBox("Could not open file for saving", message, "ok", "warning", 1);
        temp_rewind(tmp);
        return;
    }

    SetOpenFilePath(save_file_path);

    APP->serialization_buffer.count = 0;
    SerializeWorld(&APP->serialization_buffer, level, tile_set);

    // Write the buffer to the file
    size_t written = fwrite(APP->serialization_buffer.items, sizeof(uint8_t), APP->serialization_buffer.count, file);
    fclose(file);

    if (written != APP->serialization_buffer.count)
    {
        tinyfd_messageBox("Could not write to file", "Could not write to file", "ok", "warning", 1);
        perror("Error writing to file");
        APP->currently_open_world_file_path = NULL;
    }
    else
    {
        nob_log(INFO, "Saved world.");
    }

    APP->serialization_buffer.count = 0;
}

bool BytesReadFile(Bytes *b, FILE *f)
{
    if (fseek(f, 0, SEEK_END) < 0) return false;

#ifndef _WIN32
    long file_size = ftell(f);
#else
    long long file_size = _ftelli64(f);
#endif

    if (file_size < 0) return false;
    if (fseek(f, 0, SEEK_SET) < 0) return false;

    uint32_t new_count = b->count + (uint32_t)file_size;
    da_reserve(b, new_count);

    fread(b->items + b->count, file_size, 1, f);
    if (ferror(f)) return false;

    b->count = new_count;

    return true;
}

#define ChunkIdMake(A, B, C, D) (((uint32_t)(uint8_t)(A) << 0) | ((uint32_t)(uint8_t)(B) << 8) | ((uint32_t)(uint8_t)(C) << 16) | ((uint32_t)(uint8_t)(D) << 24))
#define ChunkIdMakeFromBytes(b) (((uint32_t)(uint8_t)(b)[0] << 0) | ((uint32_t)(uint8_t)(b)[1] << 8) | ((uint32_t)(uint8_t)(b)[2] << 16) | ((uint32_t)(uint8_t)(b)[3] << 24))

static inline bool DeserializeChunkId(uint8_t **at, uint8_t *end, uint32_t *chunk_id)
{
    if (*at >= end - sizeof(*chunk_id)) return false;
    *chunk_id = ChunkIdMakeFromBytes(*at);
    *at += sizeof(*chunk_id);

    return true;
}

static inline bool DeserializeExpectChunkId(uint8_t **at, uint8_t *end, uint32_t expected_chunk_id)
{
    uint32_t chunk_id;
    if (!DeserializeChunkId(at, end, &chunk_id)) return false;
    return chunk_id == expected_chunk_id;
}

bool DeserializeU8(uint8_t **at, uint8_t *end, uint8_t *v)
{
    if (*at + sizeof(*v) >= end) return false;
    *v = *(uint8_t *)(*at);
    *at += sizeof(*v);
    return true;
}

bool DeserializeU16(uint8_t **at, uint8_t *end, uint16_t *v)
{
    if (*at + sizeof(*v) >= end) return false;
    *v = *(uint16_t *)(*at);
    *at += sizeof(*v);
    return true;
}

bool DeserializeU32(uint8_t **at, uint8_t *end, uint32_t *v)
{
    if (*at + sizeof(*v) >= end) return false;
    *v = *(uint32_t *)(*at);
    *at += sizeof(*v);
    return true;
}

bool DeserializeLevel(uint8_t **at, uint8_t *end, Level *level)
{
    uint16_t width = 0;
    if (!DeserializeU16(at, end, &width)) return false;
    assert(width < 256 && width > 0);

    uint16_t height = 0;
    if (!DeserializeU16(at, end, &height)) return false;
    assert(height < 256 && height > 0);

    assert(width * height % 8 == 0);

    uint16_t tile_set_index = 0;
    if (!DeserializeU16(at, end, &tile_set_index)) return false;
    assert(tile_set_index < 256);

    void *new_tiles = realloc(level->tiles, width * height * sizeof(level->tiles[0]));
    assert(new_tiles);

    level->tiles = new_tiles;
    level->width = width;
    level->height = height;
    level->tile_set_index = tile_set_index;

    // Deserialize tiles
    for (int32_t i = 0; i < width*height; ++i)
    {
        uint16_t tile_index = 0;
        if (!DeserializeU16(at, end, &tile_index)) return false;

        level->tiles[i].index = tile_index;
    }

    // Deserialize solid bits
    if (*at + width*height/8 > end) return false;
    for (int32_t i = 0; i < width*height; i += 8, *at += 1)
    {
        uint8_t solid_bits = **at;
        level->tiles[i + 0].is_solid = !!(solid_bits & (1 << 7));
        level->tiles[i + 1].is_solid = !!(solid_bits & (1 << 6));
        level->tiles[i + 2].is_solid = !!(solid_bits & (1 << 5));
        level->tiles[i + 3].is_solid = !!(solid_bits & (1 << 4));
        level->tiles[i + 4].is_solid = !!(solid_bits & (1 << 3));
        level->tiles[i + 5].is_solid = !!(solid_bits & (1 << 2));
        level->tiles[i + 6].is_solid = !!(solid_bits & (1 << 1));
        level->tiles[i + 7].is_solid = !!(solid_bits & (1 << 0));
    }

    return true;
}

bool DeserializeTileSet(uint8_t **at, uint8_t *end, Tile_Set *tile_set)
{
    uint16_t tile_count;
    if (!DeserializeU16(at, end, &tile_count)) return false;
    assert(tile_count < 1024);

    da_reserve(tile_set, tile_count);
    tile_set->count = tile_count;

    if (*at >= end - tile_count * sizeof(Tile_Packed)) return false;

    for (uint16_t i = 0; i < tile_count; ++i, *at += sizeof(Tile_Packed))
    {
        Tile_Packed tile_gb = *(Tile_Packed *)(*at);
        Tile tile = {0};
        tile.color_indexes = UnpackTileGB(tile_gb);
        tile.tile_atlas_index = GetNewTileAtlasIndex(&APP->tile_atlas);
        tile.texture_needs_update = true;
        tile_set->items[i] = tile;
    }

    return true;
}

bool DeserializeWorld(World *world, Bytes *b)
{
    uint8_t *at = b->items;
    uint8_t *end = b->items + b->count;

    uint32_t expected_chunk_id = ChunkIdMake('\xff','W','L','D');

    if (!DeserializeExpectChunkId(&at, end, expected_chunk_id)) return false;

    uint16_t version;
    if (!DeserializeU16(&at, end, &version)) return false;

    if (version != APP->save_file_format_version) return false;

    while (at < end)
    {
        uint32_t chunk_id = 0;
        if (!DeserializeChunkId(&at, end, &chunk_id)) return false;

        uint32_t chunk_size = 0;
        if (!DeserializeU32(&at, end, &chunk_size)) return false;

        switch (chunk_id)
        {
            case ChunkIdMake('L','E','V','L'): // Level
            {
                if (!DeserializeLevel(&at, end, &world->level)) return false;
            } break;

            case ChunkIdMake('T','L','S','T'): // Tile Set
            {
                if (!DeserializeTileSet(&at, end, &world->tile_set)) return false;
            } break;

            default:
                fprintf(stderr, "Unknown chunk while reading world file, %x\n", chunk_id);
                assert(at + chunk_size <= end);
                at += chunk_size;
        }
    }

    return true;
}

void FreeWorld(World *world)
{
    world->tile_set.count = 0;
    ResetTileAtlas(&APP->tile_atlas);
}

bool LoadWorldByPath(World *world, char *load_file_path)
{
    if (load_file_path == NULL || !load_file_path[0]) return false;
    if (file_exists(load_file_path) < 1) return false;

    FILE *file = fopen(load_file_path, "rb");
    if (!file)
    {
        goto error;
    }

    APP->serialization_buffer.count = 0;
    if (!BytesReadFile(&APP->serialization_buffer, file))
    {
        goto error;
    }

    FreeWorld(world); // 313 ....FreeWorld

    if (!DeserializeWorld(world, &APP->serialization_buffer))
    {
        goto error;
    }

    // Ref count new tiles
    for (uint32_t i = 0; i < world->level.width * world->level.height; ++i)
    {
        uint32_t tile_index = world->level.tiles[i].index;
        assert(tile_index < world->tile_set.count);

        world->tile_set.items[tile_index].ref_count += 1;
    }

    fclose(file);
    SetOpenFilePath(load_file_path);
    return true;

error:
    if (file) fclose(file);
    size_t tmp = temp_save();
    char *message = temp_sprintf("Could not open world file, %.100s", load_file_path);
    tinyfd_messageBox("Could not open world file", message, "ok", "warning", 1);
    temp_rewind(tmp);
    APP->serialization_buffer.count = 0;

    return false;
}

bool LoadWorld(World *world)
{
    char *load_file_path = tinyfd_openFileDialog("Load World", NULL, 1, &world_file_pattern, NULL, 0);
    return LoadWorldByPath(world, load_file_path);
}

void GlobalShortcuts(void)
{
    KeyModifiers modifiers = GetKeyModifiers();

    if (IsKeyPressed(KEY_F11) || (modifiers.alt && IsKeyPressed(KEY_ENTER)))
    {
        ToggleFullscreen();
    }


    if (IsKeyPressed(KEY_TAB))
    {
        if (modifiers.shift)
        {
            APP->current_is_solid ^= APP->mode == MODE_DRAW_TILES;
            APP->auto_new_tile ^= APP->mode == MODE_DRAW_PIXELS;
        }
        else
        {
            APP->mode ^= MODE_DRAW_TILES ^ MODE_DRAW_PIXELS;
        }
    }

    if (IsKeyPressed(KEY_F1) || (modifiers.shift && IsKeyPressed(KEY_SLASH)))
    {
        const char *shortcut_legends =
        "Tab:\t\tSwitch mode\n"
        "Shift+Tab:\tSwitch sub-mode\n"
        "G:\t\tShow/hide grid\n"
        "Shift+G:\t\tShow/hide Game Boy display guide\n"
        "I:\t\tShow/hide tile indexes\n"
        "Ctrl+Z:\t\tUndo\n"
        "Ctrl+Shift+Z:\tRedo\n"
        "Ctrl+Y:\t\tRedo\n"
        "Ctrl+O:\t\tOpen\n"
        "Ctrl+S:\t\tSave\n"
        "Ctrl+Shift+S:\tSave as\n"
        ;

        tinyfd_messageBox("Shortcut Cheat Sheet", shortcut_legends, "ok", "info", 1);
    }

    if (modifiers.ctrl && IsKeyPressed(KEY_S)) SaveWorld(modifiers.shift, APP->world.level, APP->world.tile_set);
    if (modifiers.ctrl && IsKeyPressed(KEY_O)) LoadWorld(&APP->world);

    if (modifiers.ctrl && !modifiers.shift && IsKeyPressed(KEY_Z)) HistoryUndo();
    if ((modifiers.ctrl && IsKeyPressed(KEY_Y)) || (modifiers.shift && IsKeyPressed(KEY_Z))) HistoryRedo();

    if (modifiers.ctrl)
    {
        int change = !!IsKeyPressed(KEY_UP) - !!IsKeyPressed(KEY_DOWN);
        if (change)
        {
            APP->current_tile_index = (APP->current_tile_index + change + APP->world.tile_set.count) % (APP->world.tile_set.count);
        }
    }
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    InitWindow(1024, 768, "Game Boy World Editor");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(120);

    InitApp();

    while (!WindowShouldClose())
    {
        // Calculate sub views
        Rectangle total_view = {0.0f, 0.0f, (float)GetRenderWidth(), (float)GetRenderHeight()};
        Rectangle content_view = total_view;
        float side_panel_width = content_view.width - 400.0f;
        Rectangle world_view = PadRect(CutRectGetLeft(content_view, side_panel_width), 10);
        Rectangle side_panel_view = PadRectEx(CutRectGetRight(content_view, side_panel_width), 10, 10, 10, 0);

        Vector2 mouse_pos_screen = GetMousePosition();
        Vector2 mouse_delta = Vector2Subtract(mouse_pos_screen, APP->mouse_previous);
        float mouse_scroll = GetMouseWheelMoveV().y;

        GlobalShortcuts();

        if (CheckCollisionPointRec(mouse_pos_screen, world_view))
            UpdateWorldView(mouse_pos_screen, mouse_delta, mouse_scroll, &APP->world.level, &APP->world.tile_set);

        if (CheckCollisionPointRec(mouse_pos_screen, side_panel_view))
            UpdateSidePanelView(side_panel_view, mouse_scroll, APP->world.tile_set);

        APP->mouse_previous = mouse_pos_screen;

        UpdateTileTextures(&APP->world.tile_set);

        ///////////////////////////
        //                       //
        //        DRAWING        //
        //                       //
        ///////////////////////////

        BeginDrawing();
        ClearBackground(COLOR_WINDOW_BACKGROUND);

#if 1
        DrawWorldView(world_view, APP->world, mouse_pos_screen);
        DrawRectangleLinesEx(world_view, 3, COLOR_PANEL_BORDER);

        DrawSidePanel(side_panel_view, APP->world.tile_set);
        DrawRectangleLinesEx(side_panel_view, 3, COLOR_PANEL_BORDER);

        DrawBrushPreview(world_view, APP->world.tile_set);
#endif
        EndDrawing();

    }
    return 0;
}
