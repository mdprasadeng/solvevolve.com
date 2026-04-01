#pragma region Includes

#include "raylib.h"
#include "raymath.h"
#include "reasings.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "helper.h"

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
    float shadowAtAngle;
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
    int totalRocks;
    int *pointsPerRock;
    float *boundingRadiusPerRock;
    Vector2 *rockCenters;
    Vector2 *rockPoints;
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
    int totalRocks;
    int rockPointsRange[2];
    float rockRadiusRange[2];
} WorldConfig;

typedef enum
{
    EASE_LINEAR = 0,
    EASE_SINE_IN,
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
} World;

typedef struct Hud
{
} Hud;

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
    InitWindow(width, height, "Walk");

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

void GenerateRocks(Game *game)
{
    World *world = &game->world;
    WorldConfig params = game->config.world;
    world->floor.totalRocks = params.totalRocks;
    world->floor.pointsPerRock = (int *)malloc(params.totalRocks * sizeof(int));
    world->floor.boundingRadiusPerRock = (float *)malloc(params.totalRocks * sizeof(float));
    int totalPoints = 0;
    for (size_t i = 0; i < params.totalRocks; i++)
    {
        world->floor.pointsPerRock[i] = randomInt(params.rockPointsRange[0], params.rockPointsRange[1]);
        world->floor.boundingRadiusPerRock[i] = randomFloat(params.rockRadiusRange[0], params.rockRadiusRange[1]);
        totalPoints += world->floor.pointsPerRock[i];
    }
    world->floor.rockPoints = (Vector2 *)malloc(totalPoints * sizeof(Vector2));
    int pointsIterated = 0;
    for (size_t i = 0; i < params.totalRocks; i++)
    {
        float angleCovered = 0;
        float rockRadius = world->floor.boundingRadiusPerRock[i];
        float rockOffset = rockRadius * 0.2f;
        for (int j = 0; j < world->floor.pointsPerRock[i]; j++)
        {

            float averageAngle = (360 - angleCovered) / ((world->floor.pointsPerRock)[i] - j);
            float minAngleOffset = averageAngle * 0.75f;
            float maxAngleOffset = averageAngle * 1.25f;
            float newAngle = randomFloat(minAngleOffset, maxAngleOffset);
            angleCovered += newAngle;
            (world->floor.rockPoints)[pointsIterated] = (Vector2){
                -rockRadius * cosf(angleCovered * DEG2RAD) + randomFloat(-rockOffset, 0),
                rockRadius * sinf(angleCovered * DEG2RAD) + randomFloat(-rockOffset, 0)};
            pointsIterated++;
        }
    }
    pointsIterated = 0;

    world->floor.rockCenters = (Vector2 *)malloc(params.totalRocks * sizeof(Vector2));
    for (int i = 0; i < params.totalRocks; i++)
    {
        float rockAtRadius = params.floorRadius - world->floor.boundingRadiusPerRock[i] - 10; // 10 is just an extra offset to make sure rocks dont intersect with the world border
        rockAtRadius = rockAtRadius * randomFloat(0.9f, 1.0f);                                // add some noise to the radius of the rock to make it look more natural
        float gapAngle = (2 * PI) / params.totalRocks;
        gapAngle = gapAngle * randomFloat(0.95f, 1.0f); // add some noise to the angle between rocks to make it look more natural
        float rockAngle = (gapAngle * i);
        Vector2 rockCenter = (Vector2){
            -rockAtRadius * cosf(rockAngle),
            rockAtRadius * sinf(rockAngle)};
        for (size_t j = 0; j < world->floor.pointsPerRock[i]; j++)
        {
            world->floor.rockPoints[pointsIterated + j].x += rockCenter.x;
            world->floor.rockPoints[pointsIterated + j].y += rockCenter.y;
        }
        pointsIterated += world->floor.pointsPerRock[i];
        world->floor.rockCenters[i] = rockCenter;
    }
}

void GenerateWorld(Game *game)
{
    GenerateTrees(game);
    GenerateClouds(game);
    GenerateRocks(game);
}

void FreeGame(Game *game)
{
    World world = game->world;
    free(world.floor.rockPoints);
    free(world.floor.pointsPerRock);
    free(world.floor.boundingRadiusPerRock);
    free(world.trees);
    free(world.clouds);
}

#pragma endregion

void InitGame(Game *game, int width, int height)
{
#pragma region Config

    game->config.world.floorRadius = 900;
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
    game->config.world.cloudFloatingHeight[0] = 50;
    game->config.world.cloudFloatingHeight[1] = 150;
    game->config.world.totalRocks = 60;
    game->config.world.rockPointsRange[0] = 5;
    game->config.world.rockPointsRange[1] = 15;
    game->config.world.rockRadiusRange[0] = 5;
    game->config.world.rockRadiusRange[1] = 20;

    game->config.camera.angleShowAtRest = 40;
    game->config.camera.angleShowWhileMoving = 15;
    game->config.camera.dampenedAngle = 3;
    game->config.camera.angleOffPlayerAtRest = 0;
    game->config.camera.angleOffPlayerWhileMoving = 0;
    game->config.camera.restToMoveEase = EASE_SINE_OUT;
    game->config.camera.moveToRestEase = EASE_SINE_IN;
    game->config.camera.angleMovePerSecond = 0.5f;
    game->config.camera.restToMoveZoomDuration = 3.0f;
    game->config.camera.moveToRestZoomDuration = 5.0f;
    game->config.camera.offset = (Vector2){0.5f, 0.6f};

    game->config.gameplay.speedInDegreesPerSecond = 360.0f / (120);

    game->config.editor.showDemoWindow = false;
    game->config.editor.showAngleValues = true;

    game->config.controls.maxDurationForQuickTap = 0.2f;

    game->config.player.width = 20;
    game->config.player.height = 40;

#pragma endregion

    game->display.width = width;
    game->display.height = height;
    GenerateWorld(game);
    game->player = (Player){
        .atAngle = 0,
        .shadowAtAngle = 0,
        .state = PLAYER_STATE_MOVING_RIGHT};

    float chordLengthWhileMoving = 2 * game->config.world.floorRadius * sinf(DEG2RAD * (game->config.camera.angleShowWhileMoving / 2));
    float chordLengthAtRest = 2 * game->config.world.floorRadius * sinf(DEG2RAD * (game->config.camera.angleShowAtRest / 2));
    game->display.zoomAtRest = width / chordLengthAtRest;
    game->display.zoomWhileMoving = width / chordLengthWhileMoving;
    game->display.zoomStart = game->display.zoomWhileMoving;
    game->display.zoomEnd = game->display.zoomWhileMoving;

    game->display.camera = (Camera2D){
        .offset = {width * game->config.camera.offset.x, height * game->config.camera.offset.y},
        .zoom = game->display.zoomWhileMoving};

    game->display.worldCamera = (Camera2D){
        .offset = {width * 0.5f, height * 0.5f},
        .target = (Vector2){0, 0},
        .rotation = 0,
        .zoom = width / (game->config.world.floorRadius * 5.0f)};
}

void UpdateDrawFrame(void)
{

    Config config = game.config;
    float deltaTime = GetFrameTime();
    // Process Input
    {
        switch (game.player.state)
        {

        case PLAYER_STATE_MOVING_LEFT:
        case PLAYER_STATE_MOVING_RIGHT:
            if (IsKeyPressed(KEY_SPACE))
            {
                game.player.state = game.player.state == PLAYER_STATE_MOVING_LEFT ? PLAYER_STATE_IDLE_LEFT : PLAYER_STATE_IDLE_RIGHT;
                game.display.zoomEaseType = game.config.camera.moveToRestEase;
                game.display.elapsedZoomTime = 0;
                game.display.zoomStart = game.display.camera.zoom;
                game.display.zoomEnd = game.display.zoomAtRest;
                game.display.zoomTime = game.config.camera.moveToRestZoomDuration;
            }
            else if (IsKeyPressed(KEY_LEFT_SHIFT))
            {
                game.player.state = game.player.state == PLAYER_STATE_MOVING_LEFT ? PLAYER_STATE_MOVING_RIGHT : PLAYER_STATE_MOVING_LEFT;
            }
            else
            {
                float speed = config.gameplay.speedInDegreesPerSecond * deltaTime;
                game.player.atAngle = game.player.atAngle + (game.player.state == PLAYER_STATE_MOVING_LEFT ? -speed : speed);
            }
            break;

        case PLAYER_STATE_IDLE_RIGHT:
        case PLAYER_STATE_IDLE_LEFT:
            if (!IsKeyDown(KEY_SPACE))
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

        // Draw Floor
        DrawRing((Vector2){0, 0}, floorRadius - 5, floorRadius - 1, 0, 360, 360, BLACK);

        if (config.editor.showAngleValues)
        {
            for (int i = 0; i < 360; i += 5)
            {
                float angleRad = i * DEG2RAD;
                Vector2 pos = (Vector2){
                    (floorRadius + 15) * sinf(angleRad),
                    -(floorRadius + 15) * cosf(angleRad)};
                DrawText(TextFormat("%i", i), pos.x - 10, pos.y - 10, 10, DARKGRAY);
            }
        }

        int pointsDrawn = 0;
        for (int i = 0; i < game.world.floor.totalRocks; i++)
        {
            for (size_t j = 0; j < game.world.floor.pointsPerRock[i]; j++)
            {
                Vector2 p0 = game.world.floor.rockPoints[pointsDrawn + j];
                Vector2 p1 = game.world.floor.rockPoints[pointsDrawn + (j + 1) % game.world.floor.pointsPerRock[i]];
                DrawLineEx(p0, p1, 1, GRAY);
                DrawCircleV(p0, 0.4f, GRAY);
            }

            // Close the polygon with circle at last vertex too (from j loop it already draws p0 for last edge)
            pointsDrawn += game.world.floor.pointsPerRock[i];
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
