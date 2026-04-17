#pragma region Includes

#include "raylib.h"
#include "raymath.h"
#include "reasings.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "helper.h"
#include "gen.c"

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>

// This defines the function 'share_image_js' which maps to JavaScript
EM_JS(void, EM_JS_ShareImage, (const char* fileName), {
  (async () => {
    try {
      // Convert C string to JS string
      const name = UTF8ToString(fileName);
      
      // Read from Emscripten Virtual File System
      const data = FS.readFile(name);
      
      // Create the File object
      const file = new File([data], name, {
    type:
        'image/png' });
      const shareData = {
        files: [file],
        title: 'Image Share',
        text: 'Shared from Emscripten'
      };

      // Check for browser support and share
      if (navigator.canShare && navigator.canShare(shareData)) {
        await navigator.share(shareData);
      } else {
        console.error("Web Share not supported on this browser.");
      }
    } catch (err) {
    // This will catch the error if the 'user gesture' (key press) has expired
    console.error("Share failed:", err);
    }
})();
});

// Define the function using EM_JS
EM_JS(void, EM_JS_DownloadImage, (const char* fileName), {
  try {
    // 1. Convert C string to JS string
    const name = UTF8ToString(fileName);

    // 2. Read from Emscripten Virtual File System
    const data = FS.readFile(name);

    // 3. Create blob and download link
    const blob = new Blob([data], {
type:
    'image/png' });
    const url = window.URL.createObjectURL(blob);
    const link = document.createElement('a');

    link.href = url;
    link.download = name;
    
    // 4. Trigger download
    document.body.appendChild(link);
    link.click();

    // 5. Cleanup
    document.body.removeChild(link);
    window.URL.revokeObjectURL(url);
    
    console.log("Download triggered for:", name);
}
catch(e)
{
    console.error("Failed to download file from VFS:", e);
}
});

EM_JS(bool, EM_JS_ShareSupported, (), {return !!navigator.canShare});

#else

bool EM_JS_ShareSupported()
{
    return false;
}

#include "rlImGui.h" // include the API header
#include "dcimgui.h"
#endif

#pragma endregion

#pragma region World-Data

typedef enum PlayerState
{
    PLAYER_STATE_IDLE_LEFT = 0,
    PLAYER_STATE_IDLE_RIGHT,
    PLAYER_STATE_MOVING_LEFT,
    PLAYER_STATE_MOVING_RIGHT
} PlayerState;

typedef struct Player
{
    float atAngle;
    PlayerState state;
    float movingTime;
    float speedInDegreesPerSecond;
} Player;

typedef enum TreeType
{
    TREE_TYPE_RECT = 0,
    TREE_TYPE_TRIANGLE,
    TREE_TYPE_CIRCLE
} TreeType;

typedef struct Tree
{
    float width;
    float height;
    float canopyHeight;
    float canopyWidth;
    float atAngle;
    TreeType treeType;
    Color canopyColor;
    Color trunkColor;
} Tree;

typedef struct Cloud
{
    float width;
    float height;
    float floatingHeight;
    float atAngle;
    int cloudType;
} Cloud;

typedef enum BushType
{
    BUSH_TYPE_CLOVER = 0,
    BUSH_TYPE_GRASS,
} BushType;

typedef struct Bush
{
    float size;
    float halfUnitCount; // grass or bushes
    int randomSeed;
    float atAngle;
    BushType bushType;
    Color bushColor;
} Bush;

typedef enum StoneType
{
    STONE_TYPE_FIVE = 5,
    STONE_TYPE_SIX = 6,
    STONE_TYPE_SEVEN = 7,
    STONE_TYPE_EIGHT = 8
} StoneType;

typedef struct Stone
{
    float size;
    float atAngle;
    float depth;
    StoneType stoneType;
} Stone;

typedef struct MileStone
{
    float size;
    float atAngle;
} MileStone;

typedef struct World
{
    int treeCount;
    Tree *trees;

    int cloudCount;
    Cloud *clouds;

    int bushCount;
    Bush *bushes;

    int stoneCount;
    Stone *stones;

    int milestoneCount;
    MileStone *milestones;

    int viewlineCount;
    Vector2 skyViewlineStarts[360];
    Vector2 skyViewlineEnds[360];
    Vector2 earthViewlineStarts[360];
    bool visted[360];
    bool allVisited;

} World;

typedef struct Controls
{
    float tappedDuration;
    bool tappedOnLeft;
} Controls;

typedef enum
{
    EASE_LINEAR = 0,
    EASE_SINE_IN,
    EASE_SINE_IN_OUT,
    EASE_SINE_OUT,
    EASE_CIRCULAR_IN,
    EASE_CIRCULAR_OUT,
    EASE_CUBIC_IN,
    EASE_CUBIC_OUT,
    EASE_QUADRATIC_IN,
    EASE_QUADRATIC_OUT,
    EASE_EXPONENTIAL_IN,
    EASE_EXPONENTIAL_OUT,

} EaseType;

typedef struct Display
{
    bool isLandscape;
    int width;
    int height;
    int cwidth;  // corrected width
    int cheight; // corrected height
    int cwidthOffset;
    int cheightOffset;
    int cWidthUnits;
    int cHeightUnits;
    int pixelsPerUnit;
    float lineThicknessFactor;
    float dpi;
    float defaultRotation;
    float floorRadiusUnits;

    Camera2D *cameraInUse;
    Camera2D playerCamera;
    Camera2D worldCamera;
    float zoomAtRest;
    float zoomWhileMoving;

    EaseType zoomEaseType;
    float zoomStart;
    float zoomEnd;
    float zoomTime;
    float elapsedZoomTime;
    Vector2 startTarget;
    Vector2 startOffset;
    float startRotation;
} Display;

#pragma endregion

#pragma region Config-Data

typedef struct WorldConfig
{
    int treeCount;
    int cloudCount;
    int bushCount;
    int stoneCount;
    int milestoneCount;
} WorldConfig;

typedef struct CameraConfig
{
    float angleShowAtRest;
    float angleShowWhileMoving;

    EaseType restToMoveEase;
    EaseType moveToRestEase;

    float restToMoveZoomDuration;
    float moveToRestZoomDuration;

    Vector2 offset;

} CameraConfig;

typedef struct GameplayConfig
{
    float timeForFullRotation;
    float speedInDegreesPerSecondStart;
    float speedInDegreesPerSecondTop;
    float movingTimeToTopSpeed;
} GameplayConfig;

typedef struct EditorConfig
{
    bool showDemoWindow;
    bool showAngleValues;
    bool showRadialLines;
} EditorConfig;

typedef struct ContorlsConfig
{
    float maxDurationForQuickTap;
} ContorlsConfig;

typedef struct Config
{
    WorldConfig world;
    CameraConfig camera;
    GameplayConfig gameplay;
    EditorConfig editor;
    ContorlsConfig controls;
} Config;

#pragma endregion

typedef enum GameState
{
    GAME_STATE_START = 0,
    GAME_STATE_PLAYING,
    GAME_STATE_STOPPED
} GameState;

typedef enum DrawableType
{
    DRAW_CIRCLE = 0,
    DRAW_TRIANGLE, // Only Isosceles supported
    DRAW_RECTANGLE,
    DRAW_POLY_5,
    DRAW_POLY_6,
    DRAW_POLY_7,
    DRAW_POLY_8,
    DRAW_CLOVER,
    DRAW_GEAR,
} DrawableType;

typedef struct Drawable
{
    DrawableType type;
    Vector2 dimensions;
    Vector2 metadata;
    Color color;
} Drawable;

typedef struct RadialDrawable
{
    float atRadiusOffset;
    float atAngle;
    Drawable drawable;
} RadialDrawable;

typedef struct Game
{
    RadialDrawable drawables[1000];
    int drawableCount;
    GameState state;
    Config config;
    World world;
    Player player;
    Display display;
    Controls controls;
} Game;

Game game = {0};

void BeginRadialDraw(float atRadius, float atAngle)
{
    rlPushMatrix();
    rlTranslatef(atRadius * sinf(atAngle * DEG2RAD), -atRadius * cosf(atAngle * DEG2RAD), 0);
    rlRotatef(atAngle, 0, 0, 1);
}

void EndRadialDraw()
{
    rlPopMatrix();
}

float AngleByLine(Vector2 center, Vector2 point) {
    float angle = atan2f(point.y - center.y, point.x - center.x) * RAD2DEG;
    return angle < 0 ? 360 + angle : angle; 
    
}

void RadialDraw(RadialDrawable data)
{

    float atRadius = (game.display.floorRadiusUnits + data.atRadiusOffset) * game.display.pixelsPerUnit;
    float scale = game.display.pixelsPerUnit;
    float lineThickness = 7 * game.display.lineThicknessFactor;
    Color lineColor = BLACK;

    rlPushMatrix();
    rlTranslatef(atRadius * sinf(data.atAngle * DEG2RAD), -atRadius * cosf(data.atAngle * DEG2RAD), 0);
    rlRotatef(data.atAngle, 0, 0, 1);

    Drawable obj = data.drawable;

    switch (obj.type)
    {
    case DRAW_CLOVER:
    {

        Color color = obj.color;
        Vector2 center = {0};
        float radiusH = obj.dimensions.x * scale;
        float radiusV = obj.dimensions.y * scale;

        int angles[] = {180, 210, 250 - 3, 290 + 3, 330, 360};
        Vector2 points[6] = {0};
        for (int i = 0; i < 6; i++)
        {
            points[i].x = center.x + cosf(DEG2RAD * angles[i]) * radiusH;
            points[i].y = center.y + sinf(DEG2RAD * angles[i]) * radiusV;
        }

        for (int i = 0; i < 5; i++)
        {
            Vector2 pointA = points[i];
            Vector2 pointB = points[i + 1];
            float chordLength = Vector2Distance(pointA, pointB);
            Vector2 normal = Vector2Normalize(Vector2Rotate(Vector2Subtract(pointB, pointA), 90 * DEG2RAD));
            
            Vector2 pointCenter = Vector2Scale(Vector2Add(pointA, pointB), 0.5f);
            pointCenter = Vector2Add(pointCenter, Vector2Scale(normal, chordLength * 0.2f));

            float cloverRadius = Vector2Distance(pointCenter, pointA);
            
            float fromAngle = AngleByLine(pointCenter, pointA);
            
            float toAngle = AngleByLine(pointCenter, pointB);
            if (toAngle < fromAngle) {
                toAngle += 360;
            }
            

            DrawCircleSector(pointCenter, cloverRadius,fromAngle, toAngle, 10, obj.color);
            DrawRing(pointCenter, cloverRadius-lineThickness, cloverRadius, fromAngle, toAngle, 10, BLACK);
        }
        DrawLineEx(points[0], points[5], lineThickness, BLACK);

        
        rlBegin(RL_TRIANGLES);
        for (int i = 0; i < 5; i++)
        {
            Vector2 pointA = points[i];
            Vector2 pointB = points[i + 1];

            rlColor4ub(color.r, color.g, color.b, color.a);
            rlVertex2f(center.x, center.y);
            rlVertex2f(pointB.x, pointB.y);
            rlVertex2f(pointA.x, pointA.y);
        }
        rlEnd();

       

        break;
    }
    case DRAW_CIRCLE:
    {
        DrawCircle(0, -obj.dimensions.x * scale * 0.5f, obj.dimensions.x * scale * 0.5f, obj.color);
        DrawRing((Vector2){0, -obj.dimensions.x * scale * 0.5f}, obj.dimensions.x * scale * 0.5f - lineThickness, obj.dimensions.x * scale, 0, 360, 36, lineColor);
        break;
    }

    case DRAW_TRIANGLE:
    {
        Vector2 left = {.x = -obj.dimensions.x * scale * 0.5f, .y = 0};
        Vector2 right = {.x = obj.dimensions.x * scale * 0.5f, .y = 0};
        Vector2 top = {.x = 0, .y = -obj.dimensions.y * scale};

        DrawTriangle(left, right, top, obj.color);
        DrawLineEx(left, right, lineThickness, lineColor);
        DrawLineEx(right, top, lineThickness, lineColor);
        DrawLineEx(top, left, lineThickness, lineColor);
        break;
    }

    case DRAW_RECTANGLE:
    {
        Rectangle rect = {
            .x = -obj.dimensions.x * scale * 0.5f,
            .y = -obj.dimensions.y * scale,
            .width = obj.dimensions.x * scale,
            .height = obj.dimensions.y * scale};

        DrawRectangleRec(rect, obj.color);
        DrawRectangleLinesEx(rect, lineThickness, lineColor);
        break;
    }
    case DRAW_POLY_5:
    {
        DrawPoly((Vector2){0, -obj.dimensions.x * scale}, 5, obj.dimensions.x * scale, 0, BLACK);
        DrawPoly((Vector2){0, -obj.dimensions.x * scale}, 5, obj.dimensions.x * scale - lineThickness, 0, obj.color);
        break;
    }
    case DRAW_POLY_6:
    {
        DrawPoly((Vector2){0, -obj.dimensions.x * scale}, 6, obj.dimensions.x * scale, 0, BLACK);
        DrawPoly((Vector2){0, -obj.dimensions.x * scale}, 6, obj.dimensions.x * scale - lineThickness, 0, obj.color);
        break;
    }
    case DRAW_POLY_7:
    {
        DrawPoly((Vector2){0, -obj.dimensions.x * scale}, 7, obj.dimensions.x * scale, 0, BLACK);
        DrawPoly((Vector2){0, -obj.dimensions.x * scale}, 7, obj.dimensions.x * scale - lineThickness, 0, obj.color);
        break;
    }
    case DRAW_POLY_8:
    {
        DrawPoly((Vector2){0, -obj.dimensions.x * scale}, 8, obj.dimensions.x * scale, 0, BLACK);
        DrawPoly((Vector2){0, -obj.dimensions.x * scale}, 8, obj.dimensions.x * scale - lineThickness, 0, obj.color);
        break;
    }
    default:
        break;
    }

    rlPopMatrix();
}

#pragma region func definitions
void InitGame(Game *game, int width, int height, float dpi, bool isLandscape);
void FreeGame(Game *game);
void UpdateDrawFrame(void);
void TriggerSharePhoto();

#pragma endregion

int main(void)
{

    int width, height;
    float dpi;
    bool isLandscape = false;
#ifdef PLATFORM_WEB
    width = emscripten_run_script_int("window.innerWidth");
    height = emscripten_run_script_int("window.innerHeight");
    dpi = emscripten_run_script_int("window.devicePixelRatio * 100") / 100.0f;
    isLandscape = height > width && width < 768;
#else
    width = 1000 * 1.4f;
    height = 450 * 1.4f;
    dpi = 2.4;
    isLandscape = height > width; // Force landscape for desktop
#endif

    srand(time(NULL));
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetConfigFlags(FLAG_VSYNC_HINT);
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(width, height, "Walk");
    InitGame(&game, width, height, dpi, isLandscape);

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    rlImGuiSetup(false);                                      // sets up ImGui with ether a dark or light default theme
    SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor())); // Set our game to run at 60 frames-per-second

    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        UpdateDrawFrame();
    }
#endif

#ifdef PLATFORM_DESKTOP
    rlImGuiShutdown(); // cleans up ImGui
#endif
    CloseWindow();

    FreeGame(&game);
    return 0;
}

#pragma region Game Generation Functions

Color GetRandomColor(int count, Color options[])
{
    int index = randomInt(0, count - 1);
    return options[index];
}

int GetRandomInt(int count, int options[])
{
    int index = randomInt(0, count - 1);
    return options[index];
}

void GenerateTrees(Game *game)
{

    float averageAngleBetweenTrees = 360.0f / game->config.world.treeCount;
    float angleOffset = averageAngleBetweenTrees * 0.1f;

    int drawableOffset = game->drawableCount;
    game->drawableCount += game->config.world.treeCount * 2;

    Color trunkColors[] = {BROWN, BROWN, DARKBROWN, DARKBROWN, DARKBROWN};
    Color canopyColors[] = {GREEN, LIME, LIME, LIME, DARKGREEN, DARKGREEN, DARKGREEN, DARKGREEN, DARKGREEN, DARKGREEN};
    int canopyTypes[] = {DRAW_CLOVER, DRAW_CLOVER, DRAW_RECTANGLE, DRAW_RECTANGLE, DRAW_TRIANGLE, DRAW_TRIANGLE, DRAW_TRIANGLE};

    for (int i = 0; i < game->config.world.treeCount; i++)
    {
        DrawableType canopyType = GetRandomInt(7, canopyTypes);

        RadialDrawable tree = {0};
        RadialDrawable canopy = {0};
        switch (canopyType)
        {
        case DRAW_RECTANGLE:
            tree.drawable.dimensions.x = randomFloat(0.5f, 2.3f);
            tree.drawable.dimensions.y = randomFloat(3.5f, 13.5f);
            canopy.drawable.dimensions.x = randomFloat(12.0f, 18.0f);
            canopy.drawable.dimensions.y = randomFloat(6.0f, 12.0f);
            break;
        case DRAW_TRIANGLE:
            tree.drawable.dimensions.x = randomFloat(0.8f, 1.8f);
            tree.drawable.dimensions.y = randomFloat(2.5f, 9.7f);
            canopy.drawable.dimensions.x = randomFloat(4.5f, 8.0f);
            canopy.drawable.dimensions.y = randomFloat(12.0f, 19.0f);
            break;
        case DRAW_CIRCLE:
        case DRAW_CLOVER:
            tree.drawable.dimensions.x = randomFloat(0.5f, 1.2f);
            tree.drawable.dimensions.y = randomFloat(8.0f, 16.0f);
            canopy.drawable.dimensions.x = randomFloat(8.0f, 12.0f);
            canopy.drawable.dimensions.y = randomFloat(6.0f, 10.0f);
            break;
        default:
            break;
        }
        tree.atAngle = randomFloat(averageAngleBetweenTrees * i + angleOffset, averageAngleBetweenTrees * (i + 1) - angleOffset);
        tree.atRadiusOffset = 0;
        tree.drawable.type = DRAW_RECTANGLE;
        tree.drawable.color = GetRandomColor(5, trunkColors);

        canopy.drawable.type = canopyType;
        canopy.drawable.color = GetRandomColor(10, canopyColors);
        canopy.atAngle = tree.atAngle;
        canopy.atRadiusOffset = tree.drawable.dimensions.y;

        game->drawables[drawableOffset + i * 2] = tree;
        game->drawables[drawableOffset + i * 2 + 1] = canopy;
    }
}

void GenerateClouds(Game *game)
{
    float averageAngleBetween = 360.0f / game->config.world.cloudCount;

    int drawableCount = game->drawableCount;
    game->drawableCount += game->config.world.cloudCount;
    for (int i = 0; i < game->config.world.cloudCount; i++)
    {
        game->drawables[drawableCount + i] = (RadialDrawable){
            .atAngle = randomFloat(averageAngleBetween * i, averageAngleBetween * (i + 1)),
            .atRadiusOffset = randomFloat(12, 25),
            .drawable = {
                .type = DRAW_RECTANGLE,
                .color = LIGHTGRAY,
                .dimensions = {.x = randomFloat(5, 15), .y = randomFloat(2, 8)}}};
    }
}

void GenerateBushes(Game *game)
{
    World *world = &game->world;
    world->bushCount = game->config.world.bushCount;
    world->bushes = (Bush *)malloc(world->bushCount * sizeof(Bush));
    float averageAngleBetweenBushes = 360.0f / world->bushCount;

    for (int i = 0; i < world->bushCount; i++)
    {

        world->bushes[i].randomSeed = randomInt(0, INT32_MAX);
        world->bushes[i].atAngle = randomFloat(averageAngleBetweenBushes * i, averageAngleBetweenBushes * (i + 1));
        world->bushes[i].bushType = randomInt(BUSH_TYPE_CLOVER, BUSH_TYPE_GRASS);
        switch (world->bushes[i].bushType)
        {
        case BUSH_TYPE_CLOVER:
        {
            int halfUnitOptions[] = {1, 1, 1, 1, 2};
            world->bushes[i].halfUnitCount = GetRandomInt(5, halfUnitOptions);
            world->bushes[i].size = randomFloat(1.7f, 4.0f);
            break;
        }
        case BUSH_TYPE_GRASS:
            world->bushes[i].halfUnitCount = randomInt(1, 4);
            world->bushes[i].size = randomFloat(0.9f, 4.2f);
            break;
        default:
            break;
        }
        Color bushColors[] = {GREEN, LIME, LIME, LIME, DARKGREEN, DARKGREEN, DARKGREEN, DARKGREEN, DARKGREEN, DARKGREEN};
        world->bushes[i].bushColor = GetRandomColor(10, bushColors);
    }
}

void GenerateStones(Game *game)
{
    float averageAngleBetweenStones = 360.0f / game->config.world.stoneCount;

    int drawableCount = game->drawableCount;
    game->drawableCount += game->config.world.stoneCount;
    for (int i = 0; i < game->config.world.stoneCount; i++)
    {
        game->drawables[drawableCount + i] = (RadialDrawable){
            .atAngle = randomFloat(averageAngleBetweenStones * i, averageAngleBetweenStones * (i + 1)),
            .atRadiusOffset = -randomFloat(1.85f, 9.4f),
            .drawable = {
                .type = randomInt(DRAW_POLY_5, DRAW_POLY_8),
                .color = DARKGRAY,
                .dimensions = {.x = randomFloat(0.5f, 1.4f)}}};
    }
}

void GenerateMileStones(Game *game)
{
    World *world = &game->world;
    world->milestoneCount = game->config.world.milestoneCount;
    world->milestones = (MileStone *)malloc(world->milestoneCount * sizeof(MileStone));
    float averageAngleBetweenMilestones = 360.0f / (world->milestoneCount + 1);

    for (int i = 0; i < world->milestoneCount; i++)
    {
        world->milestones[i].size = randomFloat(1.5f, 3.0f);
        world->milestones[i].atAngle = averageAngleBetweenMilestones * (i + 1);
    }
}

void GenerateWorld(Game *game)
{
    game->drawableCount = 0;

    GenerateClouds(game);
    GenerateTrees(game);
    GenerateBushes(game);
    GenerateStones(game);
    GenerateMileStones(game);
}

void FreeGame(Game *game)
{
    World world = game->world;
    free(world.trees);
    free(world.clouds);
    free(world.bushes);
    free(world.stones);
    free(world.milestones);
}

#pragma endregion

void ScreenResized(Game *game, int width, int height, float dpi, bool isLandscape, bool isFirstTime)
{
    game->display.isLandscape = isLandscape;
    game->display.cHeightUnits = 9;
    game->display.cWidthUnits = 16;
    game->display.width = width;
    game->display.height = height;
    game->display.dpi = dpi;

    float ppy = height / (isLandscape ? game->display.cWidthUnits : game->display.cHeightUnits);
    float ppx = width / (isLandscape ? game->display.cHeightUnits : game->display.cWidthUnits);
    if (ppx < ppy)
    {
        game->display.pixelsPerUnit = ppx;
    }
    else
    {
        game->display.pixelsPerUnit = ppy;
    }
    game->display.lineThicknessFactor = game->display.pixelsPerUnit / 70.0f;
    game->display.cwidth = game->display.pixelsPerUnit * (isLandscape ? game->display.cHeightUnits : game->display.cWidthUnits);
    game->display.cheight = game->display.pixelsPerUnit * (isLandscape ? game->display.cWidthUnits : game->display.cHeightUnits);
    game->display.cwidthOffset = (width - game->display.cwidth) / 2;
    game->display.cheightOffset = (height - game->display.height) / 2;
    game->display.floorRadiusUnits = (isLandscape ? game->display.cheight : game->display.cwidth) / (2 * sinf(game->config.camera.angleShowWhileMoving * DEG2RAD * 0.5f));
    game->display.floorRadiusUnits /= game->display.pixelsPerUnit;

    float chordLengthWhileMoving = game->display.pixelsPerUnit * 2 * game->display.floorRadiusUnits * sinf(DEG2RAD * (game->config.camera.angleShowWhileMoving / 2));
    float chordLengthAtRest = game->display.pixelsPerUnit * 2 * game->display.floorRadiusUnits * sinf(DEG2RAD * (game->config.camera.angleShowAtRest / 2));
    game->display.zoomAtRest = (isLandscape ? game->display.cheight : game->display.cwidth) / chordLengthAtRest;
    game->display.zoomWhileMoving = (isLandscape ? game->display.cheight : game->display.cwidth) / chordLengthWhileMoving;
    game->display.zoomStart = game->display.zoomWhileMoving;
    game->display.zoomEnd = game->display.zoomWhileMoving;
    game->display.defaultRotation = isLandscape ? 90 : 0;
    float overviewZoom = (isLandscape ? game->display.cwidth : game->display.cheight) / (game->display.floorRadiusUnits * game->display.pixelsPerUnit * 3.2f);

    game->display.playerCamera.offset = (Vector2){width * game->config.camera.offset.x, height * game->config.camera.offset.y};
    if (isLandscape)
    {
        game->display.playerCamera.offset = (Vector2){width * (1 - game->config.camera.offset.y), height * game->config.camera.offset.x};
    }

    if (isFirstTime)
    {
        game->display.playerCamera.target = (Vector2){0, -game->display.floorRadiusUnits * game->display.pixelsPerUnit};
        game->display.playerCamera.rotation = game->display.defaultRotation;
        game->display.playerCamera.zoom = game->display.zoomWhileMoving;
    }
    else
    {
        game->display.playerCamera.zoom = game->player.state < PLAYER_STATE_MOVING_LEFT ? game->display.zoomAtRest : game->display.zoomWhileMoving;
    }

    if (isFirstTime)
    {
    }
    game->display.worldCamera.offset = (Vector2){width * 0.5f, height * 0.5f};
    game->display.worldCamera.rotation = game->display.defaultRotation;
    game->display.worldCamera.zoom = overviewZoom;

    TraceLog(LOG_INFO, "Pixels per unit: %d", game->display.pixelsPerUnit);
    TraceLog(LOG_INFO, "Radius in units: %f, actual: %f", game->display.floorRadiusUnits, game->display.floorRadiusUnits * game->display.pixelsPerUnit);
    TraceLog(LOG_INFO, "Zoom at rest: %f, zoom while moving: %f", game->display.zoomAtRest, game->display.zoomWhileMoving);
    TraceLog(LOG_INFO, "Overview zoom: %f", game->display.worldCamera.zoom);
}

void InitConfig(Game *game, int width, int height, float dpi, bool isLandscape)
{
    // Camera
    {
        game->config.camera.angleShowAtRest = 60;
        game->config.camera.angleShowWhileMoving = 14.365f; // to get moving zoom to 0.25f TODO: shift starting config to zoom level
        game->config.camera.restToMoveEase = EASE_SINE_OUT;
        game->config.camera.moveToRestEase = EASE_SINE_IN;
        game->config.camera.restToMoveZoomDuration = 3.0f;
        game->config.camera.moveToRestZoomDuration = 5.0f;
        game->config.camera.offset = (Vector2){0.5f, 0.7f};
    }

    // display
    {
        ScreenResized(game, width, height, dpi, isLandscape, true);
    }

    // World
    {
        game->config.world.treeCount = 45;
        game->config.world.cloudCount = 30;
        game->config.world.bushCount = 40;
        game->config.world.stoneCount = 65;
        game->config.world.milestoneCount = 0;
    }

    // Editor
    {
        game->config.editor.showDemoWindow = false;
        game->config.editor.showAngleValues = false;
        game->config.editor.showRadialLines = true;
    }
    game->config.gameplay.timeForFullRotation = 120.0f;
    game->config.gameplay.speedInDegreesPerSecondStart = 360.0f / game->config.gameplay.timeForFullRotation;
    game->config.gameplay.speedInDegreesPerSecondTop = game->config.gameplay.speedInDegreesPerSecondStart * 1.75f;
    game->config.gameplay.movingTimeToTopSpeed = 20.0f;          // 5 seconds to reach top speed
    game->config.controls.maxDurationForQuickTap = 20.0f / 60.f; // 20 frames in 60 frames
}

void InitGame(Game *game, int width, int height, float dpi, bool isLandscape)
{

    InitConfig(game, width, height, dpi, isLandscape);

    // Init ViewLines
    {
        float floorRadius = game->display.floorRadiusUnits;
        game->world.viewlineCount = 360;

        game->world.earthViewlineStarts[0].x = 0;
        game->world.earthViewlineStarts[0].y = 0;

        for (int i = 0; i < game->world.viewlineCount; i++)
        {
            float angle = (360.0f / game->world.viewlineCount) * i * DEG2RAD;
            game->world.earthViewlineStarts[i].x = (floorRadius - 0.2f) * sinf(angle);
            game->world.earthViewlineStarts[i].y = -(floorRadius - 0.2f) * cosf(angle);

            game->world.skyViewlineStarts[i].x = (floorRadius + 0.2f) * sinf(angle);
            game->world.skyViewlineStarts[i].y = -(floorRadius + 0.2f) * cosf(angle);

            game->world.skyViewlineEnds[i].x = (floorRadius + 35) * sinf(angle);
            game->world.skyViewlineEnds[i].y = -(floorRadius + 35) * cosf(angle);
        }
    }

    game->state = GAME_STATE_START;
    // Init Player
    game->player = (Player){.atAngle = 0, .state = PLAYER_STATE_MOVING_RIGHT};

    // Init Camera
    {
        game->display.cameraInUse = &game->display.playerCamera;
    }

    GenerateWorld(game);
}

char *start = "Tap and Hold to START";
char *stop = "Tap and Hold to FINISH";
char *wait = "Tap and Hold to OBSERVE";

void DrawSomething(float pixelsPerUnit)
{
    DrawTriangle(
        (Vector2){-10 * pixelsPerUnit, 0},
        (Vector2){10 * pixelsPerUnit, 0},
        (Vector2){0, -10 * pixelsPerUnit},

        RED);
}

void UpdateDrawFrame(void)
{
    if (IsWindowResized())
    {
        int width = GetScreenWidth();
        int height = GetScreenHeight();
        float dpi = game.display.dpi;
        bool isLandscape = height > width;
        ScreenResized(&game, width, height, dpi, isLandscape, false);
    }

    float deltaTime = GetFrameTime();

    if (IsKeyReleased(KEY_R))
    {
        GenerateWorld(&game);
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        game.controls.tappedOnLeft = GetMousePosition().x < game.display.width / 2;
        game.controls.tappedDuration = 0;
        game.player.movingTime = 0;
        game.player.speedInDegreesPerSecond = game.config.gameplay.speedInDegreesPerSecondStart;
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
    {
        game.controls.tappedDuration += deltaTime;
    }

    Config config = game.config;

    // Process Input
    {
        switch (game.state)
        {
        case GAME_STATE_START:
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
            {
                if (game.controls.tappedDuration > game.config.controls.maxDurationForQuickTap)
                {
                    if (game.display.zoomEnd != game.display.zoomAtRest)
                    {
                        game.display.zoomEaseType = game.config.camera.moveToRestEase;
                        game.display.elapsedZoomTime = 0;
                        game.display.zoomStart = game.display.playerCamera.zoom;
                        game.display.zoomEnd = game.display.zoomAtRest;
                        game.display.zoomTime = game.config.camera.moveToRestZoomDuration;
                    }
                }
            }
            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
            {
                if (game.controls.tappedDuration <= game.config.controls.maxDurationForQuickTap)
                {
                    game.player.state = game.player.state == PLAYER_STATE_MOVING_LEFT ? PLAYER_STATE_MOVING_RIGHT : PLAYER_STATE_MOVING_LEFT;
                }
                else
                {
                    game.display.zoomEaseType = game.config.camera.restToMoveEase;
                    game.display.elapsedZoomTime = 0;
                    game.display.zoomStart = game.display.playerCamera.zoom;
                    game.display.zoomEnd = game.display.zoomWhileMoving;
                    game.display.zoomTime = game.config.camera.restToMoveZoomDuration;
                }
                game.controls.tappedDuration = 0;
            }
            break;
        case GAME_STATE_PLAYING:
            switch (game.player.state)
            {

            case PLAYER_STATE_MOVING_LEFT:
            case PLAYER_STATE_MOVING_RIGHT:
                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
                {
                    if (game.controls.tappedDuration > game.config.controls.maxDurationForQuickTap)
                    {
                        game.player.state = game.player.state == PLAYER_STATE_MOVING_LEFT ? PLAYER_STATE_IDLE_LEFT : PLAYER_STATE_IDLE_RIGHT;
                        game.display.zoomEaseType = game.config.camera.moveToRestEase;
                        game.display.elapsedZoomTime = 0;
                        game.display.zoomStart = game.display.playerCamera.zoom;
                        game.display.zoomEnd = game.display.zoomAtRest;
                        game.display.zoomTime = game.config.camera.moveToRestZoomDuration;
                    }
                }
                else
                {

                    if ((IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) && game.controls.tappedDuration <= game.config.controls.maxDurationForQuickTap)
                    {
                        game.player.state = game.player.state == PLAYER_STATE_MOVING_LEFT ? PLAYER_STATE_MOVING_RIGHT : PLAYER_STATE_MOVING_LEFT;
                    }
                    else
                    {

                        game.player.movingTime += deltaTime;
                        if (game.player.movingTime > game.config.gameplay.movingTimeToTopSpeed)
                        {
                            game.player.speedInDegreesPerSecond = game.config.gameplay.speedInDegreesPerSecondTop;
                        }
                        else
                        {
                            game.player.speedInDegreesPerSecond = EaseSineIn(game.player.movingTime,
                                                                             game.config.gameplay.speedInDegreesPerSecondStart, (game.config.gameplay.speedInDegreesPerSecondTop - game.config.gameplay.speedInDegreesPerSecondStart), game.config.gameplay.movingTimeToTopSpeed);
                        }

                        float speed = game.player.speedInDegreesPerSecond * deltaTime;
                        game.player.atAngle = game.player.atAngle + (game.player.state == PLAYER_STATE_MOVING_LEFT ? -speed : speed);
                        if (game.player.atAngle > 360)
                        {
                            game.player.atAngle -= 360;
                        }
                        else if (game.player.atAngle < 0)
                        {
                            game.player.atAngle += 360;
                        }
                    }
                }

                break;

            case PLAYER_STATE_IDLE_RIGHT:
            case PLAYER_STATE_IDLE_LEFT:
                if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
                {
                    game.player.state = game.player.state == PLAYER_STATE_IDLE_RIGHT ? PLAYER_STATE_MOVING_RIGHT : PLAYER_STATE_MOVING_LEFT;
                    game.display.zoomEaseType = game.config.camera.restToMoveEase;
                    game.display.elapsedZoomTime = 0;
                    game.display.zoomStart = game.display.playerCamera.zoom;
                    game.display.zoomEnd = game.display.zoomWhileMoving;
                    game.display.zoomTime = game.config.camera.restToMoveZoomDuration;
                }
                break;

            default:
                break;
            }
            break;
        case GAME_STATE_STOPPED:
            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
            {
                game.display.zoomEaseType = EASE_LINEAR;
                game.display.zoomStart = game.display.playerCamera.zoom;
                game.display.zoomEnd = game.display.worldCamera.zoom;
                game.display.elapsedZoomTime = 0;
                game.display.zoomTime = game.config.camera.moveToRestZoomDuration * 2.0f;
                game.display.startTarget = game.display.playerCamera.target;
                game.display.startOffset = game.display.playerCamera.offset;
            }
            break;
        }
    }

    float pixelsPerUnit = game.display.pixelsPerUnit;
    float floorRadius = game.display.floorRadiusUnits * pixelsPerUnit;

    // Update Camera
    {
        float currentCameraToAngle = game.player.atAngle;
        game.display.playerCamera.target.x = floorRadius * sinf(currentCameraToAngle * DEG2RAD);
        game.display.playerCamera.target.y = -floorRadius * cosf(currentCameraToAngle * DEG2RAD);
        game.display.playerCamera.rotation = game.display.defaultRotation - currentCameraToAngle;

        if (game.display.zoomEnd != game.display.playerCamera.zoom)
        {

            game.display.elapsedZoomTime += deltaTime;
            if (game.display.elapsedZoomTime < game.display.zoomTime)
            {
                switch (game.display.zoomEaseType)
                {
                case EASE_LINEAR:
                    game.display.playerCamera.zoom = EaseLinearIn(game.display.elapsedZoomTime, game.display.zoomStart, (game.display.zoomEnd - game.display.zoomStart), game.display.zoomTime);
                    break;
                case EASE_SINE_IN:
                    game.display.playerCamera.zoom = EaseSineIn(game.display.elapsedZoomTime, game.display.zoomStart, (game.display.zoomEnd - game.display.zoomStart), game.display.zoomTime);
                    break;
                case EASE_SINE_IN_OUT:
                    game.display.playerCamera.zoom = EaseSineInOut(game.display.elapsedZoomTime, game.display.zoomStart, (game.display.zoomEnd - game.display.zoomStart), game.display.zoomTime);
                    break;
                case EASE_SINE_OUT:
                    game.display.playerCamera.zoom = EaseSineOut(game.display.elapsedZoomTime, game.display.zoomStart, (game.display.zoomEnd - game.display.zoomStart), game.display.zoomTime);
                    break;
                case EASE_CIRCULAR_IN:
                    game.display.playerCamera.zoom = EaseCircIn(game.display.elapsedZoomTime, game.display.zoomStart, (game.display.zoomEnd - game.display.zoomStart), game.display.zoomTime);
                    break;
                case EASE_CIRCULAR_OUT:
                    game.display.playerCamera.zoom = EaseCircOut(game.display.elapsedZoomTime, game.display.zoomStart, (game.display.zoomEnd - game.display.zoomStart), game.display.zoomTime);
                    break;
                case EASE_CUBIC_IN:
                    game.display.playerCamera.zoom = EaseCubicIn(game.display.elapsedZoomTime, game.display.zoomStart, (game.display.zoomEnd - game.display.zoomStart), game.display.zoomTime);
                    break;
                case EASE_CUBIC_OUT:
                    game.display.playerCamera.zoom = EaseCubicOut(game.display.elapsedZoomTime, game.display.zoomStart, (game.display.zoomEnd - game.display.zoomStart), game.display.zoomTime);
                    break;
                case EASE_QUADRATIC_IN:
                    game.display.playerCamera.zoom = EaseQuadIn(game.display.elapsedZoomTime, game.display.zoomStart, (game.display.zoomEnd - game.display.zoomStart), game.display.zoomTime);
                    break;
                case EASE_QUADRATIC_OUT:
                    game.display.playerCamera.zoom = EaseQuadOut(game.display.elapsedZoomTime, game.display.zoomStart, (game.display.zoomEnd - game.display.zoomStart), game.display.zoomTime);
                    break;
                case EASE_EXPONENTIAL_IN:
                    game.display.playerCamera.zoom = EaseExpoIn(game.display.elapsedZoomTime, game.display.zoomStart, (game.display.zoomEnd - game.display.zoomStart), game.display.zoomTime);
                    break;
                case EASE_EXPONENTIAL_OUT:
                    game.display.playerCamera.zoom = EaseExpoOut(game.display.elapsedZoomTime, game.display.zoomStart, (game.display.zoomEnd - game.display.zoomStart), game.display.zoomTime);
                    break;
                default:
                    break;
                }
                if (game.state == GAME_STATE_STOPPED)
                {
                    game.display.playerCamera.rotation = EaseSineOut(game.display.elapsedZoomTime, game.display.startRotation, (0 - game.display.startRotation), game.display.zoomTime);
                    game.display.playerCamera.target.x = EaseSineOut(game.display.elapsedZoomTime, game.display.startTarget.x, (0 - game.display.startTarget.x), game.display.zoomTime);
                    game.display.playerCamera.target.y = EaseSineOut(game.display.elapsedZoomTime, game.display.startTarget.y, (0 - game.display.startTarget.y), game.display.zoomTime);
                    game.display.playerCamera.offset.x = EaseSineOut(game.display.elapsedZoomTime, game.display.startOffset.x, (game.display.worldCamera.offset.x - game.display.startOffset.x), game.display.zoomTime);
                    game.display.playerCamera.offset.y = EaseSineOut(game.display.elapsedZoomTime, game.display.startOffset.y, (game.display.worldCamera.offset.y - game.display.startOffset.y), game.display.zoomTime);
                }
            }
            else
            {
                game.display.playerCamera.zoom = game.display.zoomEnd;
                game.display.elapsedZoomTime = 0;
                if (game.state == GAME_STATE_START)
                {
                    if (game.display.zoomEnd == game.display.zoomAtRest)
                    {
                        game.state = GAME_STATE_PLAYING;
                        game.player.state = game.controls.tappedOnLeft ? PLAYER_STATE_IDLE_LEFT : PLAYER_STATE_IDLE_RIGHT;
                    }
                }
                else if (game.state == GAME_STATE_PLAYING)
                {
                    if (game.world.allVisited && (game.player.state == PLAYER_STATE_IDLE_LEFT || game.player.state == PLAYER_STATE_IDLE_RIGHT))
                    {
                        float angleForHouse = RAD2DEG * game.display.pixelsPerUnit * 6 / (floorRadius);
                        TraceLog(LOG_INFO, "Angle for house %f", angleForHouse);
                        if (game.player.atAngle < angleForHouse / 2 || game.player.atAngle > 360 - angleForHouse / 2)
                        {
                            game.state = GAME_STATE_STOPPED;
                        }
                    }
                }
                else if (game.state == GAME_STATE_STOPPED)
                {
                    game.display.cameraInUse = &game.display.worldCamera;
                }
            }
        }
    }
    // Render
    {
        BeginDrawing();

#ifdef PLATFORM_DESKTOP
        rlImGuiBegin(); // starts the ImGui content mode. Make all ImGui calls after this
        ImGui_ShowDemoWindow(&game.config.editor.showDemoWindow);
#endif
        ClearBackground(SKYBLUE);

        if (IsKeyPressed(KEY_TAB))
        {
            if (game.display.cameraInUse == &game.display.playerCamera)
            {
                game.display.cameraInUse = &game.display.worldCamera;
            }
            else
            {
                game.display.cameraInUse = &game.display.playerCamera;
            }
        }
        // DrawCircleSector((Vector2){game.display.width / 4, game.display.height / 4}, 50 * game.display.lineThicknessFactor, 0, 360, 36, ORANGE);
        BeginMode2D(*game.display.cameraInUse);

        // float playerAngleInRad = game.player.atAngle * DEG2RAD;
        float lineThickness = game.display.lineThicknessFactor * 7;
        if (game.display.cameraInUse == &game.display.worldCamera)
        {
            lineThickness *= 3.0f;
        }
        int segments = 36;

        Color floorColor = LIME;
        float radiusTill = floorRadius;
        float radiusFrom = floorRadius;
        float floorBorderThickness = game.display.lineThicknessFactor * 7.0f;
        float floorSeperatorThickness = game.display.lineThicknessFactor * 4.0f;
        // Draw Bushes
        {
            for (int i = 0; i < game.world.bushCount; i++)
            {

                Bush *bushes = game.world.bushes;

                switch (bushes[i].bushType)
                {
                case BUSH_TYPE_CLOVER:
                {

                    float bushSize = bushes[i].size * pixelsPerUnit;
                    int halfUnitCount = bushes[i].halfUnitCount;
                    SetRandomSeed(bushes[i].randomSeed);
                    float offsetHeight = GetRandomValue(30, 50) / 100.0f;
                    float offsetBetweenUnits = GetRandomValue(90, 120) / 100.0f;
                    float scaleRadius = GetRandomValue(60, 70) / 100.0f;

                    float radius = floorRadius - bushSize * offsetHeight;
                    float bushX = radius * sinf(bushes[i].atAngle * DEG2RAD);
                    float bushY = -radius * cosf(bushes[i].atAngle * DEG2RAD);

                    DrawCircleSector((Vector2){bushX, bushY}, bushSize, 0, 360, segments, bushes[i].bushColor);
                    DrawRing((Vector2){bushX, bushY}, bushSize, bushSize + lineThickness, 0, 360, segments, BLACK);
                    float bushRadius = bushSize * scaleRadius;
                    for (int j = 0; j < halfUnitCount; j++)
                    {
                        radius = floorRadius - bushRadius * offsetHeight;
                        bushX = radius * sinf(bushes[i].atAngle * DEG2RAD);
                        bushY = -radius * cosf(bushes[i].atAngle * DEG2RAD);

                        DrawCircleSector((Vector2){bushX - bushSize * (j + 1) * offsetBetweenUnits * cosf(bushes[i].atAngle * DEG2RAD), bushY - bushSize * (j + 1) * offsetBetweenUnits * sinf(bushes[i].atAngle * DEG2RAD)}, bushRadius, 0, 360, segments, bushes[i].bushColor);
                        DrawRing((Vector2){bushX - bushSize * (j + 1) * offsetBetweenUnits * cosf(bushes[i].atAngle * DEG2RAD), bushY - bushSize * (j + 1) * offsetBetweenUnits * sinf(bushes[i].atAngle * DEG2RAD)}, bushRadius, bushRadius + lineThickness, 0, 360, segments, BLACK);

                        DrawCircleSector((Vector2){bushX + bushSize * (j + 1) * offsetBetweenUnits * cosf(bushes[i].atAngle * DEG2RAD), bushY + bushSize * (j + 1) * offsetBetweenUnits * sinf(bushes[i].atAngle * DEG2RAD)}, bushRadius, 0, 360, segments, bushes[i].bushColor);
                        DrawRing((Vector2){bushX + bushSize * (j + 1) * offsetBetweenUnits * cosf(bushes[i].atAngle * DEG2RAD), bushY + bushSize * (j + 1) * offsetBetweenUnits * sinf(bushes[i].atAngle * DEG2RAD)}, bushRadius, bushRadius + lineThickness, 0, 360, segments, BLACK);
                        bushRadius *= scaleRadius;
                    }
                }
                break;
                case BUSH_TYPE_GRASS:
                {
                    float bushSize = bushes[i].size * pixelsPerUnit;
                    int halfUnitCount = bushes[i].halfUnitCount;
                    SetRandomSeed(bushes[i].randomSeed);
                    float offsetBetweenUnits = GetRandomValue(125, 145) / 100.0f;
                    float scaleHeight = GetRandomValue(90, 130) / 100.0f;
                    float ratioOfHeight = GetRandomValue(5, 24) / 100.0f;
                    float radius = floorRadius - bushSize * 0.25f;
                    float bushX = radius * sinf(bushes[i].atAngle * DEG2RAD);
                    float bushY = -radius * cosf(bushes[i].atAngle * DEG2RAD);

                    Vector2 bushCenter = {bushX, bushY};
                    float bladeSize = bushSize * ratioOfHeight;
                    for (int j = -halfUnitCount; j <= halfUnitCount; j++)
                    {
                        Vector2 bladeCenter = Vector2Add(bushCenter, Vector2Scale(Vector2Rotate((Vector2){-1, 0}, bushes[i].atAngle * DEG2RAD), (j)*bladeSize * 2 * offsetBetweenUnits));
                        Vector2 bladeLeft = Vector2Add(bladeCenter, Vector2Scale(Vector2Rotate((Vector2){-1, 0}, bushes[i].atAngle * DEG2RAD), bladeSize));
                        Vector2 bladeRight = Vector2Add(bladeCenter, Vector2Scale(Vector2Rotate((Vector2){-1, 0}, bushes[i].atAngle * DEG2RAD), -bladeSize));
                        Vector2 bladeTop = Vector2Add(bladeCenter, Vector2Scale(Vector2Rotate((Vector2){0, -1}, bushes[i].atAngle * DEG2RAD), bushSize));

                        DrawTriangle(bladeLeft, bladeRight, bladeTop, bushes[i].bushColor);
                        DrawLineEx(bladeLeft, bladeTop, lineThickness * 0.6f, BLACK);
                        DrawLineEx(bladeRight, bladeTop, lineThickness * 0.6f, BLACK);
                    }
                    bushSize = bushSize * scaleHeight;
                    for (int j = -halfUnitCount; j <= halfUnitCount; j++)
                    {

                        Vector2 bladeCenter = Vector2Add(bushCenter, Vector2Scale(Vector2Rotate((Vector2){-1, 0}, bushes[i].atAngle * DEG2RAD), (j - 0.5f) * bladeSize * 2 * offsetBetweenUnits));
                        Vector2 bladeLeft = Vector2Add(bladeCenter, Vector2Scale(Vector2Rotate((Vector2){-1, 0}, bushes[i].atAngle * DEG2RAD), bladeSize));
                        Vector2 bladeRight = Vector2Add(bladeCenter, Vector2Scale(Vector2Rotate((Vector2){-1, 0}, bushes[i].atAngle * DEG2RAD), -bladeSize));
                        Vector2 bladeTop = Vector2Add(bladeCenter, Vector2Scale(Vector2Rotate((Vector2){0, -1}, bushes[i].atAngle * DEG2RAD), bushSize));

                        DrawTriangle(bladeLeft, bladeRight, bladeTop, bushes[i].bushColor);
                        DrawLineEx(bladeLeft, bladeTop, lineThickness * 0.6f, BLACK);
                        DrawLineEx(bladeRight, bladeTop, lineThickness * 0.6f, BLACK);
                    }
                }

                default:
                    break;
                }
            }
        }

        // Draw Milestones
        if (game.display.cameraInUse == &game.display.playerCamera)
        {
            for (int i = 0; i < game.world.milestoneCount; i++)
            {
                MileStone *milestones = game.world.milestones;
                float mileStoneSize = 2 * pixelsPerUnit;

                float radius = floorRadius + mileStoneSize * 0.8f;
                float mileStoneX = radius * sinf(milestones[i].atAngle * DEG2RAD);
                float mileStoneY = -radius * cosf(milestones[i].atAngle * DEG2RAD);
                Vector2 mileStoneCenter = (Vector2){mileStoneX, mileStoneY};

                DrawPoly(mileStoneCenter, 5, mileStoneSize, 270 + milestones[i].atAngle, ColorLerp(WHITE, GRAY, 0.6f));
                DrawPolyLinesEx(mileStoneCenter, 5, mileStoneSize, 270 + milestones[i].atAngle, lineThickness, BLACK);

                float circleRadius = mileStoneSize * 0.56f;
                float angleOffset = 3.4f;
                DrawRing(mileStoneCenter, circleRadius, circleRadius + lineThickness * 1.4f, 0, 360, 36, BLACK);
                DrawRing(mileStoneCenter, circleRadius, circleRadius + lineThickness * 1.4f, milestones[i].atAngle - angleOffset - 90, milestones[i].atAngle + angleOffset - 90, 3, GREEN);
                DrawRing(mileStoneCenter, circleRadius, circleRadius + lineThickness * 1.4f, milestones[i].atAngle * 2 - angleOffset - 90, milestones[i].atAngle * 2 + angleOffset - 90, 3, RED);
            }
        }

        float playerWidth = 1.2f * pixelsPerUnit;
        float playerHeight = 2.4f * pixelsPerUnit;

        // Draw floor border
        DrawRing((Vector2){0, 0}, radiusFrom, radiusFrom + floorBorderThickness, 0, 360, 360, BLACK);

        // Draw floor
        {
            floorColor = BEIGE;
            radiusTill = radiusFrom;
            radiusFrom = radiusFrom - game.display.pixelsPerUnit * 2.5f;
            DrawCircleSector((Vector2){0, 0}, radiusTill, 0, 360, 360, floorColor);
            DrawRing((Vector2){0, 0}, radiusFrom, radiusFrom + floorSeperatorThickness, 0, 360, 360, BLACK);

            floorColor = BROWN;
            radiusTill = radiusFrom;
            radiusFrom = radiusFrom - game.display.pixelsPerUnit * 3.5f;
            DrawCircleSector((Vector2){0, 0}, radiusTill, 0, 360, 360, floorColor);
            DrawRing((Vector2){0, 0}, radiusFrom, radiusFrom + floorSeperatorThickness, 0, 360, 360, BLACK);

            floorColor = DARKBROWN;
            radiusTill = radiusFrom;
            radiusFrom = radiusFrom - game.display.pixelsPerUnit * 4.5f;
            DrawCircleSector((Vector2){0, 0}, radiusTill, 0, 360, 360, floorColor);
            DrawRing((Vector2){0, 0}, radiusFrom, radiusFrom + floorSeperatorThickness, 0, 360, 360, BLACK);

            floorColor = DARKGRAY;
            radiusTill = radiusFrom;
            radiusFrom = 0;
            DrawCircleSector((Vector2){0, 0}, radiusTill, 0, 360, 360, floorColor);
            DrawRing((Vector2){0, 0}, radiusFrom, radiusFrom + floorSeperatorThickness, 0, 360, 360, BLACK);
        }

        if (config.editor.showAngleValues)
        {
            for (int i = 0; i < 360; i += 2)
            {
                float angleRad = i * DEG2RAD;
                Vector2 pos = (Vector2){
                    (floorRadius + 35) * sinf(angleRad),
                    -(floorRadius + 35) * cosf(angleRad)};

                DrawTextPro(GetFontDefault(), TextFormat("%i", i),
                            pos, (Vector2){0, 0}, i,
                            30, 1, DARKGRAY);
            }
        }

        {
            Display dsp = game.display;
            // Check collisions
            Vector2 topLeft = GetScreenToWorld2D((Vector2){dsp.cwidthOffset, dsp.cheightOffset}, game.display.playerCamera);
            Vector2 topRight = GetScreenToWorld2D((Vector2){dsp.width - dsp.cwidthOffset, dsp.cheightOffset}, game.display.playerCamera);
            Vector2 bottomLeft = GetScreenToWorld2D((Vector2){dsp.cwidthOffset, game.display.height - dsp.cheightOffset}, game.display.playerCamera);
            Vector2 bottomRight = GetScreenToWorld2D((Vector2){game.display.width - dsp.cwidthOffset, game.display.height - dsp.cheightOffset}, game.display.playerCamera);
            if (game.display.isLandscape)
            {
                bottomLeft = GetScreenToWorld2D((Vector2){0, 0}, game.display.playerCamera);
                topLeft = GetScreenToWorld2D((Vector2){game.display.width, 0}, game.display.playerCamera);
                topRight = GetScreenToWorld2D((Vector2){game.display.width, game.display.height}, game.display.playerCamera);
                bottomRight = GetScreenToWorld2D((Vector2){0, game.display.height}, game.display.playerCamera);
            }

            if (config.editor.showRadialLines)
            {
                DrawLineV(topLeft, topRight, GREEN);
                DrawLineV(topRight, bottomRight, GREEN);
                DrawLineV(bottomRight, bottomLeft, GREEN);
                DrawLineV(bottomLeft, topLeft, GREEN);
            }

            float bottomLeftAngle = floorf((Vector2AnglePositiveDegree((Vector2){0, -1}, bottomLeft)) * (game.world.viewlineCount / 360.0f));
            float bottomRightAngle = ceilf((Vector2AnglePositiveDegree((Vector2){0, -1}, bottomRight)) * (game.world.viewlineCount / 360.0f));
            Vector2 collisionPoint;

            int i = bottomLeftAngle;
            while (true)
            {

                if (game.state == GAME_STATE_STOPPED)
                {
                    break;
                }

                // For rays on earth bottom screen is enough
                if (CheckCollisionLines(bottomLeft, bottomRight, Vector2Scale(game.world.earthViewlineStarts[i], pixelsPerUnit), (Vector2){0, 0}, &collisionPoint))
                {
                    if (Vector2LengthSqr(collisionPoint) < Vector2LengthSqr(Vector2Scale(game.world.earthViewlineStarts[i], pixelsPerUnit)))
                    {
                        game.world.earthViewlineStarts[i] = Vector2Scale(collisionPoint, 1.0f / pixelsPerUnit);
                        game.world.visted[i] = true;
                    }
                }

                // For rays on sky check top screen
                if (CheckCollisionLines(topLeft, topRight, Vector2Scale(game.world.skyViewlineStarts[i], pixelsPerUnit), Vector2Scale(game.world.skyViewlineEnds[i], pixelsPerUnit), &collisionPoint))
                {
                    if (Vector2LengthSqr(collisionPoint) > Vector2LengthSqr(Vector2Scale(game.world.skyViewlineStarts[i], pixelsPerUnit)))
                    {
                        game.world.skyViewlineStarts[i] = Vector2Scale(collisionPoint, 1.0f / pixelsPerUnit);
                    }
                }

                // For rays on sky check left screen
                if (CheckCollisionLines(bottomLeft, topLeft, Vector2Scale(game.world.skyViewlineStarts[i], pixelsPerUnit), Vector2Scale(game.world.skyViewlineEnds[i], pixelsPerUnit), &collisionPoint))
                {
                    if (Vector2LengthSqr(collisionPoint) > Vector2LengthSqr(Vector2Scale(game.world.skyViewlineStarts[i], pixelsPerUnit)))
                    {
                        game.world.skyViewlineStarts[i] = Vector2Scale(collisionPoint, 1.0f / pixelsPerUnit);
                    }
                }

                // For rays on sky check right screen
                if (CheckCollisionLines(bottomRight, topRight, Vector2Scale(game.world.skyViewlineStarts[i], pixelsPerUnit), Vector2Scale(game.world.skyViewlineEnds[i], pixelsPerUnit), &collisionPoint))
                {
                    if (Vector2LengthSqr(collisionPoint) > Vector2LengthSqr(Vector2Scale(game.world.skyViewlineStarts[i], pixelsPerUnit)))
                    {
                        game.world.skyViewlineStarts[i] = Vector2Scale(collisionPoint, 1.0f / pixelsPerUnit);
                    }
                }

                i++;
                if (i == bottomRightAngle)
                    break;
                i = (i >= game.world.viewlineCount) ? i - game.world.viewlineCount : i;
            }

            if (!game.world.allVisited)
            {
                bool allVisited = true;
                for (int i = 0; i < game.world.viewlineCount; i++)
                {
                    allVisited = allVisited && game.world.visted[i];
                }
                game.world.allVisited = allVisited;
            }
        }

        if (config.editor.showRadialLines)
        {
            for (int i = game.world.viewlineCount - 1; i >= 0; i -= 1)
            {
                DrawLineV((Vector2){0, 0}, Vector2Scale(game.world.earthViewlineStarts[i], pixelsPerUnit), GREEN);
                DrawLineV(Vector2Scale(game.world.skyViewlineStarts[i], pixelsPerUnit), Vector2Scale(game.world.skyViewlineEnds[i], pixelsPerUnit), RED);
            }
        }

        for (int i = 0; i < game.drawableCount; i++)
        {
            RadialDraw(game.drawables[i]);
        }

        if (game.display.cameraInUse == &game.display.worldCamera && !IsKeyDown(KEY_SPACE))
        {
            for (int i = game.world.viewlineCount - 1; i >= 0; i -= 1)
            {
                DrawTriangle(
                    (Vector2){0, 0},
                    Vector2Scale(game.world.earthViewlineStarts[i], pixelsPerUnit),
                    Vector2Scale(game.world.earthViewlineStarts[(i == 0) ? game.world.viewlineCount - 1 : i - 1], pixelsPerUnit),
                    ColorAlpha(GRAY, 0.95f));

                DrawTriangle(
                    Vector2Scale(game.world.skyViewlineEnds[i], pixelsPerUnit),
                    Vector2Scale(game.world.skyViewlineStarts[(i == 0) ? game.world.viewlineCount - 1 : i - 1], pixelsPerUnit),
                    Vector2Scale(game.world.skyViewlineStarts[i], pixelsPerUnit),
                    ColorAlpha(SKYBLUE, 0.9f));
                DrawTriangle(
                    Vector2Scale(game.world.skyViewlineEnds[i], pixelsPerUnit),
                    Vector2Scale(game.world.skyViewlineEnds[(i == 0) ? game.world.viewlineCount - 1 : i - 1], pixelsPerUnit),
                    Vector2Scale(game.world.skyViewlineStarts[(i == 0) ? game.world.viewlineCount - 1 : i - 1], pixelsPerUnit),
                    ColorAlpha(SKYBLUE, 0.9f));
                if (game.state == GAME_STATE_STOPPED)
                {
                    DrawLineEx(Vector2Scale(game.world.earthViewlineStarts[i], pixelsPerUnit), Vector2Scale(game.world.earthViewlineStarts[(i == 0) ? game.world.viewlineCount - 1 : i - 1], pixelsPerUnit), 30 * game.display.lineThicknessFactor, BLACK);
                    DrawLineEx(Vector2Scale(game.world.skyViewlineStarts[i], pixelsPerUnit), Vector2Scale(game.world.skyViewlineStarts[(i == 0) ? game.world.viewlineCount - 1 : i - 1], pixelsPerUnit), 30 * game.display.lineThicknessFactor, BLACK);
                }
            }
        }

        // Draw House
        if (game.state != GAME_STATE_STOPPED)
        {

            RadialDrawable wall = {
                .drawable = {.type = DRAW_RECTANGLE, .color = BROWN, .dimensions = {.x = 6.0f, .y = 4.0f}},
                .atRadiusOffset = 0,
                .atAngle = 0,
            };
            RadialDrawable door = {
                .drawable = {.type = DRAW_RECTANGLE, .color = BEIGE, .dimensions = {.x = 1.2f, .y = 2.4f}},
                .atRadiusOffset = 0,
                .atAngle = 0,
            };
            RadialDrawable roof = {
                .drawable = {.type = DRAW_TRIANGLE, .color = DARKBROWN, .dimensions = {.x = 9.0f, .y = 3.0f}},
                .atRadiusOffset = wall.drawable.dimensions.y,
                .atAngle = 0,
            };

            RadialDraw(wall);
            RadialDraw(door);
            RadialDraw(roof);
        }

        // Draw Player
        if (game.state == GAME_STATE_PLAYING)
        {

            float wheelRadius = playerWidth * 0.55f;

            float bodyWidth = playerWidth * 0.3f;
            float bodyHeight = playerHeight * 0.4f;
            float headRadius = bodyWidth * 0.7f;
            float factor = game.player.state == PLAYER_STATE_MOVING_RIGHT || game.player.state == PLAYER_STATE_IDLE_RIGHT ? 1.0f : -1.0f;

            BeginRadialDraw(floorRadius, game.player.atAngle);
            DrawCircle(-factor * bodyWidth * 0.5f, -wheelRadius * 1.75f - bodyHeight, headRadius, BLACK); // head
            DrawRectanglePro(                                                                             // Torso
                (Rectangle){-factor * bodyWidth * 0.5f, -(wheelRadius * 1.3f),
                            bodyWidth, bodyHeight},
                (Vector2){bodyWidth / 2, 0},
                (180),
                BLACK);
            DrawRectanglePro( // Legs
                (Rectangle){
                    -factor * bodyWidth * 0.15f,
                    -(wheelRadius * 1.3f),
                    bodyWidth, bodyHeight * 1.2f},
                (Vector2){bodyWidth / 2, 0},
                (270 * factor + 30 * factor),
                BLACK);
            DrawRing((Vector2){0, (-wheelRadius)}, wheelRadius - 10 * game.display.lineThicknessFactor, wheelRadius, 0, 360, 60, BLACK); // Wheelchair tire
            DrawCircle(0, (-wheelRadius), wheelRadius - 8 * game.display.lineThicknessFactor, GRAY);                                     // Wheelchair spokes area

            DrawRectanglePro( // arms
                (Rectangle){
                    -factor * bodyWidth * 0.5f,
                    -(wheelRadius * 2.45f),
                    bodyWidth * 0.5, bodyHeight * 0.8f},
                (Vector2){bodyWidth * 0.25, 0},
                (270 * factor + 30 * factor),
                BLACK);

            float spokeThickness = 3 * game.display.lineThicknessFactor;
            float spokeCount = 3;
            for (int i = 0; i < spokeCount; i++)
            {
                DrawLineEx(
                    (Vector2){0, (-wheelRadius)},
                    (Vector2){wheelRadius * cosf((game.player.atAngle * 10 + i * 120) * DEG2RAD),
                              (-wheelRadius - wheelRadius * sinf((game.player.atAngle * 10 + i * 120) * DEG2RAD))},
                    spokeThickness, BLACK);
            }
            EndRadialDraw();
        }

        // Draw Instructions
        Color instructionColor = MAROON;
        if (game.state != GAME_STATE_STOPPED)
        {
            if (game.world.allVisited)
            {

                DrawText(stop, round(-game.display.pixelsPerUnit * 6), round(-floorRadius) - game.display.pixelsPerUnit * 4, 80 * game.display.lineThicknessFactor, instructionColor);
            }
            else
            {
                if (game.state == GAME_STATE_START)
                    DrawText(start, round(-game.display.pixelsPerUnit * 6), round(-floorRadius) - game.display.pixelsPerUnit * 4, 80 * game.display.lineThicknessFactor, instructionColor);
            }
            DrawText("Tap to FLIP", round(-game.display.pixelsPerUnit * 8), round(-floorRadius * 1.2f), 160 * game.display.lineThicknessFactor, instructionColor);
            DrawText(wait, round(-game.display.pixelsPerUnit * 16), round(-floorRadius * 1.3f), 180 * game.display.lineThicknessFactor, instructionColor);
        }

        EndMode2D();

        DrawText(TextFormat("FPS %d", GetFPS()), game.display.width / 2 - game.display.cwidth / 2 + game.display.cwidth - 500, 10, 20 * game.display.lineThicknessFactor, BLACK);
        DrawRectangle(0, 0, (game.display.width - game.display.cwidth) / 2, game.display.height, BLACK);
        DrawRectangle(game.display.width - (game.display.width - game.display.cwidth) / 2, 0, (game.display.width - game.display.cwidth) / 2, game.display.height, BLACK);

        DrawRectangle(0, 0, game.display.width, (game.display.height - game.display.cheight) / 2, BLACK);
        DrawRectangle(0, game.display.height - (game.display.height - game.display.cheight) / 2, game.display.width, (game.display.height - game.display.cheight) / 2, BLACK);

#pragma endregion

#ifdef PLATFORM_DESKTOP
        rlImGuiEnd(); // ends the ImGui content mode. Make all ImGui calls before this
#endif

        if (IsKeyPressed(KEY_S))
        {
            TriggerSharePhoto();
        }
        EndDrawing();
    }
}

void TriggerSharePhoto()
{
    char *name = "walk.png";
    Image share = LoadImageFromScreen();
    if (game.display.isLandscape)
    {
        ImageCrop(&share, (Rectangle){
                              (game.display.width - game.display.cwidth) / 2,
                              (game.display.height - game.display.cheight) / 2 + (game.display.cheight - game.display.cwidth) / 2,
                              game.display.cwidth,
                              game.display.cwidth});
        ImageRotateCCW(&share);
    }
    else
    {
        ImageCrop(&share, (Rectangle){
                              (game.display.width - game.display.cwidth) / 2 + (game.display.cwidth - game.display.cheight) / 2,
                              (game.display.height - game.display.cheight) / 2,
                              game.display.cheight,
                              game.display.cheight});
    }

    ExportImage(share, name);

#ifdef PLATFORM_WEB
    if (EM_JS_ShareSupported())
    {
        EM_JS_ShareImage(name);
    }
    else
    {
        EM_JS_DownloadImage(name);
    }

#endif
}
