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

#pragma region Data
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
} Player;

typedef struct Tree
{
    float width;
    float height;
    float atAngle;
    int treeType;
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
} Floor;

typedef struct WorldConfig
{
    float floorRadius;
    int treeCount;
    float treeWidth[2];
    float treeHeight[2];
    int cloudCount;
    float cloudWidth[2];
    float cloudHeight[2];
    float cloudFloatingHeight[2];
    int rockHatchTileSize;
    int rockHatchSeedOffset;
    float rockHatchBorder;
} WorldConfig;

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

typedef struct CameraConfig
{
    float angleShowAtRest;
    float angleShowWhileMoving;

    float dampenedAngle;
    float angleOffPlayerAtRest;
    float angleOffPlayerWhileMoving;

    EaseType restToMoveEase;
    EaseType moveToRestEase;

    float angleMovePerSecond;
    float restToMoveZoomDuration;
    float moveToRestZoomDuration;

    Vector2 offset;

} CameraConfig;

typedef struct GameplayConfig
{
    float speedInDegreesPerSecond;
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

typedef struct World
{
    Floor floor;
    Tree *trees;
    int treesCount;
    Cloud *clouds;
    int cloudsCount;

    int radialLinesCount;
    Vector2 radialOutStarts[360];
    Vector2 radialOutEnds[360];
    Vector2 radialInStarts[360];
    bool visted[360];
    bool allVisited;
    Texture2D floorTexture;
} World;

typedef struct Hud
{
} Hud;

typedef struct Controls
{
    float tappedDuration;
} Controls;

typedef struct Display
{
    int width;
    int height;
    Camera2D camera;
    Camera2D worldCamera;
    float zoomAtRest;
    float zoomWhileMoving;

    EaseType zoomEaseType;
    float zoomStart;
    float zoomEnd;
    float zoomTime;
    float elapsedZoomTime;

    Hud hud;
} Display;

typedef struct Game
{
    Config config;
    World world;
    Player player;
    Display display;
    Controls controls;
} Game;

#pragma endregion

Game game;

void InitGame(Game *game, int width, int height);
void FreeGame(Game *game);
void UpdateDrawFrame(void);

int main(void)
{

    int width, height;
#ifdef PLATFORM_WEB
    width = emscripten_run_script_int("window.innerWidth");
    height = emscripten_run_script_int("window.innerHeight");
#else
    width = 16 * 90;
    height = 9 * 90;
#endif

    srand(time(NULL));
    InitGame(&game, width, height);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(width, height, "Walk");

    Image rocksImage = GenImageRocksRadial(game.world.floor.radius, 0, 400, 0.13f);
    game.world.floorTexture = LoadTextureFromImage(rocksImage);
    SetTextureFilter(game.world.floorTexture, TEXTURE_FILTER_POINT);
    UnloadImage(rocksImage);

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    rlImGuiSetup(false); // sets up ImGui with ether a dark or light default theme
    SetTargetFPS(60);    // Set our game to run at 60 frames-per-second

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
    for (int i = 0; i < world->treesCount; i++)
    {
        world->trees[i].width = randomFloat(params.treeWidth[0], params.treeWidth[1]);
        world->trees[i].height = randomFloat(params.treeHeight[0], params.treeHeight[1]);
        world->trees[i].atAngle = 360 / world->treesCount * i;
        world->trees[i].treeType = rand() % 3; // Assuming 3 types of trees
    }
}

void GenerateClouds(Game *game)
{
    World *world = &game->world;
    WorldConfig params = game->config.world;
    world->cloudsCount = params.cloudCount;
    world->clouds = (Cloud *)malloc(world->cloudsCount * sizeof(Cloud));
    for (int i = 0; i < world->cloudsCount; i++)
    {
        world->clouds[i].width = randomFloat(params.cloudWidth[0], params.cloudWidth[1]);
        world->clouds[i].height = randomFloat(params.cloudHeight[0], params.cloudHeight[1]);
        world->clouds[i].floatingHeight = randomFloat(params.cloudFloatingHeight[0], params.cloudFloatingHeight[1]);
        world->clouds[i].atAngle = 360 / world->cloudsCount * i;
        world->clouds[i].cloudType = rand() % 3; // Assuming 3 types of clouds
    }
}

void GenerateWorld(Game *game)
{
    GenerateTrees(game);
    GenerateClouds(game);
    float floorRadius = game->config.world.floorRadius;
    game->world.radialLinesCount = 360;
    for (int i = 0; i < game->world.radialLinesCount; i++)
    {
        float angle = (360.0f / game->world.radialLinesCount) * i * DEG2RAD;
        game->world.radialInStarts[i].x = (floorRadius - 15) * sinf(angle);
        game->world.radialInStarts[i].y = -(floorRadius - 15) * cosf(angle);

        game->world.radialOutStarts[i].x = (floorRadius + 15) * sinf(angle);
        game->world.radialOutStarts[i].y = -(floorRadius + 15) * cosf(angle);

        game->world.radialOutEnds[i].x = (floorRadius + 500) * sinf(angle);
        game->world.radialOutEnds[i].y = -(floorRadius + 500) * cosf(angle);
    }
}

void FreeGame(Game *game)
{
    World world = game->world;
    free(world.trees);
    free(world.clouds);
}

#pragma endregion

void InitGame(Game *game, int width, int height)
{
#pragma region Config

    game->config.world.floorRadius = 800;
    game->config.world.rockHatchTileSize = 64;
    game->config.world.rockHatchSeedOffset = 6;
    game->config.world.rockHatchBorder = 1.3f;

    game->config.world.treeCount = 20;
    game->config.world.treeWidth[0] = 10;
    game->config.world.treeWidth[1] = 30;
    game->config.world.treeHeight[0] = 50;
    game->config.world.treeHeight[1] = 150;
    game->config.world.cloudCount = 50;
    game->config.world.cloudWidth[0] = 50;
    game->config.world.cloudWidth[1] = 150;
    game->config.world.cloudHeight[0] = 20;
    game->config.world.cloudHeight[1] = 60;
    game->config.world.cloudFloatingHeight[0] = 85;
    game->config.world.cloudFloatingHeight[1] = 250;

    game->config.camera.angleShowAtRest = 60;
    game->config.camera.angleShowWhileMoving = 20;
    game->config.camera.dampenedAngle = 3;
    game->config.camera.angleOffPlayerAtRest = 0;
    game->config.camera.angleOffPlayerWhileMoving = 0;
    game->config.camera.restToMoveEase = EASE_SINE_OUT;
    game->config.camera.moveToRestEase = EASE_SINE_IN_OUT;
    game->config.camera.angleMovePerSecond = 0.5f;
    game->config.camera.restToMoveZoomDuration = 3.0f;
    game->config.camera.moveToRestZoomDuration = 5.0f;
    game->config.camera.offset = (Vector2){0.5f, 0.6f};

    game->config.gameplay.speedInDegreesPerSecond = 360.0f / (90);

    game->config.editor.showDemoWindow = false;
    game->config.editor.showAngleValues = true;
    game->config.editor.showRadialLines = false;

    game->config.controls.maxDurationForQuickTap = 20.0f / 60.f;

    game->config.player.width = 10;
    game->config.player.height = 20;

#pragma endregion

    game->display.width = width;
    game->display.height = height;
    GenerateWorld(game);
    game->player = (Player){
        .atAngle = 0,
        .state = PLAYER_STATE_MOVING_RIGHT};

    float chordLengthWhileMoving = 2 * game->config.world.floorRadius * sinf(DEG2RAD * (game->config.camera.angleShowWhileMoving / 2));
    float chordLengthAtRest = 2 * game->config.world.floorRadius * sinf(DEG2RAD * (game->config.camera.angleShowAtRest / 2));
    game->world.floor.radius = game->config.world.floorRadius;

    game->display.zoomAtRest = width / chordLengthAtRest;
    game->display.zoomWhileMoving = width / chordLengthWhileMoving;
    game->display.zoomStart = game->display.zoomWhileMoving;
    game->display.zoomEnd = game->display.zoomWhileMoving;
    float overviewZoom = width / (game->config.world.floorRadius * 5.0f);

    game->display.camera = (Camera2D){
        .offset = {width * game->config.camera.offset.x, height * game->config.camera.offset.y},
        .target = {0, -game->config.world.floorRadius},
        .zoom = game->display.zoomWhileMoving};

    game->display.worldCamera = (Camera2D){
        .offset = {width * 0.5f, height * 0.5f},
        .target = (Vector2){0, 0},
        .rotation = 0,
        .zoom = overviewZoom};

    game->display.camera.zoom = game->display.zoomAtRest;
    Vector2 worldBottomMiddleAtRest = GetScreenToWorld2D((Vector2){width / 2, height}, game->display.camera);
    game->world.floor.radiusSeenAtRest = -worldBottomMiddleAtRest.y;
    game->display.camera.zoom = game->display.zoomWhileMoving;
    Vector2 worldBottomMiddleWhileMoving = GetScreenToWorld2D((Vector2){width / 2, height}, game->display.camera);

    game->world.floor.radiusSeenWhileMoving = -worldBottomMiddleWhileMoving.y;
}

void UpdateDrawFrame(void)
{
    if (IsKeyPressed(KEY_Z))
    {
        Vector2 worldCenterAt = GetWorldToScreen2D((Vector2){0, 0}, game.display.camera);
        Vector2 worldPointAt = GetWorldToScreen2D((Vector2){game.world.floor.radius, 0}, game.display.camera);
        TraceLog(LOG_INFO, "World Radius %f", Vector2Distance(worldPointAt, worldCenterAt));
    }
    if (IsKeyReleased(KEY_R))
    {
        Image rocksImage;
        // rocksImage = GenImageRocks(game.world.floor.radius * 2,
        //                                  game.config.world.rockHatchTileSize, game.config.world.rockHatchSeedOffset, game.config.world.rockHatchBorder);
        
        rocksImage = GenImageRocksRadial(game.world.floor.radius, game.world.floor.radiusSeenAtRest, 200, 0.23f);
        game.world.floorTexture = LoadTextureFromImage(rocksImage);
        SetTextureFilter(game.world.floorTexture, TEXTURE_FILTER_POINT);
        UnloadImage(rocksImage);
    }

    float deltaTime = GetFrameTime();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        game.controls.tappedDuration = 0;
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
    {
        game.controls.tappedDuration += deltaTime;
    }

    Config config = game.config;

    // Process Input
    {
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
                    game.display.zoomStart = game.display.camera.zoom;
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
                    float speed = config.gameplay.speedInDegreesPerSecond * deltaTime;
                    game.player.atAngle = game.player.atAngle + (game.player.state == PLAYER_STATE_MOVING_LEFT ? -speed : speed);
                }
            }

            break;

        case PLAYER_STATE_IDLE_RIGHT:
        case PLAYER_STATE_IDLE_LEFT:
            if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT))
            {
                game.player.state = game.player.state == PLAYER_STATE_IDLE_RIGHT ? PLAYER_STATE_MOVING_RIGHT : PLAYER_STATE_MOVING_LEFT;
                game.display.zoomEaseType = game.config.camera.restToMoveEase;
                game.display.elapsedZoomTime = 0;
                game.display.zoomStart = game.display.camera.zoom;
                game.display.zoomEnd = game.display.zoomWhileMoving;
                game.display.zoomTime = game.config.camera.restToMoveZoomDuration;
            }
            break;

        default:
            break;
        }
    }
    float cameraToAngle = game.player.atAngle;
    float floorRadius = config.world.floorRadius;

    float currentCameraToAngle = cameraToAngle;
    game.display.camera.target.x = floorRadius * sinf(currentCameraToAngle * DEG2RAD);
    game.display.camera.target.y = -floorRadius * cosf(currentCameraToAngle * DEG2RAD);
    game.display.camera.rotation = -currentCameraToAngle;

    if (game.display.zoomEnd != game.display.camera.zoom)
    {
        game.display.elapsedZoomTime += deltaTime;
        if (game.display.elapsedZoomTime < game.display.zoomTime)
        {
            switch (game.display.zoomEaseType)
            {
            case EASE_LINEAR:
                game.display.camera.zoom = EaseLinearIn(game.display.elapsedZoomTime, game.display.zoomStart, (game.display.zoomEnd - game.display.zoomStart), game.display.zoomTime);
                break;
            case EASE_SINE_IN:
                game.display.camera.zoom = EaseSineIn(game.display.elapsedZoomTime, game.display.zoomStart, (game.display.zoomEnd - game.display.zoomStart), game.display.zoomTime);
                break;
            case EASE_SINE_IN_OUT:
                game.display.camera.zoom = EaseSineInOut(game.display.elapsedZoomTime, game.display.zoomStart, (game.display.zoomEnd - game.display.zoomStart), game.display.zoomTime);
                break;
            case EASE_SINE_OUT:
                game.display.camera.zoom = EaseSineOut(game.display.elapsedZoomTime, game.display.zoomStart, (game.display.zoomEnd - game.display.zoomStart), game.display.zoomTime);
                break;
            case EASE_CIRCULAR_IN:
                game.display.camera.zoom = EaseCircIn(game.display.elapsedZoomTime, game.display.zoomStart, (game.display.zoomEnd - game.display.zoomStart), game.display.zoomTime);
                break;
            case EASE_CIRCULAR_OUT:
                game.display.camera.zoom = EaseCircOut(game.display.elapsedZoomTime, game.display.zoomStart, (game.display.zoomEnd - game.display.zoomStart), game.display.zoomTime);
                break;
            case EASE_CUBIC_IN:
                game.display.camera.zoom = EaseCubicIn(game.display.elapsedZoomTime, game.display.zoomStart, (game.display.zoomEnd - game.display.zoomStart), game.display.zoomTime);
                break;
            case EASE_CUBIC_OUT:
                game.display.camera.zoom = EaseCubicOut(game.display.elapsedZoomTime, game.display.zoomStart, (game.display.zoomEnd - game.display.zoomStart), game.display.zoomTime);
                break;
            case EASE_QUADRATIC_IN:
                game.display.camera.zoom = EaseQuadIn(game.display.elapsedZoomTime, game.display.zoomStart, (game.display.zoomEnd - game.display.zoomStart), game.display.zoomTime);
                break;
            case EASE_QUADRATIC_OUT:
                game.display.camera.zoom = EaseQuadOut(game.display.elapsedZoomTime, game.display.zoomStart, (game.display.zoomEnd - game.display.zoomStart), game.display.zoomTime);
                break;
            case EASE_EXPONENTIAL_IN:
                game.display.camera.zoom = EaseExpoIn(game.display.elapsedZoomTime, game.display.zoomStart, (game.display.zoomEnd - game.display.zoomStart), game.display.zoomTime);
                break;
            case EASE_EXPONENTIAL_OUT:
                game.display.camera.zoom = EaseExpoOut(game.display.elapsedZoomTime, game.display.zoomStart, (game.display.zoomEnd - game.display.zoomStart), game.display.zoomTime);
                break;
            default:
                break;
            }
        }
        else
        {
            game.display.zoomEnd = game.display.camera.zoom;
            game.display.elapsedZoomTime = 0;
        }
    }

    // Render
    {
        BeginDrawing();

#ifdef PLATFORM_DESKTOP
        rlImGuiBegin(); // starts the ImGui content mode. Make all ImGui calls after this
        ImGui_ShowDemoWindow(&game.config.editor.showDemoWindow);
#endif
        ClearBackground(WHITE);

#pragma region Render World

        if (IsKeyDown(KEY_W))
        {
            BeginMode2D(game.display.worldCamera);
        }
        else
        {
            BeginMode2D(game.display.camera);
        }

        float floorRadius = config.world.floorRadius;
        float playerAngleInRad = game.player.atAngle * DEG2RAD;

        // Draw Player
        {
            DrawRectanglePro(
                (Rectangle){
                    floorRadius * sinf(playerAngleInRad),
                    -floorRadius * cosf(playerAngleInRad),
                    config.player.width, config.player.height},
                (Vector2){config.player.width / 2, 0},
                (180 + game.player.atAngle),
                BLACK);
        }

        DrawTextureEx(game.world.floorTexture, (Vector2){-floorRadius, -floorRadius}, 0, 1.0f, WHITE);
        // Draw Floor
        DrawRing((Vector2){0, 0}, floorRadius - 2, floorRadius, 0, 360, 360, BROWN);

        // DrawCircleSector((Vector2){0, 0}, game.world.floor.radiusSeenAtRest, 0, 360, 360, WHITE);

        if (config.editor.showAngleValues)
        {
            for (int i = 0; i < 360; i += 5)
            {
                float angleRad = i * DEG2RAD;
                Vector2 pos = (Vector2){
                    (floorRadius + 15) * sinf(angleRad),
                    -(floorRadius + 15) * cosf(angleRad)};

                DrawTextPro(GetFontDefault(), TextFormat("%i", i),
                            pos, (Vector2){0, 0}, i,
                            10, 1, DARKGRAY);
            }
        }
        if (config.editor.showRadialLines)
        {
            for (int i = 0; i < game.world.radialLinesCount; i++)
            {
                DrawLineV((Vector2){0, 0}, game.world.radialInStarts[i], GRAY);
                DrawLineV(game.world.radialOutStarts[i], game.world.radialOutEnds[i], GRAY);
            }
        }

        DrawText(TextFormat("FPS %d", GetFPS()), 0, 0, 30, RED);
        {
            // Check collisions
            Vector2 topLeft = GetScreenToWorld2D((Vector2){0, 0}, game.display.camera);
            Vector2 topRight = GetScreenToWorld2D((Vector2){game.display.width, 0}, game.display.camera);
            Vector2 bottomLeft = GetScreenToWorld2D((Vector2){0, game.display.height}, game.display.camera);
            Vector2 bottomRight = GetScreenToWorld2D((Vector2){game.display.width, game.display.height}, game.display.camera);

            if (config.editor.showRadialLines)
            {
                DrawLineV(topLeft, topRight, GREEN);
                DrawLineV(topRight, bottomRight, GREEN);
                DrawLineV(bottomRight, bottomLeft, GREEN);
                DrawLineV(bottomLeft, topLeft, GREEN);
            }

            float bottomLeftAngle = floorf((Vector2AnglePositiveDegree((Vector2){0, -1}, bottomLeft)) * (game.world.radialLinesCount / 360.0f));
            float bottomRightAngle = ceilf((Vector2AnglePositiveDegree((Vector2){0, -1}, bottomRight)) * (game.world.radialLinesCount / 360.0f));
            Vector2 collisionPoint;

            int i = bottomLeftAngle;
            while (true)
            {
                if (IsKeyPressed(KEY_D))
                {
                    TraceLog(LOG_INFO, "Checking %d in (%f, %f)", i, bottomLeftAngle, bottomRightAngle);
                }

                // For rays on earth bottom screen is enough
                if (CheckCollisionLines(bottomLeft, bottomRight, game.world.radialInStarts[i], (Vector2){0, 0}, &collisionPoint))
                {
                    if (Vector2LengthSqr(collisionPoint) < Vector2LengthSqr(game.world.radialInStarts[i]))
                    {
                        game.world.radialInStarts[i] = collisionPoint;
                        game.world.visted[i] = true;
                    }
                }

                // For rays on sky check top screen
                if (CheckCollisionLines(topLeft, topRight, game.world.radialOutStarts[i], game.world.radialOutEnds[i], &collisionPoint))
                {
                    if (Vector2LengthSqr(collisionPoint) > Vector2LengthSqr(game.world.radialOutStarts[i]))
                    {
                        game.world.radialOutStarts[i] = collisionPoint;
                    }
                }

                // For rays on sky check left screen
                if (CheckCollisionLines(bottomLeft, topLeft, game.world.radialOutStarts[i], game.world.radialOutEnds[i], &collisionPoint))
                {
                    if (Vector2LengthSqr(collisionPoint) > Vector2LengthSqr(game.world.radialOutStarts[i]))
                    {
                        game.world.radialOutStarts[i] = collisionPoint;
                    }
                }

                // For rays on sky check right screen
                if (CheckCollisionLines(bottomRight, topRight, game.world.radialOutStarts[i], game.world.radialOutEnds[i], &collisionPoint))
                {
                    if (Vector2LengthSqr(collisionPoint) > Vector2LengthSqr(game.world.radialOutStarts[i]))
                    {
                        game.world.radialOutStarts[i] = collisionPoint;
                    }
                }

                i++;
                if (i == bottomRightAngle)
                    break;
                i = (i >= game.world.radialLinesCount) ? i - game.world.radialLinesCount : i;
            }

            if (!game.world.allVisited)
            {
                bool allVisited = true;
                for (int i = 0; i < game.world.radialLinesCount; i++)
                {
                    allVisited = allVisited && game.world.visted[i];
                }
                game.world.allVisited = allVisited;
            }
        }

        if (game.world.allVisited)
        {
            DrawCircleV((Vector2){0, -floorRadius}, 20, RED);
        }

        // Draw Trees
        for (int i = 0; i < game.world.treesCount; i++)
        {
            Tree *trees = game.world.trees;
            float treeX = floorRadius * sinf(trees[i].atAngle * DEG2RAD);
            float treeY = -floorRadius * cosf(trees[i].atAngle * DEG2RAD);

            DrawLineEx(
                (Vector2){treeX, treeY},
                (Vector2){treeX + trees[i].height * sinf(trees[i].atAngle * DEG2RAD), treeY - trees[i].height * cosf(trees[i].atAngle * DEG2RAD)},
                2,
                DARKGREEN);
        }

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
                (clouds[i].atAngle),
                (Color){130, 130, 130, 55});
        }

        EndMode2D();
#pragma endregion

#ifdef PLATFORM_DESKTOP
        rlImGuiEnd(); // ends the ImGui content mode. Make all ImGui calls before this
#endif
        EndDrawing();
    }
}
