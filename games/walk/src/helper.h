#include "rlgl.h"

#ifndef SMOOTH_CIRCLE_ERROR_RATE
    #define SMOOTH_CIRCLE_ERROR_RATE    0.5f      // Circle error rate
#endif

Color toColor(float col[4])
{
    Color color;
    color.r = (unsigned char)(col[0] * 255.0f);
    color.g = (unsigned char)(col[1] * 255.0f);
    color.b = (unsigned char)(col[2] * 255.0f);
    color.a = (unsigned char)(col[3] * 255.0f);
    return color;
};

#ifndef RANDOM_FLOAT

float randomFloat(float min, float max)
{
    return min + (max - min) * ((float)rand() / (float)RAND_MAX);
}

#define RANDOM_FLOAT

#endif

int randomInt(int min, int max)
{
    return min + rand() % (max - min + 1);
}

void DrawRectangleLinesPro(Rectangle rect, float rotation, Color color, int lineThick)
{

    Vector2 p1 = (Vector2){rect.x - rect.width/2 * cosf(rotation * DEG2RAD), rect.y - rect.width/2 * sinf(rotation * DEG2RAD)};
    Vector2 p2 = (Vector2){rect.x + rect.width/2 * cosf(rotation * DEG2RAD), rect.y + rect.width/2 * sinf(rotation * DEG2RAD)};
    Vector2 p3 = Vector2Add(p1, (Vector2){-rect.height * sinf(rotation * DEG2RAD), rect.height * cosf(rotation * DEG2RAD)});
    Vector2 p4 = Vector2Add(p2, (Vector2){-rect.height * sinf(rotation * DEG2RAD), rect.height * cosf(rotation * DEG2RAD)});
   
    
    DrawCircle(p1.x, p1.y, lineThick / 2.2f, color);
    DrawCircle(p2.x, p2.y, lineThick / 2.2f, color);
    DrawCircle(p3.x, p3.y, lineThick / 2.2f, color);
    DrawCircle(p4.x, p4.y, lineThick / 2.2f, color);

    DrawLineEx(p1, p2, lineThick, color);
    DrawLineEx(p1, p3, lineThick, color);  
    DrawLineEx(p2, p4, lineThick, color);
    DrawLineEx(p3, p4, lineThick, color);

}

float Vector2AnglePositiveDegree(Vector2 l1, Vector2 l2) {
    float angle = Vector2Angle(l1, l2);
    angle = (angle < 0) ? 2 * PI + angle: angle;
    return RAD2DEG * angle;
}


// Draw a piece of a circle
void DrawCircleSectorWithTeeth(Vector2 center, float radius, float startAngle, float endAngle, int segments, Color color, float toothSize)
{
    if (startAngle == endAngle) return;
    if (radius <= 0.0f) radius = 0.1f;  // Avoid div by zero

    // Function expects (endAngle > startAngle)
    if (endAngle < startAngle)
    {
        // Swap values
        float tmp = startAngle;
        startAngle = endAngle;
        endAngle = tmp;
    }

    int minSegments = (int)ceilf((endAngle - startAngle)/90);

    if (segments < minSegments)
    {
        // Calculate the maximum angle between segments based on the error rate (usually 0.5f)
        float th = acosf(2*powf(1 - SMOOTH_CIRCLE_ERROR_RATE/radius, 2) - 1);
        segments = (int)((endAngle - startAngle)*ceilf(2*PI/th)/360);

        if (segments <= 0) segments = minSegments;
    }

    float stepLength = (endAngle - startAngle)/(float)segments;
    float angle = startAngle;

#if defined(SUPPORT_QUADS_DRAW_MODE)
    rlSetTexture(GetShapesTexture().id);
    Rectangle shapeRect = GetShapesTextureRectangle();

    rlBegin(RL_QUADS);

        // NOTE: Every QUAD actually represents two segments
        for (int i = 0; i < segments/2; i++)
        {
            rlColor4ub(color.r, color.g, color.b, color.a);

            rlTexCoord2f(shapeRect.x/texShapes.width, shapeRect.y/texShapes.height);
            rlVertex2f(center.x, center.y);

            rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, shapeRect.y/texShapes.height);
            rlVertex2f(center.x + cosf(DEG2RAD*(angle + stepLength*2.0f))*radius, center.y + sinf(DEG2RAD*(angle + stepLength*2.0f))*radius);

            rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
            rlVertex2f(center.x + cosf(DEG2RAD*(angle + stepLength))*radius, center.y + sinf(DEG2RAD*(angle + stepLength))*radius);

            rlTexCoord2f(shapeRect.x/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
            rlVertex2f(center.x + cosf(DEG2RAD*angle)*radius, center.y + sinf(DEG2RAD*angle)*radius);

            angle += (stepLength*2.0f);
        }

        // NOTE: In case number of segments is odd, we add one last piece to the cake
        if ((((unsigned int)segments)%2) == 1)
        {
            rlColor4ub(color.r, color.g, color.b, color.a);

            rlTexCoord2f(shapeRect.x/texShapes.width, shapeRect.y/texShapes.height);
            rlVertex2f(center.x, center.y);

            rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
            rlVertex2f(center.x + cosf(DEG2RAD*(angle + stepLength))*radius, center.y + sinf(DEG2RAD*(angle + stepLength))*radius);

            rlTexCoord2f(shapeRect.x/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
            rlVertex2f(center.x + cosf(DEG2RAD*angle)*radius, center.y + sinf(DEG2RAD*angle)*radius);

            rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, shapeRect.y/texShapes.height);
            rlVertex2f(center.x, center.y);
        }

    rlEnd();

    rlSetTexture(0);
#else
    rlBegin(RL_TRIANGLES);
        float radiusA = radius;
        float radiusB = radius + toothSize;
        bool useToothInB = true;
        for (int i = 0; i < segments; i++)
        {
            rlColor4ub(color.r, color.g, color.b, color.a);

            rlVertex2f(center.x, center.y);
            rlVertex2f(center.x + cosf(DEG2RAD*(angle + stepLength))*radiusA, center.y + sinf(DEG2RAD*(angle + stepLength))*radiusA);
            rlVertex2f(center.x + cosf(DEG2RAD*angle)*radiusB, center.y + sinf(DEG2RAD*angle)*radiusB);

            angle += stepLength;
            if (useToothInB)
            {
                useToothInB = false;
                radiusA = radius + toothSize;
                radiusB = radius;
            }
            else
            {
                useToothInB = true;
                radiusA = radius;
                radiusB = radius + toothSize;
                
            }

        }
    rlEnd();
#endif
}