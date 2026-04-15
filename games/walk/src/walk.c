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
#else
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
    int width;
    int height;
    int cwidth;  // corrected width
    int cheight; // corrected height
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

typedef struct Game
{
    GameState state;
    Config config;
    World world;
    Player player;
    Display display;
    Controls controls;
} Game;

Game game;

#pragma region func definitions
void InitGame(Game *game, int width, int height, float dpi, bool isLandscape);
void FreeGame(Game *game);
void UpdateDrawFrame(void);
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
    World *world = &game->world;
    WorldConfig params = game->config.world;
    world->treeCount = params.treeCount;
    world->trees = (Tree *)malloc(world->treeCount * sizeof(Tree));
    float averageAngleBetweenTrees = 360.0f / world->treeCount;
    float angleOffset = averageAngleBetweenTrees * 0.1f;
    for (int i = 0; i < world->treeCount; i++)
    {
        world->trees[i].atAngle = randomFloat(averageAngleBetweenTrees * i + angleOffset, averageAngleBetweenTrees * (i + 1) - angleOffset);
        world->trees[i].treeType = rand() % 3; // Assuming 3 types of trees
        switch (world->trees[i].treeType)
        {
        case TREE_TYPE_RECT:
            world->trees[i].width = randomFloat(0.5f, 2.3f);
            world->trees[i].height = randomFloat(3.5f, 13.5f);
            world->trees[i].canopyWidth = randomFloat(12.0f, 18.0f);
            world->trees[i].canopyHeight = randomFloat(6.0f, 12.0f);
            break;
        case TREE_TYPE_TRIANGLE:
            world->trees[i].width = randomFloat(0.8f, 1.8f);
            world->trees[i].height = randomFloat(2.5f, 9.7f);
            world->trees[i].canopyWidth = randomFloat(4.5f, 8.0f);
            world->trees[i].canopyHeight = randomFloat(12.0f, 19.0f);
            break;
        case TREE_TYPE_CIRCLE:
            world->trees[i].width = randomFloat(0.5f, 1.2f);
            world->trees[i].height = randomFloat(8.0f, 16.0f);
            world->trees[i].canopyWidth = randomFloat(8.0f, 12.0f);
            world->trees[i].canopyHeight = randomFloat(6.0f, 10.0f);
            break;
        default:
            break;
        }
        Color trunkColors[] = {BROWN, BROWN, DARKBROWN, DARKBROWN, DARKBROWN};
        world->trees[i].trunkColor = GetRandomColor(5, trunkColors);

        Color canopyColors[] = {GREEN, LIME, LIME, LIME, DARKGREEN, DARKGREEN, DARKGREEN, DARKGREEN, DARKGREEN, DARKGREEN};
        world->trees[i].canopyColor = GetRandomColor(10, canopyColors);
    }
}

void GenerateClouds(Game *game)
{
    World *world = &game->world;
    world->cloudCount = game->config.world.cloudCount;
    world->clouds = (Cloud *)malloc(world->cloudCount * sizeof(Cloud));
    float averageAngleBetweenClouds = 360.0f / world->cloudCount;

    for (int i = 0; i < world->cloudCount; i++)
    {
        world->clouds[i].width = randomFloat(5, 15);
        world->clouds[i].height = randomFloat(2, 8);
        world->clouds[i].floatingHeight = randomFloat(12, 25);
        world->clouds[i].atAngle = randomFloat(averageAngleBetweenClouds * i, averageAngleBetweenClouds * (i + 1));
        world->clouds[i].cloudType = rand() % 3; // Assuming 3 types of clouds
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
    World *world = &game->world;
    world->stoneCount = game->config.world.stoneCount;
    world->stones = (Stone *)malloc(world->stoneCount * sizeof(Stone));
    float averageAngleBetweenStones = 360.0f / world->stoneCount;

    for (int i = 0; i < world->stoneCount; i++)
    {
        world->stones[i].size = randomFloat(0.5f, 1.4f);
        world->stones[i].depth = randomFloat(1.5f, 10.4f);
        world->stones[i].atAngle = randomFloat(averageAngleBetweenStones * i, averageAngleBetweenStones * (i + 1));
        world->stones[i].stoneType = randomInt(STONE_TYPE_FIVE, STONE_TYPE_EIGHT);
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
    GenerateTrees(game);
    GenerateClouds(game);
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
    game->display.floorRadiusUnits = (isLandscape ? game->display.cheight : game->display.cwidth) / (2 * sinf(game->config.camera.angleShowWhileMoving * DEG2RAD * 0.5f));
    game->display.floorRadiusUnits /= game->display.pixelsPerUnit;

    float chordLengthWhileMoving = game->display.pixelsPerUnit * 2 * game->display.floorRadiusUnits * sinf(DEG2RAD * (game->config.camera.angleShowWhileMoving / 2));
    float chordLengthAtRest = game->display.pixelsPerUnit * 2 * game->display.floorRadiusUnits * sinf(DEG2RAD * (game->config.camera.angleShowAtRest / 2));
    game->display.zoomAtRest = (isLandscape ? game->display.cheight : game->display.cwidth) / chordLengthAtRest;
    game->display.zoomWhileMoving = (isLandscape ? game->display.cheight : game->display.cwidth) / chordLengthWhileMoving;
    game->display.zoomStart = game->display.zoomWhileMoving;
    game->display.zoomEnd = game->display.zoomWhileMoving;
    game->display.defaultRotation = isLandscape ? 90 : 0;
    float overviewZoom = (isLandscape ? width : height) / (game->display.floorRadiusUnits * game->display.pixelsPerUnit * 3.2f);

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
        game->config.world.milestoneCount = 5;
    }

    // Editor
    {
        game->config.editor.showDemoWindow = false;
        game->config.editor.showAngleValues = false;
        game->config.editor.showRadialLines = false;
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
                        game.player.state = game.player.state == PLAYER_STATE_MOVING_LEFT ? PLAYER_STATE_IDLE_LEFT : PLAYER_STATE_IDLE_RIGHT;
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

        float playerAngleInRad = game.player.atAngle * DEG2RAD;
        float lineThickness = game.display.lineThicknessFactor * 7;
        if (game.display.cameraInUse == &game.display.worldCamera)
        {
            lineThickness *= 3.0f;
        }
        int segments = 36;
        // Draw Clouds
        for (int i = 0; i < game.world.cloudCount; i++)
        {
            Cloud *clouds = game.world.clouds;
            float radius = floorRadius + clouds[i].floatingHeight * pixelsPerUnit;
            float cloudX = radius * sinf(clouds[i].atAngle * DEG2RAD);
            float cloudY = -radius * cosf(clouds[i].atAngle * DEG2RAD);

            DrawRectanglePro(
                (Rectangle){cloudX, cloudY, clouds[i].width * pixelsPerUnit, clouds[i].height * pixelsPerUnit},
                (Vector2){clouds[i].width * pixelsPerUnit / 2, 0},
                (clouds[i].atAngle + 180),
                LIGHTGRAY);
            DrawRectangleLinesPro(
                (Rectangle){cloudX, cloudY, clouds[i].width * pixelsPerUnit, clouds[i].height * pixelsPerUnit},
                (clouds[i].atAngle + 180),
                BLACK, lineThickness);
        }

        Tree *trees = game.world.trees;
        // Draw Trees
        for (int i = 0; i < game.world.treeCount; i++)
        {

            float treeWidth = trees[i].width * pixelsPerUnit;
            float treeHeight = trees[i].height * pixelsPerUnit;
            float treeX = (floorRadius)*sinf(trees[i].atAngle * DEG2RAD);
            float treeY = -floorRadius * cosf(trees[i].atAngle * DEG2RAD);
            DrawRectanglePro(
                (Rectangle){treeX, treeY, treeWidth, treeHeight},
                (Vector2){treeWidth / 2, 0},
                (trees[i].atAngle + 180),
                trees[i].trunkColor);
            DrawRectangleLinesPro(
                (Rectangle){treeX, treeY, treeWidth, treeHeight},
                (trees[i].atAngle + 180),
                BLACK, lineThickness);

            float canopyX = (floorRadius + treeHeight) * sinf(trees[i].atAngle * DEG2RAD);
            float canopyY = -(floorRadius + treeHeight) * cosf(trees[i].atAngle * DEG2RAD);
            float canopyWidth = trees[i].canopyWidth * pixelsPerUnit;
            float canopyHeight = trees[i].canopyHeight * pixelsPerUnit;
            switch (trees[i].treeType)
            {
            case TREE_TYPE_RECT:
                DrawRectanglePro(
                    (Rectangle){canopyX, canopyY, canopyWidth, canopyHeight},
                    (Vector2){canopyWidth / 2, 0},
                    (trees[i].atAngle + 180),
                    trees[i].canopyColor);
                DrawRectangleLinesPro(
                    (Rectangle){canopyX, canopyY, canopyWidth, canopyHeight},
                    (trees[i].atAngle + 180),
                    BLACK, lineThickness);
                break;
            case TREE_TYPE_TRIANGLE:
            {
                Vector2 top = (Vector2){
                    (floorRadius + treeHeight + canopyHeight) * sinf(trees[i].atAngle * DEG2RAD),
                    -(floorRadius + treeHeight + canopyHeight) * cosf(trees[i].atAngle * DEG2RAD)};
                Vector2 left = (Vector2){
                    (floorRadius + treeHeight) * sinf(trees[i].atAngle * DEG2RAD) - canopyWidth / 2 * cosf(trees[i].atAngle * DEG2RAD),
                    -(floorRadius + treeHeight) * cosf(trees[i].atAngle * DEG2RAD) - canopyWidth / 2 * sinf(trees[i].atAngle * DEG2RAD)};
                Vector2 right = (Vector2){
                    (floorRadius + treeHeight) * sinf(trees[i].atAngle * DEG2RAD) + canopyWidth / 2 * cosf(trees[i].atAngle * DEG2RAD),
                    -(floorRadius + treeHeight) * cosf(trees[i].atAngle * DEG2RAD) + canopyWidth / 2 * sinf(trees[i].atAngle * DEG2RAD)};
                DrawTriangle(top, left, right, trees[i].canopyColor);
                DrawLineEx(top, left, lineThickness, BLACK);
                DrawLineEx(left, right, lineThickness, BLACK);
                DrawLineEx(right, top, lineThickness, BLACK);
                break;
            }
            case TREE_TYPE_CIRCLE:
                DrawCircleSector((Vector2){canopyX, canopyY}, canopyWidth / 2, 0, 360, 36, trees[i].canopyColor);
                DrawRing((Vector2){canopyX, canopyY}, canopyWidth / 2, canopyWidth / 2 + lineThickness, 0, 360, 36, BLACK);
                break;
            }
        }

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
        // Draw House
        if (game.state != GAME_STATE_STOPPED)
        {

            float houseWidth = game.display.pixelsPerUnit * 6;
            float houseHeight = game.display.pixelsPerUnit * 4;
            float roofHeight = houseHeight * 0.75f;
            float roofWidth = houseWidth * 1.5f;
            DrawRectangle(-houseWidth / 2, -floorRadius - houseHeight - 2, houseWidth, houseHeight, BROWN);
            DrawRectangleLinesPro((Rectangle){0, -floorRadius - houseHeight - 2, houseWidth, houseHeight}, 0, BLACK, lineThickness);
            DrawTriangle((Vector2){-roofWidth / 2, -floorRadius - houseHeight}, (Vector2){roofWidth / 2, -floorRadius - houseHeight}, (Vector2){0, -floorRadius - houseHeight - roofHeight}, DARKBROWN);
            DrawLineEx((Vector2){-roofWidth / 2, -floorRadius - houseHeight}, (Vector2){roofWidth / 2, -floorRadius - houseHeight}, lineThickness, BLACK);
            DrawLineEx((Vector2){-roofWidth / 2, -floorRadius - houseHeight}, (Vector2){0, -floorRadius - houseHeight - roofHeight}, lineThickness, BLACK);
            DrawLineEx((Vector2){0, -floorRadius - houseHeight - roofHeight}, (Vector2){roofWidth / 2, -floorRadius - houseHeight}, lineThickness, BLACK);
            DrawRectangle(-playerWidth / 2, -floorRadius - playerHeight, playerWidth, playerHeight, BEIGE);
            DrawRectangleLinesPro((Rectangle){0, -floorRadius - playerHeight, playerWidth, playerHeight}, 0, BLACK, 5 * game.display.lineThicknessFactor);
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
                DrawText(start, round(-game.display.pixelsPerUnit * 6), round(-floorRadius) - game.display.pixelsPerUnit * 4, 80 * game.display.lineThicknessFactor, instructionColor);
            }
            DrawText("Tap to FLIP", round(-game.display.pixelsPerUnit * 8), round(-floorRadius * 1.2f), 160 * game.display.lineThicknessFactor, instructionColor);
            DrawText(wait, round(-game.display.pixelsPerUnit * 16), round(-floorRadius * 1.3f), 180 * game.display.lineThicknessFactor, instructionColor);
        }

        // Draw floor border
        DrawRing((Vector2){0, 0}, radiusFrom, radiusFrom + floorBorderThickness, 0, 360, 360, BLACK);
        // Draw Player
        if (game.state == GAME_STATE_PLAYING)
        {

            float wheelRadius = playerWidth * 0.55f;

            float bodyWidth = playerWidth * 0.3f;
            float bodyHeight = playerHeight * 0.4f;
            float headRadius = bodyWidth * 0.7f;
            float factor = game.player.state == PLAYER_STATE_MOVING_RIGHT || game.player.state == PLAYER_STATE_IDLE_RIGHT ? 1.0f : -1.0f;
            // head
            DrawCircle(
                (floorRadius + wheelRadius * 1.75f + bodyHeight) * sinf(playerAngleInRad) - factor * bodyWidth * 0.5f * cosf(playerAngleInRad),
                -(floorRadius + wheelRadius * 1.75f + bodyHeight) * cosf(playerAngleInRad) - factor * bodyWidth * 0.5f * sinf(playerAngleInRad),
                headRadius, BLACK);
            // Torso
            DrawRectanglePro(
                (Rectangle){
                    (floorRadius + wheelRadius * 1.3f) * sinf(playerAngleInRad) - factor * bodyWidth * 0.5f * cosf(playerAngleInRad),
                    -(floorRadius + wheelRadius * 1.3f) * cosf(playerAngleInRad) - factor * bodyWidth * 0.5f * sinf(playerAngleInRad),
                    bodyWidth, bodyHeight},
                (Vector2){bodyWidth / 2, 0},
                (180 + game.player.atAngle),
                BLACK);
            // Legs
            DrawRectanglePro(
                (Rectangle){
                    (floorRadius + wheelRadius * 1.3f) * sinf(playerAngleInRad) - factor * bodyWidth * 0.15f * cosf(playerAngleInRad),
                    -(floorRadius + wheelRadius * 1.3f) * cosf(playerAngleInRad) - factor * bodyWidth * 0.15f * sinf(playerAngleInRad),
                    bodyWidth, bodyHeight * 1.2f},
                (Vector2){bodyWidth / 2, 0},
                (270 * factor + 30 * factor + game.player.atAngle),
                BLACK);

            DrawRing((Vector2){(floorRadius + wheelRadius) * sinf(playerAngleInRad), (-floorRadius - wheelRadius) * cosf(playerAngleInRad)}, wheelRadius - 10 * game.display.lineThicknessFactor, wheelRadius, 0, 360, 60, BLACK);
            DrawCircle((floorRadius + wheelRadius) * sinf(playerAngleInRad), (-floorRadius - wheelRadius) * cosf(playerAngleInRad), wheelRadius - 8 * game.display.lineThicknessFactor, GRAY);
            float spokeThickness = 3 * game.display.lineThicknessFactor;
            float spokeCount = 3;
            for (int i = 0; i < spokeCount; i++)
            {
                DrawLineEx(
                    (Vector2){(floorRadius + wheelRadius) * sinf(playerAngleInRad), (-floorRadius - wheelRadius) * cosf(playerAngleInRad)},
                    (Vector2){(floorRadius + wheelRadius) * sinf(playerAngleInRad) + wheelRadius * cosf((game.player.atAngle * 10 + i * 120) * DEG2RAD),
                              (-floorRadius - wheelRadius) * cosf(playerAngleInRad) + wheelRadius * sinf((game.player.atAngle * 10 + i * 120) * DEG2RAD)},
                    spokeThickness, BLACK);
            }
            // Arms
            DrawRectanglePro(
                (Rectangle){
                    (floorRadius + wheelRadius * 2.45f) * sinf(playerAngleInRad) - factor * bodyWidth * 0.5f * cosf(playerAngleInRad),
                    -(floorRadius + wheelRadius * 2.45f) * cosf(playerAngleInRad) - factor * bodyWidth * 0.5f * sinf(playerAngleInRad),
                    bodyWidth * 0.5, bodyHeight * 0.8f},
                (Vector2){bodyWidth * 0.25, 0},
                (270 * factor + 30 * factor + game.player.atAngle),
                BLACK);
        }

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

        // Draw Stones
        for (int i = 0; i < game.world.stoneCount; i++)
        {
            Stone *stones = game.world.stones;
            float radius = floorRadius - stones[i].depth * pixelsPerUnit;

            Vector2 stoneCenter = {radius * sinf(stones[i].atAngle * DEG2RAD), -radius * cosf(stones[i].atAngle * DEG2RAD)};

            DrawPoly(stoneCenter, stones[i].stoneType, stones[i].size * pixelsPerUnit, stones[i].atAngle, DARKGRAY);
            DrawPolyLinesEx(stoneCenter, stones[i].stoneType, stones[i].size * pixelsPerUnit, stones[i].atAngle, lineThickness, BLACK);
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
            // Check collisions
            Vector2 topLeft = GetScreenToWorld2D((Vector2){0, 0}, game.display.playerCamera);
            Vector2 topRight = GetScreenToWorld2D((Vector2){game.display.width, 0}, game.display.playerCamera);
            Vector2 bottomLeft = GetScreenToWorld2D((Vector2){0, game.display.height}, game.display.playerCamera);
            Vector2 bottomRight = GetScreenToWorld2D((Vector2){game.display.width, game.display.height}, game.display.playerCamera);
            if (game.display.defaultRotation == 90)
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

        for (int i = game.world.viewlineCount - 1; i >= 0; i -= 1)
        {
            // DrawTriangle(
            //     (Vector2){0, 0},
            //     Vector2Scale(game.world.earthViewlineStarts[i], pixelsPerUnit),
            //     Vector2Scale(game.world.earthViewlineStarts[(i == 0) ? game.world.viewlineCount - 1 : i - 1], pixelsPerUnit),
            //     ColorAlpha(GRAY, 0.95f));

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

        EndDrawing();
    }
}
