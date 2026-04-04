#include "raylib.h"
#include "raymath.h"
#include "reasings.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "stb_perlin.h"

Image GenImageRocks(int width, int tileSize, int seedOffset, float border)
{
    int height = width;
    Color *pixels = (Color *)RL_MALLOC(width * height * sizeof(Color));

    int seedsPerRow = width / tileSize;
    int seedsPerCol = height / tileSize;
    int seedCount = seedsPerRow * seedsPerCol;

    Vector2 *seeds = (Vector2 *)RL_MALLOC(seedCount * sizeof(Vector2));

    for (int i = 0; i < seedCount; i++)
    {
        int y = (i / seedsPerRow) * tileSize + GetRandomValue(seedOffset, tileSize - seedOffset);
        int x = (i % seedsPerRow) * tileSize + GetRandomValue(seedOffset, tileSize - seedOffset);
        seeds[i] = (Vector2){(float)x, (float)y};
    }

    float radius = width / 2;

    for (int y = 0; y < height; y++)
    {
        int tileY = y / tileSize;

        for (int x = 0; x < width; x++)
        {
            // Inside your x/y loop, before calculating distances:
            float noiseScale = 0.03f;    // Frequency of the "wiggles"
            float noiseIntensity = 3.0f; // Magnitude of the "wiggles" (in pixels)

            // Calculate offsets using Perlin noise
            float offsetX = stb_perlin_noise3((float)x * noiseScale, (float)y * noiseScale, 0, 0, 0, 0) * noiseIntensity;
            float offsetY = stb_perlin_noise3((float)y * noiseScale, (float)x * noiseScale, 1.0f, 0, 0, 0) * noiseIntensity;

            // Use these "warped" coordinates for the distance check
            float warpedX = (float)x + offsetX;
            float warpedY = (float)y + offsetY;

            int tileX = x / tileSize;

            float minDistance = 65536.0f; //(float)strtod("Inf", NULL);
            float secondMinDistance = 65536.0f;

            // Check all adjacent tiles
            for (int i = -1; i < 2; i++)
            {
                if ((tileX + i < 0) || (tileX + i >= seedsPerRow))
                    continue;

                for (int j = -1; j < 2; j++)
                {
                    if ((tileY + j < 0) || (tileY + j >= seedsPerCol))
                        continue;

                    Vector2 neighborSeed = seeds[(tileY + j) * seedsPerRow + tileX + i];

                    float dist = (float)hypot(warpedX - (int)neighborSeed.x, warpedY - (int)neighborSeed.y);
                    float lastMinDistance = minDistance;
                    minDistance = (float)fmin(minDistance, dist);
                    if (minDistance != lastMinDistance)
                    {
                        secondMinDistance = lastMinDistance;
                    }
                    else
                    {
                        secondMinDistance = (float)fmin(secondMinDistance, dist);
                    }
                }
            }

            // This approach seems to give good results at all tile sizes
            int intensity = (int)(minDistance * 256.0f / tileSize);
            if (intensity > 255)
                intensity = 255;

            unsigned char intensityUC = (unsigned char)intensity;
            pixels[y * width + x] = (Color){intensityUC, intensityUC, intensityUC, 255};
            int cx = x - radius;
            int cy = y - radius;
            if (cx * cx + cy * cy > radius * radius)
            {
                pixels[y * width + x] = BLANK;
            }
            else if (secondMinDistance - minDistance < border * 0.1f * minDistance)
            {
                pixels[y * width + x] = WHITE;
            }
            else
            {
                pixels[y * width + x] = BROWN;
            }
        }
    }

    Image image = {
        .data = pixels,
        .width = width,
        .height = height,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};

    // for (int i = 0; i < seedCount; i++)
    // {
    //     Vector2 seed = seeds[i];
    //     int x = seed.x;
    //     int y = seed.y;
    //     ImageDrawCircle(&image, x, y, 3, ColorAlpha(RED, 0.2f));
    // }

    // for (int i = 0; i < seedsPerRow; i++)
    // {
    //     for (int j = 0; j < seedsPerCol; j++)
    //     {
    //         ImageDrawRectangleLines(&image, (Rectangle){
    //                                             i * tileSize + seedOffset,
    //                                             j * tileSize + seedOffset,
    //                                             tileSize - seedOffset,
    //                                             tileSize - seedOffset,
    //                                         },
    //                                 1, ColorAlpha(RED, 0.2f));
    //     }
    // }

    RL_FREE(seeds);

    return image;
}

Image GenImageRocksPlanet(int fullRadius, int tileLength, int tileRadialCount, float tileDegree)
{
    int width = fullRadius * 2;
    int height = fullRadius * 2;
    int rocksFromRadius = fullRadius - tileLength * tileRadialCount;
    Color *pixels = (Color *)RL_MALLOC(width * height * sizeof(Color));

    Image image = {
        .data = pixels,
        .width = width,
        .height = height,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};

    Color faintBlack = ColorAlpha(BLACK, 0.1f);

    int seedCirles = 1;
    int radius = fullRadius;
    while (radius > rocksFromRadius)
    {
        ImageDrawCircleLines(&image, fullRadius, fullRadius, radius, faintBlack);
        radius -= tileLength;
        seedCirles++;
    }
    ImageDrawCircleLines(&image, fullRadius, fullRadius, radius, faintBlack);

    int seedAngles = 0;
    int angle = 0;
    while (angle < 360)
    {

        ImageDrawLineEx(&image,
                        (Vector2){fullRadius + radius * sinf(DEG2RAD * angle), fullRadius - radius * cosf(DEG2RAD * angle)},
                        (Vector2){fullRadius + fullRadius * sinf(DEG2RAD * angle), fullRadius - fullRadius * cosf(DEG2RAD * angle)},
                        1, faintBlack);
        angle += tileDegree;
        seedAngles++;
    }

    int seedCount = seedCirles * seedAngles;
    Vector2 *seeds = (Vector2 *)RL_MALLOC(seedCount * sizeof(Vector2));
    int seedIndex = 0;
    radius = fullRadius;
    int offset = 1;
    while (radius > rocksFromRadius)
    {
        angle = 0;
        while (angle < 360)
        {
            if (radius == fullRadius) {
                offset = 5;
            }

            int seedAtRadius = GetRandomValue(radius - tileLength, radius - offset);
            float seedAtAngle = GetRandomValue(angle * 10, (angle + tileDegree) * 10) / 10.0f;

            int x = fullRadius + seedAtRadius * sinf(seedAtAngle * DEG2RAD);
            int y = fullRadius - seedAtRadius * cosf(seedAtAngle * DEG2RAD);
            seeds[seedIndex] = (Vector2){(float)x, (float)y};
            seedIndex++;

            if (radius == fullRadius)
            {
                int outOfCircleRadius = (fullRadius - seedAtRadius) * 0.6f + fullRadius + 5;
                int x = fullRadius + outOfCircleRadius * sinf(seedAtAngle * DEG2RAD);
                int y = fullRadius - outOfCircleRadius * cosf(seedAtAngle * DEG2RAD);
                seeds[seedIndex] = (Vector2){(float)x, (float)y};
                seedIndex++;
            }

            angle += tileDegree;
        }
        radius -= tileLength;
    }

    for (int i = 0; i < seedCount; i++)
    {
        ImageDrawCircleV(&image, seeds[i], 2, RED);
    }

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            float pointRadius = hypot(x - fullRadius, y - fullRadius);
            if (pointRadius < rocksFromRadius || pointRadius > fullRadius - tileLength/2)
            {
                continue;
            }

            float minDistance = width + height;
            float secondMinDistance = width + height;
            for (int i = 0; i < seedCount; i++)
            {
                float dist = hypot(x - seeds[i].x, y - seeds[i].y);
                float lastMinDistance = minDistance;
                minDistance = (float)fmin(minDistance, dist);
                if (minDistance != lastMinDistance)
                {
                    secondMinDistance = lastMinDistance;
                }
                else
                {
                    secondMinDistance = (float)fmin(secondMinDistance, dist);
                }
            }
            if (!ColorIsEqual(BLANK, pixels[y * width + x]))
            {
                continue;
            }

            if (secondMinDistance - minDistance > 1)
            {
                pixels[y * width + x] = WHITE;
            }
            else
            {
                pixels[y * width + x] = BLACK;
            }
        }
    }

    for (int i = 0; i < seedCount; i++)
    {
        ImageDrawCircleV(&image, seeds[i], 2, RED);
    }

    RL_FREE(seeds);

    return image;
}

int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 900;
    const int screenHeight = 900;

    InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window");

    Image cellular;
    Texture2D texture;

    bool reGenerate = true;

    SetTargetFPS(60); // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    float border = 0.5f;

    // Main game loop
    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        if (IsKeyPressed(KEY_UP))
        {
            border += 0.1f;
            reGenerate = true;
        }

        if (IsKeyPressed(KEY_DOWN))
        {
            border -= 0.1f;
            if (border <= 0.1f)
            {
                border = 0.1f;
            }
            reGenerate = true;
        }

        if (IsKeyPressed(KEY_R) || reGenerate)
        {
            if (!reGenerate)
            {
                UnloadImage(cellular);
                UnloadTexture(texture);
            }
            else
            {
                reGenerate = false;
            }
            TraceLog(LOG_INFO, "Using border %f", border);
            cellular = GenImageRocksPlanet(450, 50, 6, 5);
            texture = LoadTextureFromImage(cellular);
        }

        // Update
        //----------------------------------------------------------------------------------
        // TODO: Update your variables here
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

        ClearBackground(RAYWHITE);

        DrawTexture(texture, 0, 0, WHITE);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    UnloadImage(cellular);
    UnloadTexture(texture);
    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow(); // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}