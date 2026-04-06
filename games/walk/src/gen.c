#include "raylib.h"
#include "rlgl.h"               // OpenGL abstraction layer to multiple versions

#include "raymath.h"
#include "reasings.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "stb_perlin.h"

#ifndef RANDOM_FLOAT

float randomFloat(float min, float max)
{
    return min + (max - min) * ((float)rand() / (float)RAND_MAX);
}

#define RANDOM_FLOAT

#endif



void DrawTextureInRing(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint, float destOffset)
{
    // Check if texture is valid
    if (texture.id > 0)
    {
        float width = (float)texture.width;
        float height = (float)texture.height;

        bool flipX = false;

        if (source.width < 0) { flipX = true; source.width *= -1; }
        if (source.height < 0) source.y -= source.height;

        if (dest.width < 0) dest.width *= -1;
        if (dest.height < 0) dest.height *= -1;

        Vector2 topLeft = { 0 };
        Vector2 topRight = { 0 };
        Vector2 bottomLeft = { 0 };
        Vector2 bottomRight = { 0 };

        // Only calculate rotation if needed
        if (rotation == 0.0f)
        {
            float x = dest.x - origin.x;
            float y = dest.y - origin.y;
            topLeft = (Vector2){ x, y };
            topRight = (Vector2){ x + dest.width, y };
            bottomLeft = (Vector2){ x + destOffset, y + dest.height };
            bottomRight = (Vector2){ x + dest.width - destOffset, y + dest.height };
        }
        else
        {
            float sinRotation = sinf(rotation*DEG2RAD);
            float cosRotation = cosf(rotation*DEG2RAD);
            float x = dest.x;
            float y = dest.y;
            float dx = -origin.x;
            float dy = -origin.y;

            topLeft.x = x + dx*cosRotation - dy*sinRotation;
            topLeft.y = y + dx*sinRotation + dy*cosRotation;

            topRight.x = x + (dx + dest.width)*cosRotation - dy*sinRotation;
            topRight.y = y + (dx + dest.width)*sinRotation + dy*cosRotation;

            bottomLeft.x = x + dx*cosRotation - (dy + dest.height)*sinRotation + destOffset*cosRotation;
            bottomLeft.y = y + dx*sinRotation + (dy + dest.height)*cosRotation + destOffset*sinRotation;

            bottomRight.x = x + (dx + dest.width)*cosRotation - (dy + dest.height)*sinRotation -destOffset*cosRotation;
            bottomRight.y = y + (dx + dest.width)*sinRotation + (dy + dest.height)*cosRotation -destOffset*sinRotation;
        }

        rlSetTexture(texture.id);
        rlBegin(RL_QUADS);

            rlColor4ub(tint.r, tint.g, tint.b, tint.a);
            rlNormal3f(0.0f, 0.0f, 1.0f);                          // Normal vector pointing towards viewer

            // Top-left corner for texture and quad
            if (flipX) rlTexCoord2f((source.x + source.width)/width, source.y/height);
            else rlTexCoord2f(source.x/width, source.y/height);
            rlVertex2f(topLeft.x, topLeft.y);

            // Bottom-left corner for texture and quad
            if (flipX) rlTexCoord2f((source.x + source.width)/width, (source.y + source.height)/height);
            else rlTexCoord2f(source.x/width, (source.y + source.height)/height);
            rlVertex2f(bottomLeft.x, bottomLeft.y);

            // Bottom-right corner for texture and quad
            if (flipX) rlTexCoord2f(source.x/width, (source.y + source.height)/height);
            else rlTexCoord2f((source.x + source.width)/width, (source.y + source.height)/height);
            rlVertex2f(bottomRight.x, bottomRight.y);

            // Top-right corner for texture and quad
            if (flipX) rlTexCoord2f(source.x/width, source.y/height);
            else rlTexCoord2f((source.x + source.width)/width, source.y/height);
            rlVertex2f(topRight.x, topRight.y);

        rlEnd();
        rlSetTexture(0);

        
    }
}

Image GenTilableRocks(int width, int height, int outerRadius, int innerRadius, int tileSize)
{
    int seedsPerRow = width / tileSize;
    int seedsPerCol = height / tileSize;
    int seedCount = seedsPerRow * seedsPerCol;
    float seedOffset = tileSize / 15.0f;
    Vector2 *seeds = (Vector2 *)RL_MALLOC(seedCount * sizeof(Vector2));
    for (int i = 0; i < seedCount; i++)
    {
        int y = (i / seedsPerRow) * tileSize + GetRandomValue(seedOffset, tileSize - seedOffset);
        int x = (i % seedsPerRow) * tileSize + GetRandomValue(seedOffset, tileSize - seedOffset);
        seeds[i] = (Vector2){(float)x, (float)y};
    }

    Color *pixels = (Color *)RL_MALLOC(width * height * sizeof(Color));
    float outerSquared = outerRadius * outerRadius;
    // float innerSquared = innerRadius * innerRadius;
    int cx = width / 2;
    int cy = outerRadius;
    for (int y = 0; y < height; y++)
    {
        int tileY = y / tileSize;
        for (int x = 0; x < width; x++)
        {
            float distSquareFromCenter = (y - cy) * (y - cy) + (x - cx) * (x - cx);
            if (distSquareFromCenter > outerSquared)
            {
                pixels[y * width + x] = BLANK;
                continue;
            }

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
                for (int j = -1; j < 2; j++)
                {
                    if ((tileY + j < 0) || (tileY + j >= seedsPerCol))
                        continue;

                    int wrappedTileX = tileX;
                    if (tileX + i < 0) {
                        wrappedTileX += seedsPerRow;
                    }
                    else if (tileX + i >= seedsPerRow) {
                        wrappedTileX -= seedsPerRow;
                    }
                        


                    Vector2 neighborSeed = seeds[(tileY + j) * seedsPerRow + wrappedTileX + i];
                    if (tileX + i < 0) {
                        neighborSeed.x -= width; 
                    }
                    else if (tileX + i >= seedsPerRow) {
                        neighborSeed.x += width;
                    }
                        


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

            float border = 1.0f;
            if (secondMinDistance - minDistance < border * 0.1f * minDistance)
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

    return image;
}

Image GenImageRocks(int width, int innerRadius, int tileSize, int seedOffset, float border)
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
            float radDist = hypot(x - radius, y - radius);
            if (radDist > radius || radDist < innerRadius)
            {
                continue;
            }

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

            if (secondMinDistance - minDistance < border * 0.1f * minDistance)
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
            if (radius == fullRadius)
            {
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
            if (pointRadius < rocksFromRadius || pointRadius > fullRadius - tileLength / 2)
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

Image GenImageRocksRadial(int fullRadius, int fromRadius, int seedCount, float powFactor)
{
    int width = fullRadius * 2;
    int height = fullRadius * 2;
    Color *pixels = (Color *)RL_MALLOC(width * height * sizeof(Color));

    Image image = {
        .data = pixels,
        .width = width,
        .height = height,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};

    Vector2 *seeds = (Vector2 *)RL_MALLOC(seedCount * sizeof(Vector2));
    for (int i = 0; i < seedCount; i++)
    {
        while (true)
        {
            float factor = randomFloat(0.0f, 1.0f);
            factor = powf(factor, powFactor);
            int seedAtRadius = fromRadius + (fullRadius - fromRadius) * factor;
            float seedAtAngle = GetRandomValue(0, 360 * 10) / 10.0f;

            int x = fullRadius + seedAtRadius * sinf(seedAtAngle * DEG2RAD);
            int y = fullRadius - seedAtRadius * cosf(seedAtAngle * DEG2RAD);
            seeds[i] = (Vector2){(float)x, (float)y};
            bool anotherPointNearby = false;
            for (int j = 0; j < i; j++)
            {
                if (Vector2Distance(seeds[j], seeds[i]) < 5)
                {
                    anotherPointNearby = true;
                    break;
                }
            }
            if (anotherPointNearby)
            {
                continue;
            }
            break;
        }
    }

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            float pointRadius = hypot(x - fullRadius, y - fullRadius);
            if (pointRadius < fromRadius || pointRadius > fullRadius)
            {
                continue;
            }

            // Inside your x/y loop, before calculating distances:
            float noiseScale = 0.03f;    // Frequency of the "wiggles"
            float noiseIntensity = 3.0f; // Magnitude of the "wiggles" (in pixels)

            // Calculate offsets using Perlin noise
            float offsetX = stb_perlin_noise3((float)x * noiseScale, (float)y * noiseScale, 0, 0, 0, 0) * noiseIntensity;
            float offsetY = stb_perlin_noise3((float)y * noiseScale, (float)x * noiseScale, 1.0f, 0, 0, 0) * noiseIntensity;

            // Use these "warped" coordinates for the distance check
            float warpedX = (float)x + offsetX;
            float warpedY = (float)y + offsetY;

            float minDistance = width + height;
            float secondMinDistance = width + height;
            for (int i = 0; i < seedCount; i++)
            {
                float dist = hypot(warpedX - seeds[i].x, warpedY - seeds[i].y);
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

            if (secondMinDistance - minDistance >= 1.0f)
            {
                pixels[y * width + x] = BROWN;
            }
            else
            {
                pixels[y * width + x] = WHITE;
            }
        }
    }

    RL_FREE(seeds);

    return image;
}

// Image GenImageRocksSquareRadial(int fullRadius, int tileSize,  )

int main2(void)
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

    float powFactor = 0.09;

    // Main game loop
    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        if (IsKeyPressed(KEY_UP))
        {
            powFactor += 0.01f;
            reGenerate = true;
        }

        if (IsKeyPressed(KEY_DOWN))
        {
            powFactor -= 0.01f;
            if (powFactor <= 0.1f)
            {
                powFactor = 0.1f;
            }
            reGenerate = true;
        }

        float angleInDegreeCoveredByTex = 20.0f;
    
        float outRad = 1640;
        float inRad = 1300;
        int texWidth = outRad * sinf(angleInDegreeCoveredByTex * DEG2RAD * 0.5f);
        int texHeight = (outRad - inRad) * cosf(angleInDegreeCoveredByTex * DEG2RAD * 0.5f);
        int offset = 0;
        offset = (texWidth/2 -  (outRad - inRad) * sinf(angleInDegreeCoveredByTex * DEG2RAD * 0.5f));
        float tileSize = 40;


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
            TraceLog(LOG_INFO, "Tex Size %dx%d, offset %d", texWidth, texHeight, offset);
            cellular = GenTilableRocks(texWidth, texHeight, outRad, inRad, tileSize);
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

        float cx = 450;
        float cy = 450;
        float scale = 1.0f;
        
        for (int i=0; i  < 360; i+= angleInDegreeCoveredByTex) {
            float cosFactor = cosf(i * DEG2RAD * 0.5f)/2;
            float sinFactor = sinf(i * DEG2RAD * 0.5f)/2;

            Vector2 position = {cx + outRad * sinFactor - texWidth * cosFactor * 0.5f, cy - outRad * cosFactor - texHeight * sinFactor };
            Rectangle source = { 0.0f, 0.0f, (float)texture.width, (float)texture.height };
            Rectangle dest = { position.x, position.y, (float)texture.width*scale, (float)texture.height*scale };
            Vector2 origin = { 0.0f, 0.0f };
            DrawTextureInRing(texture, source, dest, origin, i, WHITE, offset);

        }
        

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