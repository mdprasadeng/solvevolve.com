#include "raylib.h"
#include "raymath.h"
#include "reasings.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "helper.h"

#define PLATFORM_DESKTOP 1

#ifdef PLATFORM_DESKTOP
#include "rlImGui.h" // include the API header
#include "dcimgui.h"
#endif

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

#pragma region Game Definitions

typedef struct Player
{
    float width;
    float height;
    float atAngle;
    bool isMoving;
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
    int totalRocks;
    int *pointsPerRock;
    float *boundingRadiusPerRock;
    Vector2 *rockCenters;
    Vector2 *rockPoints;
} Floor;

typedef struct World
{
    Player player;
    Floor floor;
    Tree *trees;
    int treesCount;
    Cloud *clouds;
    int cloudsCount;
} World;

typedef struct WorldParams
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
} WorldParams;

typedef enum
{
    EASE_LINEAR=0,
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


typedef struct CameraParams
{
    float angleShowAtRest;
    float angleShowWhileMoving;
    float angleOffPlayerAtRest;
    float angleOffPlayerWhileMoving;
    EaseType restToMoveEase;
    EaseType moveToRestEase;
    float restToMoveDuration;
    float moveToRestDuration;

} CameraParams;

typedef struct GameplayParams
{
    float moveBySpeed;
    float cameraLerpSpeed;
    float targetCameraYOffsetMax;
    float arcToShowAtRest;
    float arcToShowWhileMoving;
    float frameCounter;
    float frameDuratinForZoomIn;
    float frameDuratinForZoomOut;
} GameplayParams;

typedef struct DebugParams
{
    bool showDemoWindow;
    bool showAngleValues;
    float arcToShow;
} DebugParams;

#pragma endregion

#pragma region Game Generation Functions

void InitWorldParams(WorldParams *params)
{
    params->floorRadius = 900;
    params->treeCount = 20;
    params->treeWidth[0] = 10;
    params->treeWidth[1] = 30;
    params->treeHeight[0] = 50;
    params->treeHeight[1] = 150;
    params->cloudCount = 50;
    params->cloudWidth[0] = 50;
    params->cloudWidth[1] = 150;
    params->cloudHeight[0] = 20;
    params->cloudHeight[1] = 60;
    params->cloudFloatingHeight[0] = 50;
    params->cloudFloatingHeight[1] = 150;
    params->totalRocks = 60;
    params->rockPointsRange[0] = 5;
    params->rockPointsRange[1] = 15;
    params->rockRadiusRange[0] = 5;
    params->rockRadiusRange[1] = 20;
}

void GenerateTrees(World *world, WorldParams params)
{
    world->treesCount = params.treeCount;
    world->trees = (Tree *)malloc(world->treesCount * sizeof(Tree));
    for (int i = 0; i < world->treesCount; i++)
    {
        world->trees[i].width = randomFloat(params.treeWidth[0], params.treeWidth[1]);
        world->trees[i].height = randomFloat(params.treeHeight[0], params.treeHeight[1]);
        world->trees[i].atAngle = (2 * PI) / world->treesCount * i;
        world->trees[i].treeType = rand() % 3; // Assuming 3 types of trees
    }
}

void GenerateClouds(World *world, WorldParams params)
{
    world->cloudsCount = params.cloudCount;
    world->clouds = (Cloud *)malloc(world->cloudsCount * sizeof(Cloud));
    for (int i = 0; i < world->cloudsCount; i++)
    {
        world->clouds[i].width = randomFloat(params.cloudWidth[0], params.cloudWidth[1]);
        world->clouds[i].height = randomFloat(params.cloudHeight[0], params.cloudHeight[1]);
        world->clouds[i].floatingHeight = randomFloat(params.cloudFloatingHeight[0], params.cloudFloatingHeight[1]);
        world->clouds[i].atAngle = (2 * PI) / world->cloudsCount * i;
        world->clouds[i].cloudType = rand() % 3; // Assuming 3 types of clouds
    }
}

void GenerateRocks(World *world, WorldParams params)
{
    world->floor.totalRocks = params.treeCount;
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
        float rockAtRadius = world->floor.radius - world->floor.boundingRadiusPerRock[i] - 10; // 10 is just an extra offset to make sure rocks dont intersect with the world border
        rockAtRadius = rockAtRadius * randomFloat(0.9f, 1.0f);                                 // add some noise to the radius of the rock to make it look more natural
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

void GenerateWorld(World *world, WorldParams params)
{
    world->player.width = 20;
    world->player.height = 40;
    world->player.atAngle = 0;
    world->player.isMoving = false;

    world->floor.radius = params.floorRadius;
    GenerateTrees(world, params);
    GenerateClouds(world, params);
    GenerateRocks(world, params);
}

void freeWorld(World *world)
{
    free(world->floor.rockPoints);
    free(world->floor.pointsPerRock);
    free(world->floor.boundingRadiusPerRock);
    free(world->trees);
    free(world->clouds);
}

#pragma endregion

#pragma region Hud Definitions

typedef struct Screen
{
    int width;
    int height;
} Screen;

typedef struct MyCamera
{
    Camera2D playerCamera;
    Camera2D worldCamera;
    Camera2D *activeCamera;
    float currentCameraYOffset;
    float targetCameraYOffset;
    float targetZoom;
    float startZoom;
} MyCamera;

#pragma endregion

World world = {0};
WorldParams params = {0};
Screen screen = {0};
MyCamera myCamera = {
    .playerCamera = {.zoom = 2.0f},
    .worldCamera = {.zoom = 0.9f},
};



GameplayParams gameplayParams = {
    .cameraLerpSpeed = 0.1f,
    .targetCameraYOffsetMax = 50.0f,
    .moveBySpeed = 2 * PI / (120 * 60),
    .arcToShowAtRest = 60.0f,
    .arcToShowWhileMoving = 20.0f,
    .frameCounter = 0,
    .frameDuratinForZoomIn = 60.0f,
    .frameDuratinForZoomOut = 6 * 60.0f};

DebugParams debugParams = {
    .showDemoWindow = false,
    .showAngleValues = true,
    .arcToShow = 10,
};

void UpdateCameraZoom()
{

    if (myCamera.startZoom < myCamera.targetZoom)
    {
        if (gameplayParams.frameCounter <= gameplayParams.frameDuratinForZoomIn)
        {
            myCamera.playerCamera.zoom = EaseCubicOut(gameplayParams.frameCounter, myCamera.startZoom, myCamera.targetZoom - myCamera.startZoom, gameplayParams.frameDuratinForZoomIn);
        }
        else
        {
            myCamera.playerCamera.zoom = myCamera.targetZoom;
        }
    }
    else
    {
        if (gameplayParams.frameCounter <= gameplayParams.frameDuratinForZoomOut)
        {
            myCamera.playerCamera.zoom = EaseSineIn(gameplayParams.frameCounter, myCamera.startZoom, myCamera.targetZoom - myCamera.startZoom, gameplayParams.frameDuratinForZoomOut);
        }
        else
        {
            myCamera.playerCamera.zoom = myCamera.targetZoom;
        }
    }
}
void StartZooming()
{
    TraceLog(LOG_INFO, "StartZooming called. player is moving: %i", world.player.isMoving);
    float targetArc = world.player.isMoving ? gameplayParams.arcToShowWhileMoving : gameplayParams.arcToShowAtRest;
    myCamera.startZoom = myCamera.playerCamera.zoom;
    myCamera.targetZoom = screen.width / (world.floor.radius * cosf(DEG2RAD * (90 - targetArc / 2)) * 2);
    gameplayParams.frameCounter = 0;
}

void UpdateDrawFrame(void)
{

    gameplayParams.frameCounter += 1;
    UpdateCameraZoom();
    {
        BeginDrawing();

#ifdef PLATFORM_DESKTOP
        rlImGuiBegin(); // starts the ImGui content mode. Make all ImGui calls after this
#endif
        ClearBackground(WHITE);

#pragma region ImGui in HUD
        // #ifdef PLATFORM_DESKTOP
        ImGui_ShowDemoWindow(&debugParams.showDemoWindow);

        ImGui_Begin("Render", NULL, 0);
        ImGui_Text("FPS: %i", GetFPS());

        ImGui_Checkbox("Show Demo Window", &debugParams.showDemoWindow);
        ImGui_Checkbox("Show Angle Values", &debugParams.showAngleValues);
        ImGui_SliderFloat("Camera Lerp Speed", &gameplayParams.cameraLerpSpeed, 0.01f, 0.1f);

        int arcChanged = ImGui_SliderFloat("Arc To Show", &debugParams.arcToShow, 1.0f, 60.0f);
        if (arcChanged)
        {
            float angle = DEG2RAD * (90 - debugParams.arcToShow / 2.0f);
            myCamera.playerCamera.zoom = screen.width / (world.floor.radius * cosf(angle) * 2);
        }
        ImGui_End();
        // #endif

#pragma endregion

#pragma region Walk Buttons
        int walkButtonWidth = 200;
        int buttonOffset = 20;
        int buttonYOff = screen.height - walkButtonWidth - buttonOffset * 2;
        Color buttonUnpressedColor = GRAY;
        Color buttonPressedColor = BLACK;
        Color currentColor = buttonUnpressedColor;
        int cameraButtonWidth = 180;

        Vector2 leftBtnCenter = (Vector2){walkButtonWidth / 2 + buttonOffset, buttonYOff + walkButtonWidth / 2 + buttonOffset};
        float moveAngleBy = 0;
        if (IsKeyDown(KEY_LEFT) ||
            (CheckCollisionPointCircle(GetMousePosition(), leftBtnCenter, walkButtonWidth / 2) && IsMouseButtonDown(MOUSE_LEFT_BUTTON)))
        {
            moveAngleBy = gameplayParams.moveBySpeed;
            currentColor = buttonPressedColor;
        }
        else
        {
            currentColor = buttonUnpressedColor;
        }
        DrawRing(
            leftBtnCenter,
            walkButtonWidth / 2 - 5, walkButtonWidth / 2, 0, 360, 36, currentColor);
        DrawPoly(leftBtnCenter, 3, walkButtonWidth / 2 - 40, 180, currentColor);

        Vector2 rightBtnCenter = (Vector2){screen.width - walkButtonWidth / 2 - buttonOffset, buttonYOff + walkButtonWidth / 2 + buttonOffset};
        if (IsKeyDown(KEY_RIGHT) ||
            (CheckCollisionPointCircle(GetMousePosition(), rightBtnCenter, walkButtonWidth / 2) && IsMouseButtonDown(MOUSE_LEFT_BUTTON)))
        {
            moveAngleBy = -gameplayParams.moveBySpeed;
            currentColor = buttonPressedColor;
        }
        else
        {
            currentColor = buttonUnpressedColor;
        }
        DrawRing(
            rightBtnCenter,
            walkButtonWidth / 2 - 5, walkButtonWidth / 2, 0, 360, 36, currentColor);
        DrawPoly(rightBtnCenter, 3, walkButtonWidth / 2 - 40, 0, currentColor);

        bool wasMoving = world.player.isMoving;

        world.player.atAngle += moveAngleBy;
        world.player.isMoving = moveAngleBy != 0;

        if (wasMoving && moveAngleBy == 0)
        {
            StartZooming();
        }
        if (!wasMoving && moveAngleBy != 0)
        {
            StartZooming();
        }

#pragma endregion
#pragma region Camera Buttons
        if (!world.player.isMoving)
        {
            int lineThick = 5;
            int ringInner = walkButtonWidth / 2 + 8;
            int ringOuter = ringInner + lineThick;
            float ringCenter = walkButtonWidth / 2 + 10;

            Vector2 lUpTopLeft = (Vector2){
                leftBtnCenter.x + ringCenter * cosf(DEG2RAD * -60),
                leftBtnCenter.y + ringCenter * sinf(DEG2RAD * -60)};
            Vector2 lUpBtmLeft = (Vector2){
                leftBtnCenter.x + ringCenter * cosf(DEG2RAD * -5),
                leftBtnCenter.y + ringCenter * sinf(DEG2RAD * -5)};
            Vector2 lUpBtmRight = (Vector2){
                leftBtnCenter.x + ringCenter * cosf(DEG2RAD * -5) + cameraButtonWidth,
                leftBtnCenter.y + ringCenter * sinf(DEG2RAD * -5)};
            Vector2 lUpTopRight = (Vector2){
                leftBtnCenter.x + ringCenter * cosf(DEG2RAD * -5) + cameraButtonWidth,
                leftBtnCenter.y + ringCenter * sinf(DEG2RAD * -60)};
            Vector2 lUpCenter = Vector2Scale(Vector2Add(lUpBtmLeft, lUpTopRight), 0.5f);
            Vector2 lUpPoints[4] = {lUpBtmLeft, lUpBtmRight, lUpTopRight, lUpTopLeft};

            Vector2 lDownTopLeft = (Vector2){
                leftBtnCenter.x + ringCenter * cosf(DEG2RAD * 60),
                leftBtnCenter.y + ringCenter * sinf(DEG2RAD * 60)};
            Vector2 lDownBtmLeft = (Vector2){
                leftBtnCenter.x + ringCenter * cosf(DEG2RAD * 5),
                leftBtnCenter.y + ringCenter * sinf(DEG2RAD * 5)};
            Vector2 lDownBtmRight = (Vector2){
                leftBtnCenter.x + ringCenter * cosf(DEG2RAD * 5) + cameraButtonWidth,
                leftBtnCenter.y + ringCenter * sinf(DEG2RAD * 5)};
            Vector2 lDownTopRight = (Vector2){
                leftBtnCenter.x + ringCenter * cosf(DEG2RAD * 5) + cameraButtonWidth,
                leftBtnCenter.y + ringCenter * sinf(DEG2RAD * 60)};
            Vector2 lDownCenter = Vector2Scale(Vector2Add(lDownBtmLeft, lDownTopRight), 0.5f);
            Vector2 lDownPoints[4] = {lDownBtmLeft, lDownBtmRight, lDownTopRight, lDownTopLeft};

            Vector2 rUpTopLeft = (Vector2){
                rightBtnCenter.x + ringCenter * cosf(DEG2RAD * (60 + 180)),
                rightBtnCenter.y + ringCenter * sinf(DEG2RAD * (60 + 180))};
            Vector2 rUpBtmLeft = (Vector2){
                rightBtnCenter.x + ringCenter * cosf(DEG2RAD * (5 + 180)),
                rightBtnCenter.y + ringCenter * sinf(DEG2RAD * (5 + 180))};
            Vector2 rUpBtmRight = (Vector2){
                rightBtnCenter.x + ringCenter * cosf(DEG2RAD * (5 + 180)) - cameraButtonWidth,
                rightBtnCenter.y + ringCenter * sinf(DEG2RAD * (5 + 180))};
            Vector2 rUpTopRight = (Vector2){
                rightBtnCenter.x + ringCenter * cosf(DEG2RAD * (5 + 180)) - cameraButtonWidth,
                rightBtnCenter.y + ringCenter * sinf(DEG2RAD * (60 + 180))};
            Vector2 rUpCenter = Vector2Scale(Vector2Add(rUpBtmLeft, rUpTopRight), 0.5f);
            Vector2 rUpPoints[4] = {rUpBtmLeft, rUpBtmRight, rUpTopRight, rUpTopLeft};

            Vector2 rDownTopLeft = (Vector2){
                rightBtnCenter.x + ringCenter * cosf(DEG2RAD * (-60 + 180)),
                rightBtnCenter.y + ringCenter * sinf(DEG2RAD * (-60 + 180))};
            Vector2 rDownBtmLeft = (Vector2){
                rightBtnCenter.x + ringCenter * cosf(DEG2RAD * (-5 + 180)),
                rightBtnCenter.y + ringCenter * sinf(DEG2RAD * (-5 + 180))};
            Vector2 rDownBtmRight = (Vector2){
                rightBtnCenter.x + ringCenter * cosf(DEG2RAD * (-5 + 180)) - cameraButtonWidth,
                rightBtnCenter.y + ringCenter * sinf(DEG2RAD * (-5 + 180))};
            Vector2 rDownTopRight = (Vector2){
                rightBtnCenter.x + ringCenter * cosf(DEG2RAD * (-5 + 180)) - cameraButtonWidth,
                rightBtnCenter.y + ringCenter * sinf(DEG2RAD * (-60 + 180))};
            Vector2 rDownCenter = Vector2Scale(Vector2Add(rDownBtmLeft, rDownTopRight), 0.5f);
            Vector2 rDownPoints[4] = {rDownBtmLeft, rDownBtmRight, rDownTopRight, rDownTopLeft};

            Color upButtonColor = buttonUnpressedColor;
            Color downButtonColor = buttonUnpressedColor;
            if (IsKeyDown(KEY_DOWN) ||
                (CheckCollisionPointPoly(GetMousePosition(), rDownPoints, 4) && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) ||
                (CheckCollisionPointPoly(GetMousePosition(), lDownPoints, 4) && IsMouseButtonDown(MOUSE_LEFT_BUTTON)))
            {
                myCamera.targetCameraYOffset = -gameplayParams.targetCameraYOffsetMax;
                downButtonColor = buttonPressedColor;
            }
            else if (IsKeyDown(KEY_UP) ||
                     (CheckCollisionPointPoly(GetMousePosition(), rUpPoints, 4) && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) ||
                     (CheckCollisionPointPoly(GetMousePosition(), lUpPoints, 4) && IsMouseButtonDown(MOUSE_LEFT_BUTTON)))
            {
                myCamera.targetCameraYOffset = gameplayParams.targetCameraYOffsetMax;
                upButtonColor = buttonPressedColor;
            }
            else
            {
                myCamera.targetCameraYOffset = 0;
            }

            DrawRing((Vector2){leftBtnCenter.x, leftBtnCenter.y}, ringInner, ringOuter, -60, -5, 12, upButtonColor);
            DrawLineEx(lUpBtmLeft, lUpBtmRight, 5, upButtonColor);
            DrawLineEx(lUpBtmRight, lUpTopRight, 5, upButtonColor);
            DrawLineEx(lUpTopRight, lUpTopLeft, 5, upButtonColor);
            DrawPoly(lUpCenter, 3, 30, -90, upButtonColor);

            DrawRing((Vector2){leftBtnCenter.x, leftBtnCenter.y}, ringInner, ringOuter, 5, 60, 12, downButtonColor);
            DrawLineEx(lDownBtmLeft, lDownBtmRight, 5, downButtonColor);
            DrawLineEx(lDownBtmRight, lDownTopRight, 5, downButtonColor);
            DrawLineEx(lDownTopRight, lDownTopLeft, 5, downButtonColor);
            DrawPoly(lDownCenter, 3, 30, 90, downButtonColor);

            DrawRing((Vector2){rightBtnCenter.x, rightBtnCenter.y}, ringInner, ringOuter, 5 + 180, 60 + 180, 12, upButtonColor);
            DrawLineEx(rUpBtmLeft, rUpBtmRight, 5, upButtonColor);
            DrawLineEx(rUpBtmRight, rUpTopRight, 5, upButtonColor);
            DrawLineEx(rUpTopRight, rUpTopLeft, 5, upButtonColor);
            DrawPoly(rUpCenter, 3, 30, -90, upButtonColor);

            DrawRing((Vector2){rightBtnCenter.x, rightBtnCenter.y}, ringInner, ringOuter, -5 + 180, -60 + 180, 12, downButtonColor);
            DrawLineEx(rDownBtmLeft, rDownBtmRight, 5, downButtonColor);
            DrawLineEx(rDownBtmRight, rDownTopRight, 5, downButtonColor);
            DrawLineEx(rDownTopRight, rDownTopLeft, 5, downButtonColor);
            DrawPoly(rDownCenter, 3, 30, 90, downButtonColor);
        }
#pragma endregion

        myCamera.playerCamera.target = (Vector2){
            -world.floor.radius * cosf(world.player.atAngle),
            world.floor.radius * sinf(world.player.atAngle)};
        myCamera.playerCamera.rotation = 90 + (world.player.atAngle * RAD2DEG);
        myCamera.currentCameraYOffset = Lerp(myCamera.currentCameraYOffset, myCamera.targetCameraYOffset, gameplayParams.cameraLerpSpeed);
        myCamera.playerCamera.offset.y = screen.height * 0.6f + myCamera.currentCameraYOffset;

#pragma region Render World

        BeginMode2D(*myCamera.activeCamera);
        // Draw Floor
        DrawRing((Vector2){0, 0}, world.floor.radius - 3, world.floor.radius, 0, 360, 360, BLACK);

        if (debugParams.showAngleValues)
        {
            for (int i = 0; i < 360; i += 10)
            {
                float angleRad = i * DEG2RAD;
                Vector2 pos = (Vector2){
                    -(world.floor.radius + 15) * cosf(angleRad),
                    (world.floor.radius + 15) * sinf(angleRad)};
                DrawText(TextFormat("%i", i), pos.x - 10, pos.y - 10, 10, DARKGRAY);
            }
        }

        int pointsDrawn = 0;
        for (int i = 0; i < world.floor.totalRocks; i++)
        {
            for (size_t j = 0; j < world.floor.pointsPerRock[i]; j++)
            {
                Vector2 p0 = world.floor.rockPoints[pointsDrawn + j];
                Vector2 p1 = world.floor.rockPoints[pointsDrawn + (j + 1) % world.floor.pointsPerRock[i]];
                DrawLineEx(p0, p1, 1, GRAY);
                DrawCircleV(p0, 0.4f, GRAY);
            }

            // Close the polygon with circle at last vertex too (from j loop it already draws p0 for last edge)
            pointsDrawn += world.floor.pointsPerRock[i];
        }

        // Draw Trees
        for (int i = 0; i < world.treesCount; i++)
        {
            Tree *trees = world.trees;
            float treeX = -world.floor.radius * cosf(trees[i].atAngle);
            float treeY = world.floor.radius * sinf(trees[i].atAngle);

            // DrawRectanglePro(
            //     (Rectangle){treeX, treeY, -trees[i].width, -trees[i].height},
            //     (Vector2){-trees[i].width / 2, 0},
            //     ((-PI / 2) - trees[i].atAngle) * RAD2DEG,
            //     BLACK);

            DrawLineEx(
                (Vector2){treeX, treeY},
                (Vector2){treeX - trees[i].height * cosf(trees[i].atAngle), treeY + trees[i].height * sinf(trees[i].atAngle)},
                2,
                DARKGREEN);
        }

        // Draw Clouds
        for (int i = 0; i < world.cloudsCount; i++)
        {
            Cloud *clouds = world.clouds;
            float radius = world.floor.radius + clouds[i].floatingHeight;
            float cloudX = -radius * cosf(clouds[i].atAngle);
            float cloudY = radius * sinf(clouds[i].atAngle);

            DrawRectanglePro(
                (Rectangle){cloudX, cloudY, -clouds[i].width, -clouds[i].height},
                (Vector2){-clouds[i].width / 2, 0},
                ((-PI / 2) - clouds[i].atAngle) * RAD2DEG,
                (Color){130, 130, 130, 55});
        }

        // Draw Player
        {
            DrawRectanglePro(
                (Rectangle){
                    -world.floor.radius * cosf(world.player.atAngle),
                    world.floor.radius * sinf(world.player.atAngle),
                    -world.player.width, -world.player.height},
                (Vector2){-world.player.width / 2, 0},
                ((-PI / 2) - world.player.atAngle) * RAD2DEG,
                BLACK);
        }

        EndMode2D();
#pragma endregion

#pragma region Process Input

        if (IsKeyPressed(KEY_TAB))
        {
            myCamera.activeCamera = myCamera.activeCamera == &myCamera.playerCamera ? &myCamera.worldCamera : &myCamera.playerCamera;
        }

#pragma endregion

#ifdef PLATFORM_DESKTOP
        rlImGuiEnd(); // ends the ImGui content mode. Make all ImGui calls before this
#endif
        EndDrawing();
    }
}

void UpdatePortraitFrame(void)
{
    DrawText("Please rotate your device to landscape mode", screen.width / 2 - 150, screen.height / 2, 20, BLACK);
}

int main(void)
{

#ifdef PLATFORM_WEB

    int width = emscripten_run_script_int("window.innerWidth");
    int height = emscripten_run_script_int("window.innerHeight");
#else
    int width = 16 * 90;
    int height = 9 * 90;
#endif

    srand(time(NULL));
    InitWorldParams(&params);
    GenerateWorld(&world, params);

    TraceLog(LOG_INFO, "Screen Width: %i, Screen Height: %i", width, height);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(width, height, "Walk");
    screen.width = GetScreenWidth();
    screen.height = GetScreenHeight();

    myCamera.playerCamera.target = (Vector2){0, 0};
    myCamera.playerCamera.offset = (Vector2){screen.width / 2, screen.height / 2};
    myCamera.targetZoom = screen.width / (world.floor.radius * (PI / 4));
    myCamera.startZoom = myCamera.playerCamera.zoom;

    myCamera.worldCamera.rotation = 0;
    myCamera.worldCamera.target = (Vector2){0, 0};
    myCamera.worldCamera.offset = (Vector2){screen.width / 2, screen.height / 2};
    myCamera.worldCamera.zoom = (screen.height) / (world.floor.radius * 2 + 2 * params.cloudFloatingHeight[1] + 100);

    myCamera.activeCamera = &myCamera.playerCamera;

#ifdef PLATFORM_DESKTOP
    rlImGuiSetup(false); // sets up ImGui with ether a dark or light default theme
#endif

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    SetTargetFPS(60); // Set our game to run at 60 frames-per-second

    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        if (IsWindowResized())
        {
            screen.width = GetScreenWidth();
            screen.height = GetScreenHeight();

            // TODO: Verify this implementation
            myCamera.playerCamera.offset = (Vector2){screen.width / 2, screen.height / 2};
            myCamera.playerCamera.zoom = screen.width / (world.floor.radius * (PI / 4));
            myCamera.worldCamera.offset = (Vector2){screen.width / 2, screen.height / 2};
            myCamera.worldCamera.zoom = (screen.height) / (world.floor.radius * 2 + 2 * params.cloudFloatingHeight[1] + 100);
        }
        UpdateDrawFrame();
    }
#endif

#ifdef PLATFORM_DESKTOP
    rlImGuiShutdown(); // cleans up ImGui
#endif
    CloseWindow();

    freeWorld(&world);
    return 0;
}
