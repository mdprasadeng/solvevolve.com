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
    return min + (max - min) * ((float)rand() / (float)RAND_MAX);
}

int randomInt(int min, int max)
{
    return min + rand() % (max - min + 1);
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