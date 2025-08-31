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

typedef struct Tile_Line_GB
{
    uint8_t bit_planes[2];
} Tile_Line_GB;

typedef union Tile_GB
{
    // NOTE(jkk): 128 bits
    //
    // Tiles in the game boy are stored line by line, using 2 bytes per line.
    // For each line, the first byte specifies the least significant bit of the
    // color ID of each pixel, and the second byte specifies the most
    // significant bit. In both bytes, bit 7 represents the leftmost pixel, and
    // bit 0 the rightmost.
    Tile_Line_GB lines[8];
    uint64_t u64s[2];
} Tile_GB;

typedef struct Tile
{
    uint8_t color_indexes[8*8]; // NOTE(jkk): In range 0-3
    Texture2D texture;
    uint32_t ref_count;
} Tile;

typedef struct Tiles
{
    // Dynamic array of tiles
    Tile *items;
    uint32_t count;
    uint32_t capacity;
} Tiles;

typedef struct TilePtrs
{
    // Dynamic array of indexes
    Tile **items;
    size_t count;
    size_t capacity;
} TilePtrs;

typedef struct World_Tile
{
    uint32_t index;
    bool is_solid;
} World_Tile;

typedef struct Level
{
    World_Tile *tiles;
    size_t width;
    size_t height;
} Level;

typedef enum Editor_Mode
{
    MODE_DRAW_TILES,
    MODE_DRAW_PIXELS,
} Editor_Mode;

typedef struct App
{
    // View stuff
    Camera2D camera_world;

    float side_panel_width;
    float side_panel_width_min;
    float side_panel_zoom;
    float side_panel_scroll_offset;

    Vector2 mouse_previous;
    uint8_t current_color_idx; // NOTE(jkk): In range 0-3
    uint32_t current_tile_idx;
    bool current_is_solid;

    // Mode stuff
    Editor_Mode mode;
    bool draw_solid_mask;
    bool hide_grid;
    bool show_tile_indexes;
    bool auto_new_tile;

    bool is_auto_tiling;

    Level level;
    Tiles tile_set;
    Tile edit_tile; // The tile currently being drawn

    Tile **sorted_tiles;
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

Texture CreateTileTexture(Color pixels[8*8])
{
    Texture texture = LoadTextureFromImage((Image){
        .data = (void *)pixels,
        .width = 8,
        .height = 8,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
    });
    UpdateTexture(texture, (void *)pixels);
    return texture;
}

static Tile CreateCloneTile(Tile *src_tile)
{
    Tile tile = {0};
    Color pixels[8*8];

    for (int i = 0; i < 8*8; ++i)
    {
        uint8_t color_index = src_tile->color_indexes[i];
        if (!(color_index < 4))
        {
            fprintf(stderr, "color_index < 4: i = %d\n", i);
            src_tile = src_tile;
        }
        assert(color_index < 4);
        tile.color_indexes[i] = color_index;
        pixels[i] = palette_gbp[color_index];
    }

    tile.texture = CreateTileTexture(pixels);

    return tile;
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

    tile.texture = CreateTileTexture(pixels);

    return tile;
}

Tile_GB TileToGB(Tile *tile)
{
    Tile_GB tile_gb = {0};

    for (int y = 0; y < 8; ++y)
    {
        for (int x = 0; x < 8; ++x)
        {
            uint8_t pixel = tile->color_indexes[8*y + x];
            if (pixel & 1) tile_gb.lines[y].bit_planes[0] |= (0x80 >> x);
            if (pixel & 2) tile_gb.lines[y].bit_planes[1] |= (0x80 >> x);
        }
    }

    return tile_gb;
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

Tile *GetTile(uint32_t index)
{
    assert(index < APP->tile_set.count);
    return &APP->tile_set.items[index];
}

void SetTilePixel(Tile *tile, uint8_t pixel_x, uint8_t pixel_y, uint8_t color_index)
{
    assert(pixel_x >= 0 && pixel_x < 8 && pixel_y >= 0 && pixel_y < 8);
    assert(color_index < 4);

    tile->color_indexes[pixel_y * 8 + pixel_x] = color_index;
    Color pixel = palette_gbp[color_index];
    UpdateTextureRec(tile->texture, (Rectangle){(float)pixel_x, (float)pixel_y, 1.0f, 1.0f}, &pixel);
}

void InitApp(void)
{
    APP = malloc(sizeof(*APP));
    memset(APP, 0xcd, sizeof(*APP));

    APP->camera_world.offset = (Vector2){0};
    APP->camera_world.target = (Vector2){0};
    APP->camera_world.rotation = 0.0f;
    APP->camera_world.zoom = 5.0f;
    APP->mode = MODE_DRAW_PIXELS;

    APP->side_panel_width_min = 300.0f;
    APP->side_panel_width = 200.0f;
    APP->side_panel_zoom = 4.0f;
    APP->side_panel_scroll_offset = 0.0f;

    APP->mouse_previous = (Vector2){0.0f};
    APP->current_color_idx = 0; // NOTE(jkk): In range 0-3
    APP->current_tile_idx = 0;
    APP->current_is_solid = false;

    APP->mode = MODE_DRAW_PIXELS;
    APP->draw_solid_mask = false;
    APP->hide_grid = false;
    APP->show_tile_indexes = false;
    APP->auto_new_tile = true;
    APP->is_auto_tiling = true;

    APP->level.width = 128;
    APP->level.height = 64;
    size_t num_tiles = APP->level.width * APP->level.height;
    APP->level.tiles = malloc(num_tiles*sizeof(*APP->level.tiles));
    memset(APP->level.tiles, 0xcd, num_tiles*sizeof(*APP->level.tiles));

    APP->tile_set.capacity = 0;
    APP->tile_set.count = 0;
    APP->tile_set.items = NULL;

    for (size_t i = 0; i < num_tiles; ++i)
    {
        APP->level.tiles[i].index = 0;
        APP->level.tiles[i].is_solid = 0;
    }

    Tile tile = CreateTile(COLOR_GB_LIGHT);
    tile.ref_count = (uint32_t)num_tiles;
    da_append(&APP->tile_set, tile);

    APP->edit_tile = CreateTile(COLOR_GB_MID_DARK);
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

void DrawTileGrid(void)
{
    Vector2 grid_start = GetWorldToScreen2D((Vector2){0,0}, APP->camera_world);
    Vector2 grid_end = GetWorldToScreen2D((Vector2){APP->level.width * 8.0f, APP->level.height * 8.0f}, APP->camera_world);

    bool show_pixels = APP->camera_world.zoom > ZOOM_SHOW_PIXELS;

    for (int y = 0; y < APP->level.height * 8; ++y)
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

    for (int x = 0; x < APP->level.width * 8; ++x)
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

void DrawTileIndexes(void)
{
    Vector2 grid_start = GetWorldToScreen2D((Vector2){0,0}, APP->camera_world);

    Font font = GetFontDefault();
    for (int y = 0; y < APP->level.height; ++y)
    {
        float yf = grid_start.y + y * 8.0f * APP->camera_world.zoom;

        for (int x = 0; x < APP->level.width; ++x)
        {
            float xf = grid_start.x + x * 8.0f * APP->camera_world.zoom;
            int tile_index = APP->level.tiles[y * APP->level.width + x].index;
            Vector2 pos = { xf + APP->camera_world.zoom * 0.2f, yf + APP->camera_world.zoom * 0.2f};
            DrawTextEx(font, TextFormat("%d", tile_index), pos, 48, 1.0f, BLACK);
        }
    }
}

void DrawWorldView(Rectangle view, Vector2 mouse_pos_screen)
{
    Vector2 mouse_pos_world = GetScreenToWorld2D(mouse_pos_screen, APP->camera_world);
    BeginScissorMode((int)view.x, (int)view.y, (int)view.width, (int)view.height);
    ClearBackground(COLOR_WORLD_BACKGROUND);
    // DrawRectangleGradientV((int)view.x, (int)view.y, (int)view.width, (int)view.height, (Color){52, 61, 89, 255}, (Color){18, 22, 42, 255});

    // Draw tiles
    BeginMode2D(APP->camera_world);
    {

        for (int y = 0; y < APP->level.height; ++y)
        {
            for (int x = 0; x < APP->level.width; ++x)
            {
                World_Tile world_tile = APP->level.tiles[y * APP->level.width + x];
                Texture2D texture = APP->tile_set.items[world_tile.index].texture;
                DrawTexture(texture, x*8, y*8, WHITE);
                if (world_tile.is_solid) DrawRectangleRec((Rectangle){(float)x*8,(float)y*8,8,8}, COLOR_SOLID_BRUSH);
            }
        }

        if (APP->mode == MODE_DRAW_PIXELS && APP->auto_new_tile && IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            World_Position edit_pos = GetWorldPosition(mouse_pos_world);
            DrawTexture(APP->edit_tile.texture, edit_pos.tile_x*8, edit_pos.tile_y*8, WHITE);
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

            Color draw_color = palette_gbp[APP->current_color_idx];
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

        if (APP->draw_solid_mask)
        {
            if (APP->current_is_solid) DrawRectangleRec(tile_rect, COLOR_SOLID_BRUSH);
            DrawRectangleLinesEx(tile_rect, 3, BLACK);
        }
    }

    if (!APP->hide_grid && APP->camera_world.zoom > ZOOM_SHOW_TILES)
    {
        DrawTileGrid();
    }

    if (APP->show_tile_indexes && APP->camera_world.zoom > ZOOM_SHOW_TILE_INDEXES)
    {
        DrawTileIndexes();
    }

    EndScissorMode();
}

static inline bool TileEqualsGB(Tile_GB a, Tile_GB b)
{
    return a.u64s[0] == b.u64s[0] && a.u64s[1] == b.u64s[1];
}

static int CompareTiles(const Tile **pa, const Tile **pb)
{
    int sum_a = 0;
    int sum_b = 0;

    const uint8_t *colors_a = (*pa)->color_indexes;
    const uint8_t *colors_b = (*pb)->color_indexes;

    for (int y = 0; y < 8; ++y)
    {
        for (int x = 0; x < 8; ++x)
        {
            sum_a += colors_a[x + y*8];
            sum_b += colors_b[x + y*8];
        }
    }

    return sum_a - sum_b;
}

static int CompareTilesVoidPtr(const void *a, const void *b)
{
    return CompareTiles((const Tile **)a, (const Tile **)b);
}

void UpdateSortedTiles(void)
{
    if (APP->tile_set.count == 0) return;
    APP->sorted_tiles = NOB_REALLOC(APP->sorted_tiles, APP->tile_set.count * sizeof(*APP->sorted_tiles));
    NOB_ASSERT(APP->sorted_tiles != NULL && "Buy more RAM lol");

    for (size_t i = 0; i < APP->tile_set.count; ++i)
    {
        APP->sorted_tiles[i] = &APP->tile_set.items[i];
    }

    qsort(APP->sorted_tiles, APP->tile_set.count, sizeof(*APP->sorted_tiles), CompareTilesVoidPtr);
}

typedef struct
{
    float tile_size;
    float gap;
    float gap_min;
    uint32_t tiles_per_row;
    uint32_t num_rows;
} Tile_Picker_Props;

Tile_Picker_Props GetTilePickerProps(Rectangle view)
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
    p.num_rows = (APP->tile_set.count + tiles_per_row - 1) / tiles_per_row;
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

void DrawSidePanel(Rectangle view)
{
    BeginScissorMode((int)view.x, (int)view.y, (int)view.width, (int)view.height);

    ClearBackground(COLOR_TILESET_BACKGROUND);

    Tile_Picker_Props props = GetTilePickerProps(view);

    // Draw tile set
#if SORT_TILES
    UpdateSortedTiles();
#endif

    Vector2 mouse = GetMousePosition();
    int32_t hovered_tile_index = SidePanelGetHoveredTileIndex(view, mouse, props);

    for (int32_t i = 0; i < (int32_t)APP->tile_set.count; ++i)
    {
#if SORT_TILES
        Tile *tile = &APP->sorted_tiles[i];
#else
        Tile *tile = &APP->tile_set.items[i];
#endif
        Texture texture = tile->texture;

        Rectangle tile_rect = GetSidePanelTileRect(view, i, props);
        Color tint = tile->ref_count ? WHITE : (Color){255, 100, 100, 255};
        if (i == hovered_tile_index) tint = BLUE;
        DrawTexturePro(texture, (Rectangle){0,0,8,8}, tile_rect, (Vector2){0}, 0, tint);
    }
    // DrawText(TextFormat("%0.2f", tile_size), (int)(view.x + 20), (int)(view.y + 20), 50, RED);

    EndScissorMode();
}

void CopyTile(Tile *dst, Tile *src)
{
    Color pixels[8*8];
    for (int i = 0; i < 8*8; ++i)
    {
        uint8_t color_index = src->color_indexes[i];
        if (color_index >= 4)
        {
            dst = dst;
            assert(0 && "Color index is not in range 0-3");
        }
        dst->color_indexes[i] = color_index;
        pixels[i] = palette_gbp[color_index];
    }
    UpdateTexture(dst->texture, pixels);
}

#define INVALID_TILE_INDEX ((uint32_t)-1)

uint32_t FindTileMatch(Tile *src_tile)
{
    Tile_GB src_tile_gb = TileToGB(src_tile);

    for (uint32_t i = 0; i < APP->tile_set.count; ++i)
    {
        Tile *tile = &APP->tile_set.items[i];
        Tile_GB tile_gb = TileToGB(tile);
        if (TileEqualsGB(src_tile_gb, tile_gb))
        {
            return i;
        }
    }
    return INVALID_TILE_INDEX;
}

void UpdateWorldTileFromEditTile(World_Tile *world_tile)
{
    assert(world_tile >= APP->level.tiles && world_tile < &APP->level.tiles[APP->level.width*APP->level.height]);
    assert(world_tile->index < APP->tile_set.count);

    Tile *old_tile = GetTile(world_tile->index);
    assert(old_tile->ref_count >= 1);

    uint32_t index = FindTileMatch(&APP->edit_tile);

    // No existing tile matches the edit tile
    if (index == INVALID_TILE_INDEX)
    {
        // The old tile was only used here, just overwrite it and reuse the index
        if (old_tile->ref_count == 1)
        {
            CopyTile(old_tile, &APP->edit_tile);
            return;
        }

        index = (uint32_t)APP->tile_set.count;
        Tile tile = CreateCloneTile(&APP->edit_tile);

        da_append(&APP->tile_set, tile);
    }

    assert(index < APP->tile_set.count);

    world_tile->index = index;
    Tile *tile = GetTile(index);
    assert(tile->color_indexes[0] < 4);
    tile->ref_count += 1; // Update reference count of new tile
    assert(tile->ref_count < APP->level.width*APP->level.height);

    old_tile->ref_count -= 1;
    assert(old_tile->ref_count < APP->level.width*APP->level.height);
}

void UpdateWorldView(Vector2 mouse_pos_screen, Vector2 mouse_delta, float mouse_scroll)
{
    Vector2 mouse_pos_world = GetScreenToWorld2D(mouse_pos_screen, APP->camera_world);
    if (!IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && !IsKeyDown(KEY_SPACE))
    {
        mouse_delta = (Vector2){0};
    }
    ViewUpdate(&APP->camera_world, mouse_scroll, mouse_pos_screen, ZOOM_MIN, ZOOM_MAX, mouse_delta);

    if (IsKeyPressed(KEY_G))  APP->hide_grid = !APP->hide_grid;
    if (IsKeyPressed(KEY_I))  APP->show_tile_indexes = !APP->show_tile_indexes;


    if (APP->mode == MODE_DRAW_PIXELS)
    {
        if (IsKeyPressed(KEY_ONE)) APP->current_color_idx = COLOR_GB_DARK;
        if (IsKeyPressed(KEY_TWO)) APP->current_color_idx = COLOR_GB_MID_DARK;
        if (IsKeyPressed(KEY_THREE)) APP->current_color_idx = COLOR_GB_MID_LIGHT;
        if (IsKeyPressed(KEY_FOUR)) APP->current_color_idx = COLOR_GB_LIGHT;

        World_Position pos = GetWorldPosition(mouse_pos_world);
        World_Tile *world_tile = GetWorldTile(pos.tile_x, pos.tile_y);

        if (world_tile)
        {
            Tile *tile = GetTile(world_tile->index);

            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
            {
                if (APP->auto_new_tile)
                {
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                    {
                        CopyTile(&APP->edit_tile, tile);
                    }

                    World_Position pos_prev = GetWorldPosition(GetScreenToWorld2D(APP->mouse_previous, APP->camera_world));
                    World_Tile *world_tile_prev = GetWorldTile(pos_prev.tile_x, pos_prev.tile_y);
                    bool has_left_previous_tile = world_tile_prev && (pos_prev.tile_x != pos.tile_x || pos_prev.tile_y != pos.tile_y);

                    if (has_left_previous_tile)
                    {
                        UpdateWorldTileFromEditTile(world_tile_prev);

                        // TODO make functions deal with only tile indicies
                        // and NOT tile pointers, the tile pointers are not
                        // stable!
                        tile = GetTile(world_tile->index);

                        CopyTile(&APP->edit_tile, tile);
                    }

                    tile = &APP->edit_tile;
                }
                SetTilePixel(tile, pos.pixel_x, pos.pixel_y, APP->current_color_idx);
            }
            else if (APP->auto_new_tile && IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
            {
                UpdateWorldTileFromEditTile(world_tile);
            }
        }
    }
    else if (APP->mode == MODE_DRAW_TILES)
    {
        if (APP->draw_solid_mask)
        {
            if (IsKeyPressed(KEY_ONE)) APP->current_is_solid = false;
            if (IsKeyPressed(KEY_TWO)) APP->current_is_solid = true;
        }

        if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))
        {
            int change = -!!IsKeyPressed(KEY_UP) + !!IsKeyPressed(KEY_DOWN);
            if (change)
            {
                APP->current_tile_idx += change;
                if (APP->current_tile_idx < 0) APP->current_tile_idx = 0;
                else if (APP->current_tile_idx > APP->tile_set.count - 1) APP->current_tile_idx = (uint32_t)APP->tile_set.count - 1;
            }
        }

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            World_Position pos = GetWorldPosition(mouse_pos_world);
            World_Tile *world_tile = GetWorldTile(pos.tile_x, pos.tile_y);
            if (world_tile)
            {
                if (APP->draw_solid_mask)
                {
                    world_tile->is_solid = APP->current_is_solid;
                }
                else
                {
                    Tile *old_tile = GetTile(world_tile->index);
                    --old_tile->ref_count;

                    world_tile->index = APP->current_tile_idx;

                    Tile *new_tile = GetTile(world_tile->index);
                    ++new_tile->ref_count;
                }
            }
        }
    }
    else
    {
        TODO("Unimplemented app mode");
    }

}

void UpdateSidePanelView(Rectangle view, float scroll_input)
{
    bool click = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    if (!scroll_input && !click) return;

    Tile_Picker_Props p = GetTilePickerProps(view);

    if (scroll_input)
    {
        if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))
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
            APP->current_tile_idx = tile_index;
        }
    }
}

void DrawBrushPreview(Rectangle world_view)
{
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
        if (APP->draw_solid_mask) legend = "TILE (SOLID)";

        Texture texture = APP->tile_set.items[APP->current_tile_idx].texture;
        DrawTexturePro(texture, (Rectangle){0,0,8,8}, rect, (Vector2){0}, 0, WHITE);
    }
    else if (APP->mode == MODE_DRAW_PIXELS)
    {
        legend = "PIXEL";
        if (APP->auto_new_tile) legend = "PIXEL (AUTO TILE)";
        DrawRectangleRec(rect, palette_gbp[APP->current_color_idx]);
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

void GlobalShortcuts(void)
{
    if ((IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) && IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_F11))
    {
        ToggleFullscreen();
    }


    if (IsKeyPressed(KEY_TAB))
    {
        if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))
        {
            APP->draw_solid_mask ^= APP->mode == MODE_DRAW_TILES;
            APP->auto_new_tile ^= APP->mode == MODE_DRAW_PIXELS;
        }
        else
        {
            APP->mode ^= MODE_DRAW_TILES ^ MODE_DRAW_PIXELS;
        }
    }
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    printf("Hello, World!\n");

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
            UpdateWorldView(mouse_pos_screen, mouse_delta, mouse_scroll);

        if (CheckCollisionPointRec(mouse_pos_screen, side_panel_view))
            UpdateSidePanelView(side_panel_view, mouse_scroll);

        APP->mouse_previous = mouse_pos_screen;

        ///////////////////////////
        //                       //
        //        DRAWING        //
        //                       //
        ///////////////////////////

        BeginDrawing();
        ClearBackground(COLOR_WINDOW_BACKGROUND);

        DrawWorldView(world_view, mouse_pos_screen);
        DrawRectangleLinesEx(world_view, 3, COLOR_PANEL_BORDER);

        DrawSidePanel(side_panel_view);
        DrawRectangleLinesEx(side_panel_view, 3, COLOR_PANEL_BORDER);

        DrawBrushPreview(world_view);

        EndDrawing();

    }
    return 0;
}
