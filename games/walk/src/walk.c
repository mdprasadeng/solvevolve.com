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
    float dpi;
    Camera2D camera;
    Camera2D worldCamera;
    float zoomAtRest;
    float zoomWhileMoving;

    EaseType zoomEaseType;
    float zoomStart;
    float zoomEnd;
    float zoomTime;
    float elapsedZoomTime;

} Display;

#pragma endregion

#pragma region Config-Data

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

#pragma endregion

typedef struct Game
{
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
    width = 1000 * 1.4f;
    height = 450 * 1.4f;
    dpi = 2.4;
#endif

    srand(time(NULL));
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(width, height, "Walk");
    InitGame(&game, width, height, dpi);

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
    // Generate rocks
    Image rocksImage;
    
    float angleInDegreeCoveredByTex = 30.0f;

    float outRad = game->world.floor.radius;
    float inRad = game->world.floor.radiusSeenAtRest / 4;
    int texWidth = outRad * tanf(angleInDegreeCoveredByTex * DEG2RAD * 0.5f) * 2;
    int texHeight = (outRad - inRad);
    int offset = 0;
    offset = (outRad - inRad) * tanf(angleInDegreeCoveredByTex * DEG2RAD * 0.5f);

    rocksImage = GenTilableRocks(texWidth, texHeight, game->world.floor.radius, game->world.floor.radiusSeenAtRest, game->display.pixelsPerUnit);
    game->world.floorTexture = LoadTextureFromImage(rocksImage);
    game->world.floorTextureOffset = offset;
    game->world.floorTextureAngle = angleInDegreeCoveredByTex;

    UnloadImage(rocksImage);
    TraceLog(LOG_INFO, "Out rad %f, In rad %f, tex width %d, tex height %d offset %d", outRad, inRad, texWidth, texHeight, offset);
}

void FreeGame(Game *game)
{
    World world = game->world;
    free(world.trees);
    free(world.clouds);
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

        game->display.pixelsPerUnit = height / game->display.cHeightUnits;
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

        game->config.world.treeCount = 20;
        game->config.world.treeWidth[0] = 10;
        game->config.world.treeWidth[1] = 30;
        game->config.world.treeHeight[0] = 4 * game->display.pixelsPerUnit;
        game->config.world.treeHeight[1] = 20 * game->display.pixelsPerUnit;
        game->config.world.cloudCount = 50;
        game->config.world.cloudWidth[0] = game->display.pixelsPerUnit * 3;
        game->config.world.cloudWidth[1] = game->display.pixelsPerUnit * 8;
        game->config.world.cloudHeight[0] = game->display.pixelsPerUnit / 2;
        game->config.world.cloudHeight[1] = 2 * game->display.pixelsPerUnit;
        game->config.world.cloudFloatingHeight[0] = 6 * game->display.pixelsPerUnit;
        game->config.world.cloudFloatingHeight[1] = 25 * game->display.pixelsPerUnit;
    }

    game->config.player.width = game->display.pixelsPerUnit * 2.0f;
    game->config.player.height = game->display.pixelsPerUnit * 4.0f;

    // Editor
    {
        game->config.editor.showDemoWindow = false;
        game->config.editor.showAngleValues = true;
        game->config.editor.showRadialLines = false;
    }
    game->config.gameplay.timeForFullRotation = 120.0f;
    game->config.gameplay.speedInDegreesPerSecond = 360.0f / game->config.gameplay.timeForFullRotation;
    game->config.controls.maxDurationForQuickTap = 20.0f / 60.f; // 20 frames in 60 frames
}

void InitGame(Game *game, int width, int height, float dpi)
{

    InitConfig(game, width, height, dpi);

    // Init ViewLines
    {
        float floorRadius = game->config.world.floorRadius;
        game->world.viewlineCount = 360;
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
        game->world.floor.gapSeenAtRest = game->world.floor.radius - game->world.floor.radiusSeenAtRest;

        TraceLog(LOG_INFO, "Radius seen %f, %f, %f", game->world.floor.radius, game->world.floor.radiusSeenAtRest, game->world.floor.radius - game->world.floor.radiusSeenAtRest);
    }

    GenerateWorld(game);
}

void UpdateDrawFrame(void)
{
    float deltaTime = GetFrameTime();
    if (IsKeyPressed(KEY_Z))
    {
        Vector2 worldCenterAt = GetWorldToScreen2D((Vector2){0, 0}, game.display.camera);
        Vector2 worldPointAt = GetWorldToScreen2D((Vector2){game.world.floor.radius, 0}, game.display.camera);
        TraceLog(LOG_INFO, "World Radius %f", Vector2Distance(worldPointAt, worldCenterAt));
    }
    if (IsKeyReleased(KEY_R))
    {
        GenerateWorld(&game);
    }

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

    float floorRadius = config.world.floorRadius;

    // Update Camera
    {
        float currentCameraToAngle = game.player.atAngle;
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

        float floorRadius = game.world.floor.radius;
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

        static float offset = 25.0f;
        if (IsKeyDown(KEY_UP))
        {
            offset += 0.1f;
        }
        if (IsKeyDown(KEY_DOWN))
        {
            offset -= 0.1f;
        }

        float deltaAngleInDegree = game.world.floorTextureAngle;
        float deltaAngleInRad = deltaAngleInDegree * DEG2RAD;
        
        for (int i = 0; i * deltaAngleInRad < 2 * PI; i+=1)
        {
            
            float topMiddleX = floorRadius * sinf(i*deltaAngleInRad);
            float topMiddleY = -floorRadius * cosf(i*deltaAngleInRad);

            float drawX = topMiddleX - game.world.floorTexture.width * cosf(i*deltaAngleInRad) / 2;
            float drawY = topMiddleY - game.world.floorTexture.width * sinf(i*deltaAngleInRad) / 2;

            float scale = 1.0f;

            Vector2 position = {drawX, drawY};
            Rectangle source = {0.0f, 0.0f, (float)game.world.floorTexture.width, (float)game.world.floorTexture.height};
            Rectangle dest = {position.x, position.y, (float)game.world.floorTexture.width * scale, (float)game.world.floorTexture.height * scale};
            Vector2 origin = {0, 0};

            DrawTextureInRing(game.world.floorTexture, source, dest, origin, i*deltaAngleInRad*RAD2DEG, WHITE, game.world.floorTextureOffset);
            
            
            DrawCircle(topMiddleX, topMiddleY, 50, PINK);
            DrawCircle(drawX, drawY, 50, GREEN);

        }

        // DrawTextureEx(game.world.floorTexture, (Vector2){0, 0}, 0, 1.0f, WHITE);1
        //   Draw Floor
        DrawRing((Vector2){0, 0}, floorRadius - 10, floorRadius, 0, 360, 360, BROWN);

        // DrawCircleSector((Vector2){0, 0}, game.world.floor.radiusSeenAtRest, 0, 360, 360, WHITE);

        if (config.editor.showAngleValues)
        {
            for (int i = 0; i < 360; i += 5)
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
        if (config.editor.showRadialLines)
        {
            for (int i = 0; i < game.world.viewlineCount; i++)
            {
                DrawLineV((Vector2){0, 0}, game.world.earthViewlineStarts[i], GREEN);
                DrawLineV(game.world.skyViewlineStarts[i], game.world.skyViewlineEnds[i], RED);
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

            float bottomLeftAngle = floorf((Vector2AnglePositiveDegree((Vector2){0, -1}, bottomLeft)) * (game.world.viewlineCount / 360.0f));
            float bottomRightAngle = ceilf((Vector2AnglePositiveDegree((Vector2){0, -1}, bottomRight)) * (game.world.viewlineCount / 360.0f));
            Vector2 collisionPoint;

            int i = bottomLeftAngle;
            while (true)
            {
                if (IsKeyPressed(KEY_D))
                {
                    TraceLog(LOG_INFO, "Checking %d in (%f, %f)", i, bottomLeftAngle, bottomRightAngle);
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
