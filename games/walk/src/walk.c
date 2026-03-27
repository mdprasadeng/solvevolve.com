#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "rlImGui.h" // include the API header
#include "dcimgui.h"

Color toColor(float col[4])
{
    Color color;
    color.r = (unsigned char)(col[0] * 255.0f);
    color.g = (unsigned char)(col[1] * 255.0f);
    color.b = (unsigned char)(col[2] * 255.0f);
    color.a = (unsigned char)(col[3] * 255.0f);
    return color;
};

float randomFloat(float min, float max)
{
    return min + (max - min) * ((float)rand() / RAND_MAX);
}

// State

typedef struct GlobalState
{
    int screenWidth;
    int screenHeight;
} GlobalState;

typedef struct Player
{
    float width;
    float height;
    float atAngle;
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

typedef struct World
{
    float radius;
    Player player;
    Tree *trees;
    int treesCount;
    Cloud *clouds;
    int cloudsCount;
} World;

// Generators
void GenerateTrees(World *world, int treeCount, float treeWidth[2], float treeHeight[2])
{
    if (world->trees != NULL)
    {
        free(world->trees);
    }
    world->treesCount = treeCount;
    world->trees = (Tree *)malloc(world->treesCount * sizeof(Tree));
    for (int i = 0; i < world->treesCount; i++)
    {
        world->trees[i].width = randomFloat(treeWidth[0], treeWidth[1]);
        world->trees[i].height = randomFloat(treeHeight[0], treeHeight[1]);
        world->trees[i].atAngle = (2 * PI) / world->treesCount * i;
        world->trees[i].treeType = rand() % 3; // Assuming 3 types of trees
    }
}

void GenerateClouds(World *world, int cloudCount, float cloudWidth[2], float cloudHeight[2], float cloudFloatingHeight[2])
{
    if (world->clouds != NULL)
    {
        free(world->clouds);
    }
    world->cloudsCount = cloudCount;
    world->clouds = (Cloud *)malloc(world->cloudsCount * sizeof(Cloud));
    for (int i = 0; i < world->cloudsCount; i++)
    {
        world->clouds[i].width = randomFloat(cloudWidth[0], cloudWidth[1]);
        world->clouds[i].height = randomFloat(cloudHeight[0], cloudHeight[1]);
        world->clouds[i].floatingHeight = randomFloat(cloudFloatingHeight[0], cloudFloatingHeight[1]);
        world->clouds[i].atAngle = (2 * PI) / world->cloudsCount * i;
        world->clouds[i].cloudType = rand() % 3; // Assuming 3 types of clouds
    }
}

void DrawRectangleLinesPro(Rectangle rect, Vector2 origin, float rotation, Color color, int lineThick)
{

    Vector2 p1 = (Vector2){rect.x, rect.y};
    Vector2 p2 = (Vector2){rect.x + rect.width, rect.y};
    Vector2 p3 = (Vector2){rect.x + rect.width, rect.y + rect.height};
    Vector2 p4 = (Vector2){rect.x, rect.y + rect.height};

    p1 = Vector2Rotate((Vector2){p1.x - rect.x - origin.x, p1.y - rect.y - origin.y}, rotation * DEG2RAD);
    p2 = Vector2Rotate((Vector2){p2.x - rect.x - rect.width - origin.x, p2.y - rect.y - origin.y}, rotation * DEG2RAD);
    p3 = Vector2Rotate((Vector2){p3.x - rect.x - rect.width - origin.x, p3.y - rect.y - rect.height - origin.y}, rotation * DEG2RAD);
    p4 = Vector2Rotate((Vector2){p4.x - rect.x - origin.x, p4.y - rect.y - rect.height - origin.y}, rotation * DEG2RAD);

    // use DrawLineEx on transformed points
    DrawLineEx(p1, p2, lineThick, color);
    DrawLineEx(p2, p3, lineThick, color);
    DrawLineEx(p3, p4, lineThick, color);
    DrawLineEx(p4, p1, lineThick, color);
}

int main(void)
{

    srand(time(NULL));

    GlobalState globalState;
    World world;

#pragma region Initialization
    {
        globalState.screenWidth = 1024;
        globalState.screenHeight = 1024;
        world.radius = 400;
        world.player.width = 20;
        world.player.height = 30;
        world.trees = NULL; // Initialize pointers to NULL
        world.clouds = NULL;
        GenerateTrees(&world, 20, (float[2]){10, 20}, (float[2]){50, 150});
        GenerateClouds(&world, 13, (float[2]){50, 100}, (float[2]){20, 50}, (float[2]){60, 120});
    }
#pragma endregion

#pragma region ImGui Variables

    bool showDemo = false;
    float IM_TREE_WIDTH[2] = {10, 20};
    float IM_TREE_HEIGHT[2] = {50, 150};
    float IM_CLOUD_WIDTH[2] = {50, 100};
    float IM_CLOUD_HEIGHT[2] = {20, 50};
    float IM_CLOUD_FLOATING_HEIGHT[2] = {200, 400};

#pragma endregion

#pragma region Camera Variables
    Camera2D playerCamera = {0};
    playerCamera.target = (Vector2){0, 0};
    playerCamera.offset = (Vector2){globalState.screenWidth / 2.0f, globalState.screenHeight * 0.6f};
    playerCamera.rotation = 90;
    playerCamera.zoom = 5.0f;

    Camera2D worldCamera = {0};
    worldCamera.target = (Vector2){globalState.screenWidth / 2.0f, globalState.screenHeight / 2.0f};
    worldCamera.offset = (Vector2){globalState.screenWidth / 2.0f, globalState.screenHeight / 2.0f};
    worldCamera.rotation = 0;
    worldCamera.zoom = 0.5f;

    Camera2D *activeCamera = &playerCamera;
    bool usingPlayerCamera = true;

    float targetCameraLerpSpeed = 0.003f;
    float targetCameraYOffsetMax = (globalState.screenHeight * 0.32f);
    float currentCameraYOffset = 0;
    float targetCameraYOffset = 0;
#pragma endregion

    InitWindow(globalState.screenWidth, globalState.screenHeight, "Walk");
    rlImGuiSetup(false); // sets up ImGui with ether a dark or light default theme

    while (!WindowShouldClose())
    {
        BeginDrawing();
        rlImGuiBegin(); // starts the ImGui content mode. Make all ImGui calls after this
        ClearBackground(WHITE);

#pragma region ImGui in HUD

        ImGui_ShowDemoWindow(&showDemo);

        ImGui_Begin("Render", NULL, 0);
        ImGui_InputFloat("Radius", &world.radius);
        bool regenerateTrees = false;
        regenerateTrees |= ImGui_InputInt("Tree Count", &world.treesCount);
        regenerateTrees |= ImGui_InputFloat2("Tree Width Range", IM_TREE_WIDTH);
        regenerateTrees |= ImGui_InputFloat2("Tree Height Range", IM_TREE_HEIGHT);
        if (regenerateTrees)
        {
            GenerateTrees(&world, world.treesCount, IM_TREE_WIDTH, IM_TREE_HEIGHT);
        }

        bool regenerateClouds = false;
        regenerateClouds |= ImGui_InputInt("Cloud Count", &world.cloudsCount);
        regenerateClouds |= ImGui_InputFloat2("Cloud Width Range", IM_CLOUD_WIDTH);
        regenerateClouds |= ImGui_InputFloat2("Cloud Height Range", IM_CLOUD_HEIGHT);
        regenerateClouds |= ImGui_InputFloat2("Cloud Floating Height Range", IM_CLOUD_FLOATING_HEIGHT);
        if (regenerateClouds)
        {
            GenerateClouds(&world, world.cloudsCount, IM_CLOUD_WIDTH, IM_CLOUD_HEIGHT, IM_CLOUD_FLOATING_HEIGHT);
        }

        ImGui_End();

#pragma endregion

        int cX = globalState.screenWidth / 2;
        int cY = globalState.screenHeight / 2;

#pragma region Render World

        BeginMode2D(*activeCamera);
        // Draw Floor
        DrawRing((Vector2){cX, cY}, world.radius - 3, world.radius, 0, 360, 360, BLACK);

        // Draw Trees
        for (int i = 0; i < world.treesCount; i++)
        {
            Tree *trees = world.trees;
            float treeX = cX - world.radius * cosf(trees[i].atAngle);
            float treeY = cY + world.radius * sinf(trees[i].atAngle);

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
            float radius = world.radius + clouds[i].floatingHeight;
            float cloudX = cX - radius * cosf(clouds[i].atAngle);
            float cloudY = cY + radius * sinf(clouds[i].atAngle);

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
                    cX - world.radius * cosf(world.player.atAngle),
                    cY + world.radius * sinf(world.player.atAngle),
                    -world.player.width, -world.player.height},
                (Vector2){-world.player.width / 2, 0},
                ((-PI / 2) - world.player.atAngle) * RAD2DEG,
                BLACK);
        }

        EndMode2D();
#pragma endregion

#pragma region Process Input

        if (IsKeyDown(KEY_RIGHT))
            world.player.atAngle -= 0.0005f;
        if (IsKeyDown(KEY_LEFT))
            world.player.atAngle += 0.0005f;

        if (IsKeyPressed(KEY_TAB))
        {
            usingPlayerCamera = !usingPlayerCamera;
            activeCamera = usingPlayerCamera ? &playerCamera : &worldCamera;
        }

        playerCamera.target = (Vector2){
            cX - world.radius * cosf(world.player.atAngle),
            cY + world.radius * sinf(world.player.atAngle)};
        playerCamera.rotation = 90 + (world.player.atAngle * RAD2DEG);

        if (IsKeyDown(KEY_DOWN))
        {
            targetCameraYOffset = -targetCameraYOffsetMax;
            currentCameraYOffset = Lerp(currentCameraYOffset, -targetCameraYOffsetMax, targetCameraLerpSpeed);
        }
        else if (IsKeyDown(KEY_UP))
        {
            targetCameraYOffset = targetCameraYOffsetMax;
            currentCameraYOffset = Lerp(currentCameraYOffset, targetCameraYOffsetMax, targetCameraLerpSpeed);
        }
        else
        {
            targetCameraYOffset = 0;
        }

        currentCameraYOffset = Lerp(currentCameraYOffset, targetCameraYOffset, targetCameraLerpSpeed);
        playerCamera.offset.y = globalState.screenHeight * 0.6f + currentCameraYOffset;

#pragma endregion

        rlImGuiEnd(); // ends the ImGui content mode. Make all ImGui calls before this
        EndDrawing();
    }

    rlImGuiShutdown(); // cleans up ImGui
    CloseWindow();

    // Cleanup
    if (world.trees != NULL)
        free(world.trees);
    if (world.clouds != NULL)
        free(world.clouds);

    return 0;
}
