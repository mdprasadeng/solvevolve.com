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

typedef struct Floor
{
    float radius;
    float radiusSeenWhileMoving;
    float radiusSeenAtRest;
    float gapSeenAtRest;
} Floor;

typedef struct World
{
    Floor floor;
    Tree *trees;
    int treesCount;
    Cloud *clouds;
    int cloudsCount;

    int viewlineCount;
    Vector2 skyViewlineStarts[360];
    Vector2 skyViewlineEnds[360];
    Vector2 earthViewlineStarts[360];
    bool visted[360];
    bool allVisited;
    Texture2D floorTexture;
    int floorTextureOffset;
    float floorTextureAngle;
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
    int cwidth;
    int cheight;
    int cWidthUnits;
    int cHeightUnits;
    int pixelsPerUnit;
    float pixelFactor;
    float dpi;
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
    float floorRadius;
    int treeCount;
    int cloudCount;
    float cloudWidth[2];
    float cloudHeight[2];
    float cloudFloatingHeight[2];
    int rockHatchTileSize;
    int rockHatchSeedOffset;
    float rockHatchBorder;
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

typedef struct PlayerConfig
{
    float width;
    float height;
} PlayerConfig;

typedef struct Config
{
    WorldConfig world;
    CameraConfig camera;
    GameplayConfig gameplay;
    EditorConfig editor;
    ContorlsConfig controls;
    PlayerConfig player;
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
void InitGame(Game *game, int width, int height, float dpi);
void FreeGame(Game *game);
void UpdateDrawFrame(void);
#pragma endregion

int main(void)
{

    int width, height;
    float dpi;
#ifdef PLATFORM_WEB
    width = emscripten_run_script_int("window.innerWidth");
    height = emscripten_run_script_int("window.innerHeight");
    dpi = emscripten_run_script_int("window.devicePixelRatio * 100") / 100.0f;
#else
    width = 1000 * 2.4f;
    height = 450 * 2.4f;
    dpi = 2.4;
#endif

    srand(time(NULL));
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetConfigFlags(FLAG_VSYNC_HINT);
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(width, height, "Walk");
    InitGame(&game, width, height, dpi);

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

void GenerateTrees(Game *game)
{
    World *world = &game->world;
    WorldConfig params = game->config.world;
    world->treesCount = params.treeCount;
    world->trees = (Tree *)malloc(world->treesCount * sizeof(Tree));
    float averageAngleBetweenTrees = 360.0f / world->treesCount;
    float angleOffset = averageAngleBetweenTrees * 0.1f;
    for (int i = 0; i < world->treesCount; i++)
    {
        world->trees[i].atAngle = randomFloat(averageAngleBetweenTrees * i + angleOffset, averageAngleBetweenTrees * (i + 1) - angleOffset);
        world->trees[i].treeType = rand() % 3; // Assuming 3 types of trees
        switch (world->trees[i].treeType)
        {
        case TREE_TYPE_RECT:
            world->trees[i].width = game->display.pixelsPerUnit * randomFloat(0.5f, 2.3f);
            world->trees[i].height = game->display.pixelsPerUnit * randomFloat(3.5f, 13.5f);
            world->trees[i].canopyWidth = game->display.pixelsPerUnit * randomFloat(12.0f, 18.0f);
            world->trees[i].canopyHeight = game->display.pixelsPerUnit * randomFloat(6.0f, 12.0f);
            break;
        case TREE_TYPE_TRIANGLE:
            world->trees[i].width = game->display.pixelsPerUnit * randomFloat(0.8f, 1.8f);
            world->trees[i].height = game->display.pixelsPerUnit * randomFloat(2.5f, 9.7f);
            world->trees[i].canopyWidth = game->display.pixelsPerUnit * randomFloat(4.5f, 8.0f);
            world->trees[i].canopyHeight = game->display.pixelsPerUnit * randomFloat(12.0f, 19.0f);
            break;
        case TREE_TYPE_CIRCLE:
            world->trees[i].width = game->display.pixelsPerUnit * randomFloat(0.5f, 1.2f);
            world->trees[i].height = game->display.pixelsPerUnit * randomFloat(8.0f, 16.0f);
            world->trees[i].canopyWidth = game->display.pixelsPerUnit * randomFloat(8.0f, 12.0f);
            world->trees[i].canopyHeight = game->display.pixelsPerUnit * randomFloat(6.0f, 10.0f);
            break;
        default:
            break;
        }
        int trunkColor = randomInt(0, 100);
        if (trunkColor < 25)
        {
            world->trees[i].trunkColor = BEIGE;
        }
        else if (trunkColor < 75)
        {
            world->trees[i].trunkColor = BROWN;
        }
        else
        {
            world->trees[i].trunkColor = DARKBROWN;
        }
        int canopyColor = randomInt(0, 100);
        if (canopyColor < 10)
        {
            world->trees[i].canopyColor = GREEN;
        }
        else if (canopyColor < 40)
        {
            world->trees[i].canopyColor = LIME;
        }
        else
        {
            world->trees[i].canopyColor = DARKGREEN;
        }
    }
}

void GenerateClouds(Game *game)
{
    World *world = &game->world;
    WorldConfig params = game->config.world;
    world->cloudsCount = params.cloudCount;
    world->clouds = (Cloud *)malloc(world->cloudsCount * sizeof(Cloud));
    float averageAngleBetweenClouds = 360.0f / world->cloudsCount;

    for (int i = 0; i < world->cloudsCount; i++)
    {
        world->clouds[i].width = randomFloat(params.cloudWidth[0], params.cloudWidth[1]);
        world->clouds[i].height = randomFloat(params.cloudHeight[0], params.cloudHeight[1]);
        world->clouds[i].floatingHeight = randomFloat(params.cloudFloatingHeight[0], params.cloudFloatingHeight[1]);
        world->clouds[i].atAngle = randomFloat(averageAngleBetweenClouds * i, averageAngleBetweenClouds * (i + 1));
        world->clouds[i].cloudType = rand() % 3; // Assuming 3 types of clouds
    }
}

void GenerateWorld(Game *game)
{
    GenerateTrees(game);
    GenerateClouds(game);
}

void FreeGame(Game *game)
{
    World world = game->world;
    free(world.trees);
    free(world.clouds);
    UnloadTexture(game->world.floorTexture);
}

#pragma endregion

void InitConfig(Game *game, int width, int height, float dpi)
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
        game->display.cHeightUnits = 9;
        game->display.cWidthUnits = 16;
        game->display.width = width;
        game->display.height = height;
        game->display.dpi = dpi;

        float ppy = height / game->display.cHeightUnits;
        float ppx = width / game->display.cWidthUnits;
        if (ppx < ppy)
        {
            game->display.pixelsPerUnit = ppx;
        }
        else
        {
            game->display.pixelsPerUnit = ppy;
        }
        game->display.pixelFactor = game->display.pixelsPerUnit / 70.0f;
        TraceLog(LOG_INFO, "Pixels per unit: %d", game->display.pixelsPerUnit);
        game->display.cwidth = game->display.pixelsPerUnit * game->display.cWidthUnits;
        game->display.cheight = game->display.pixelsPerUnit * game->display.cHeightUnits;
    }

    // World
    {
        game->config.world.floorRadius = game->display.cwidth / (2 * sinf(game->config.camera.angleShowWhileMoving * DEG2RAD * 0.5f));
        TraceLog(LOG_INFO, "Floor radius %f", game->config.world.floorRadius);
        game->config.world.rockHatchTileSize = game->display.pixelsPerUnit * 4;
        game->config.world.rockHatchSeedOffset = game->display.pixelsPerUnit / 3;
        game->config.world.rockHatchBorder = 50.0f / game->display.pixelsPerUnit;

        game->config.world.treeCount = 45;

        game->config.world.cloudCount = 30;

        game->config.world.cloudWidth[0] = game->display.pixelsPerUnit * 5;
        game->config.world.cloudWidth[1] = game->display.pixelsPerUnit * 15;
        game->config.world.cloudHeight[0] = game->display.pixelsPerUnit * 2;
        game->config.world.cloudHeight[1] = 2 * game->display.pixelsPerUnit * 4;

        game->config.world.cloudFloatingHeight[0] = 12 * game->display.pixelsPerUnit;
        game->config.world.cloudFloatingHeight[1] = 25 * game->display.pixelsPerUnit;
    }

    game->config.player.width = game->display.pixelsPerUnit * 1.2f;
    game->config.player.height = game->display.pixelsPerUnit * 2.4f;

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

void InitGame(Game *game, int width, int height, float dpi)
{

    InitConfig(game, width, height, dpi);

    // Init ViewLines
    {
        float floorRadius = game->config.world.floorRadius;
        game->world.viewlineCount = 360;

        game->world.earthViewlineStarts[0].x = 0;
        game->world.earthViewlineStarts[0].y = 0;

        for (int i = 0; i < game->world.viewlineCount; i++)
        {
            float angle = (360.0f / game->world.viewlineCount) * i * DEG2RAD;
            game->world.earthViewlineStarts[i].x = (floorRadius - 15) * sinf(angle);
            game->world.earthViewlineStarts[i].y = -(floorRadius - 15) * cosf(angle);

            game->world.skyViewlineStarts[i].x = (floorRadius + 15) * sinf(angle);
            game->world.skyViewlineStarts[i].y = -(floorRadius + 15) * cosf(angle);

            game->world.skyViewlineEnds[i].x = (floorRadius + game->display.pixelsPerUnit * 35) * sinf(angle);
            game->world.skyViewlineEnds[i].y = -(floorRadius + game->display.pixelsPerUnit * 35) * cosf(angle);
        }
    }

    game->state = GAME_STATE_START;
    // Init Player
    game->player = (Player){.atAngle = 0, .state = PLAYER_STATE_MOVING_RIGHT};

    // Init Camera
    {
        float chordLengthWhileMoving = 2 * game->config.world.floorRadius * sinf(DEG2RAD * (game->config.camera.angleShowWhileMoving / 2));
        float chordLengthAtRest = 2 * game->config.world.floorRadius * sinf(DEG2RAD * (game->config.camera.angleShowAtRest / 2));
        game->world.floor.radius = game->config.world.floorRadius;
        game->display.zoomAtRest = game->display.cwidth / chordLengthAtRest;
        game->display.zoomWhileMoving = game->display.cwidth / chordLengthWhileMoving;
        game->display.zoomStart = game->display.zoomWhileMoving;
        game->display.zoomEnd = game->display.zoomWhileMoving;
        float overviewZoom = height / (game->config.world.floorRadius * 3.2f);

        game->display.playerCamera = (Camera2D){
            .offset = {width * game->config.camera.offset.x, height * game->config.camera.offset.y}, // TODO fix offset for vertical resolution
            .target = {0, -game->config.world.floorRadius},
            .zoom = game->display.zoomWhileMoving};

        game->display.worldCamera = (Camera2D){
            .offset = {width * 0.5f, height * 0.5f},
            .target = (Vector2){0, 0},
            .rotation = 0,
            .zoom = overviewZoom};

        game->display.playerCamera.zoom = game->display.zoomAtRest;
        Vector2 worldBottomMiddleAtRest = GetScreenToWorld2D((Vector2){width / 2, height}, game->display.playerCamera);
        game->world.floor.radiusSeenAtRest = -worldBottomMiddleAtRest.y;
        game->display.playerCamera.zoom = game->display.zoomWhileMoving;
        Vector2 worldBottomMiddleWhileMoving = GetScreenToWorld2D((Vector2){width / 2, height}, game->display.playerCamera);
        game->world.floor.radiusSeenWhileMoving = -worldBottomMiddleWhileMoving.y;
        game->world.floor.gapSeenAtRest = game->world.floor.radius - game->world.floor.radiusSeenAtRest;
        game->display.cameraInUse = &game->display.playerCamera;

        TraceLog(LOG_INFO, "Radius seen %f, %f, %f", game->world.floor.radius, game->world.floor.radiusSeenAtRest, game->world.floor.radius - game->world.floor.radiusSeenAtRest);
    }

    GenerateWorld(game);
}


char *start = "Tap and Hold to START";
char *stop = "Tap and Hold to FINISH";
char *wait = "Tap and Hold to OBSERVE";

void UpdateDrawFrame(void)
{
    float deltaTime = GetFrameTime();
    if (IsKeyPressed(KEY_Z))
    {
        Vector2 worldCenterAt = GetWorldToScreen2D((Vector2){0, 0}, game.display.playerCamera);
        Vector2 worldPointAt = GetWorldToScreen2D((Vector2){game.world.floor.radius, 0}, game.display.playerCamera);
        TraceLog(LOG_INFO, "World Radius %f", Vector2Distance(worldPointAt, worldCenterAt));
    }
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
            break;
        }
    }

    float floorRadius = config.world.floorRadius;

    // Update Camera
    {
        float currentCameraToAngle = game.player.atAngle;
        game.display.playerCamera.target.x = floorRadius * sinf(currentCameraToAngle * DEG2RAD);
        game.display.playerCamera.target.y = -floorRadius * cosf(currentCameraToAngle * DEG2RAD);
        game.display.playerCamera.rotation = -currentCameraToAngle;

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
                if (game.state == GAME_STATE_STOPPED) {
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
                            game.display.zoomEaseType = EASE_LINEAR;
                            game.display.zoomStart = game.display.playerCamera.zoom;
                            game.display.zoomEnd = game.display.worldCamera.zoom;
                            game.display.elapsedZoomTime = 0;
                            game.display.zoomTime = game.config.camera.moveToRestZoomDuration * 2.0f;
                            game.display.startTarget = game.display.playerCamera.target;
                            game.display.startOffset = game.display.playerCamera.offset;
                        }
                    }
                } else if (game.state == GAME_STATE_STOPPED)
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
        BeginMode2D(*game.display.cameraInUse);

        float floorRadius = game.world.floor.radius;
        float playerAngleInRad = game.player.atAngle * DEG2RAD;
        float lineThickness = game.display.pixelFactor * 7;

        // Draw Clouds
        for (int i = 0; i < game.world.cloudsCount; i++)
        {
            Cloud *clouds = game.world.clouds;
            float radius = floorRadius + clouds[i].floatingHeight;
            float cloudX = radius * sinf(clouds[i].atAngle * DEG2RAD);
            float cloudY = -radius * cosf(clouds[i].atAngle * DEG2RAD);

            DrawRectanglePro(
                (Rectangle){cloudX, cloudY, clouds[i].width, clouds[i].height},
                (Vector2){clouds[i].width / 2, 0},
                (clouds[i].atAngle + 180),
                LIGHTGRAY);
            DrawRectangleLinesPro(
                (Rectangle){cloudX, cloudY, clouds[i].width, clouds[i].height},
                (clouds[i].atAngle + 180),
                BLACK, lineThickness);
        }

        Tree *trees = game.world.trees;
        // Draw Trees
        for (int i = 0; i < game.world.treesCount; i++)
        {

            float treeWidth = trees[i].width;
            float treeHeight = trees[i].height;
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
            float canopyWidth = trees[i].canopyWidth;
            float canopyHeight = trees[i].canopyHeight;
            switch (trees[i].treeType)
            {
            case TREE_TYPE_RECT:
                DrawRectanglePro(
                    (Rectangle){canopyX, canopyY, trees[i].canopyWidth, trees[i].canopyHeight},
                    (Vector2){trees[i].canopyWidth / 2, 0},
                    (trees[i].atAngle + 180),
                    trees[i].canopyColor);
                DrawRectangleLinesPro(
                    (Rectangle){canopyX, canopyY, trees[i].canopyWidth, trees[i].canopyHeight},
                    (trees[i].atAngle + 180),
                    BLACK, lineThickness);
                break;
            case TREE_TYPE_TRIANGLE:
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
            case TREE_TYPE_CIRCLE:
                DrawCircleSector((Vector2){canopyX, canopyY}, canopyWidth / 2, 0, 360, 36 * 3, trees[i].canopyColor);
                DrawRing((Vector2){canopyX, canopyY}, canopyWidth / 2, canopyWidth / 2 + lineThickness, 0, 360, 360, BLACK);
                break;
            }
        }

        Color floorColor = LIME;
        float radiusTill = floorRadius;
        float radiusFrom = floorRadius;
        float floorBorderThickness = game.display.pixelFactor * 7.0f;
        float floorSeperatorThickness = game.display.pixelFactor * 4.0f;
        // Draw Grass
        {
            DrawCircleSectorWithTeeth((Vector2){0, 0}, radiusTill - game.display.pixelsPerUnit * 3.2f + 10 * game.display.pixelFactor, 0, 360, 360 / 2, BLACK, game.display.pixelsPerUnit * 4.45f);
            DrawCircleSectorWithTeeth((Vector2){0, 0}, radiusTill - game.display.pixelsPerUnit * 3.2f, 0, 360, 360 / 2, floorColor, game.display.pixelsPerUnit * 4.45f);
        }
        // Draw House
        if (game.state != GAME_STATE_STOPPED) {
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
            DrawRectangle(-game.config.player.width / 2, -floorRadius - game.config.player.height, game.config.player.width, game.config.player.height, BEIGE);
            DrawRectangleLinesPro((Rectangle){0, -floorRadius - game.config.player.height, game.config.player.width, game.config.player.height}, 0, BLACK, 5 * game.display.pixelFactor);
        }

        // Draw Instructions
        Color instructionColor = MAROON;
        if (game.state != GAME_STATE_STOPPED)
        {
            if (game.world.allVisited)
            {

                DrawText(stop, round(-game.display.pixelsPerUnit * 6), round(-floorRadius) - game.display.pixelsPerUnit * 4, 80 * game.display.pixelFactor, instructionColor);
            }
            else
            {
                DrawText(start, round(-game.display.pixelsPerUnit * 6), round(-floorRadius) - game.display.pixelsPerUnit * 4, 80 * game.display.pixelFactor, instructionColor);
            }
            DrawText("Tap to FLIP", round(-game.display.pixelsPerUnit * 8), round(-floorRadius * 1.2f), 160 * game.display.pixelFactor, instructionColor);
            DrawText(wait, round(-game.display.pixelsPerUnit * 16), round(-floorRadius * 1.3f), 180 * game.display.pixelFactor, instructionColor);
        }

        // Draw floor border
        DrawRing((Vector2){0, 0}, radiusFrom, radiusFrom + floorBorderThickness, 0, 360, 360, BLACK);
        // Draw Player
        if (game.state == GAME_STATE_PLAYING && game.display.cameraInUse == &game.display.playerCamera)
        {

            float wheelRadius = game.config.player.width * 0.55f;

            float bodyWidth = game.config.player.width * 0.3f;
            float bodyHeight = game.config.player.height * 0.4f;
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

            DrawRing((Vector2){(floorRadius + wheelRadius) * sinf(playerAngleInRad), (-floorRadius - wheelRadius) * cosf(playerAngleInRad)}, wheelRadius - 10 * game.display.pixelFactor, wheelRadius, 0, 360, 60, BLACK);
            DrawCircle((floorRadius + wheelRadius) * sinf(playerAngleInRad), (-floorRadius - wheelRadius) * cosf(playerAngleInRad), wheelRadius - 8 * game.display.pixelFactor, GRAY);
            float spokeThickness = 3 * game.display.pixelFactor;
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

                if (game.state == GAME_STATE_STOPPED) {
                    break;
                }
                

                // For rays on earth bottom screen is enough
                if (CheckCollisionLines(bottomLeft, bottomRight, game.world.earthViewlineStarts[i], (Vector2){0, 0}, &collisionPoint))
                {
                    if (Vector2LengthSqr(collisionPoint) < Vector2LengthSqr(game.world.earthViewlineStarts[i]))
                    {
                        game.world.earthViewlineStarts[i] = collisionPoint;
                        game.world.visted[i] = true;
                    }
                }

                // For rays on sky check top screen
                if (CheckCollisionLines(topLeft, topRight, game.world.skyViewlineStarts[i], game.world.skyViewlineEnds[i], &collisionPoint))
                {
                    if (Vector2LengthSqr(collisionPoint) > Vector2LengthSqr(game.world.skyViewlineStarts[i]))
                    {
                        game.world.skyViewlineStarts[i] = collisionPoint;
                    }
                }

                // For rays on sky check left screen
                if (CheckCollisionLines(bottomLeft, topLeft, game.world.skyViewlineStarts[i], game.world.skyViewlineEnds[i], &collisionPoint))
                {
                    if (Vector2LengthSqr(collisionPoint) > Vector2LengthSqr(game.world.skyViewlineStarts[i]))
                    {
                        game.world.skyViewlineStarts[i] = collisionPoint;
                    }
                }

                // For rays on sky check right screen
                if (CheckCollisionLines(bottomRight, topRight, game.world.skyViewlineStarts[i], game.world.skyViewlineEnds[i], &collisionPoint))
                {
                    if (Vector2LengthSqr(collisionPoint) > Vector2LengthSqr(game.world.skyViewlineStarts[i]))
                    {
                        game.world.skyViewlineStarts[i] = collisionPoint;
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
                // DrawLineV((Vector2){0, 0}, game.world.earthViewlineStarts[i], GREEN);
                DrawLineV(game.world.skyViewlineStarts[i], game.world.skyViewlineEnds[i], RED);
            }
        }

        for (int i = game.world.viewlineCount - 1; i >= 0; i -= 1)
        {
            DrawTriangle(
                (Vector2){0, 0},
                game.world.earthViewlineStarts[i],
                game.world.earthViewlineStarts[(i == 0) ? game.world.viewlineCount - 1 : i - 1],
                ColorAlpha(GRAY, 0.95f));

            DrawTriangle(
                game.world.skyViewlineEnds[i],
                game.world.skyViewlineStarts[(i == 0) ? game.world.viewlineCount - 1 : i - 1],
                game.world.skyViewlineStarts[i],
                ColorAlpha(SKYBLUE, 0.9f));
            DrawTriangle(
                game.world.skyViewlineEnds[i],
                game.world.skyViewlineEnds[(i == 0) ? game.world.viewlineCount - 1 : i - 1],
                game.world.skyViewlineStarts[(i == 0) ? game.world.viewlineCount - 1 : i - 1],
                ColorAlpha(SKYBLUE, 0.9f));
            if (game.state == GAME_STATE_STOPPED)
            {
                DrawLineEx(game.world.earthViewlineStarts[i], game.world.earthViewlineStarts[(i == 0) ? game.world.viewlineCount - 1 : i - 1], 30 * game.display.pixelFactor, BLACK);
                DrawLineEx(game.world.skyViewlineStarts[i], game.world.skyViewlineStarts[(i == 0) ? game.world.viewlineCount - 1 : i - 1], 30 * game.display.pixelFactor, BLACK);
            }
        }

        EndMode2D();
        DrawText(TextFormat("FPS %d", GetFPS()), game.display.width / 2 - game.display.cwidth / 2 + game.display.cwidth - 100, 10, 20 * game.display.pixelFactor, BLACK);
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
