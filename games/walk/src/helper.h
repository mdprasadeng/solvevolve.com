
float randomFloat(float min, float max)
{
    return min + (max - min) * ((float)rand() / (float)RAND_MAX);
}


int randomInt(int min, int max)
{
    return min + rand() % (max - min + 1);
}


float Vector2AnglePositiveDegree(Vector2 l1, Vector2 l2)
{
    float angle = Vector2Angle(l1, l2);
    angle = (angle < 0) ? 2 * PI + angle : angle;
    return RAD2DEG * angle;
}

float AngleByLine(Vector2 center, Vector2 point)
{
    float angle = atan2f(point.y - center.y, point.x - center.x) * RAD2DEG;
    return angle < 0 ? 360 + angle : angle;
}

void GetRandomValueDistribution(float *result, int count, float sum, float min, float max)
{

    float total = 0;
    for (int i = 0; i < count; i++)
    {
        result[i] = GetRandomValue(min * 100000 / sum, max * 100000 / sum) / 100000.0f;
        total += result[i];
    }

    float scaleBy = sum / total;
    for (int i = 0; i < count; i++)
    {
        result[i] *= scaleBy;
    }
}

Color GetOneRandomColorFrom(int count, Color options[])
{
    int index = randomInt(0, count - 1);
    return options[index];
}

int GetOneRandomIntFrom(int count, int options[])
{
    int index = randomInt(0, count - 1);
    return options[index];
}