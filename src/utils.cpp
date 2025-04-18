#include <Arduino.h>

#include <cstdarg>
#include <cstdio>

#define SERIAL_BUFFER_SIZE 2048

void serialPrintf(const char* format, ...) {
    char buf[SERIAL_BUFFER_SIZE];

    va_list args;
    va_start(args, format);

    int length = vsnprintf(buf, SERIAL_BUFFER_SIZE, format, args);

    va_end(args);

    if (length < 0) {
        Serial.print("ERROR: Error during string formatting");
        return;
    }
    if (static_cast<size_t>(length) >= SERIAL_BUFFER_SIZE) {
        Serial.print("ERROR: String is longer than buf size.");
    }
    Serial.print(buf);
}

float chooseValue(float a, float b, float c, int n) {
    if (n == 0) {
        return a;
    } else if (n == 1) {
        return b;
    } else {
        return c;
    }
}

// returns exponential approximation, x between 0 and 1024, result between 0 and 1
float faderLog(float x) {
    float y1 = 0.0001374269 * x;
    float y2 = 0.00053041 * x - 0.134401;
    float y3 = 0.00271217 * x - 1.77726937;
    if (y2 > y1) {
        y1 = y2;
    }
    if (y3 > y1) {
        y1 = y3;
    }
    return y1;
}

float faderLin(float x) {
    return (1. / 1024.0) * x;
}

float faderLinSnap(float x, float eps) {
    x = faderLin(x);
    if (x >= (1.0f - eps)) {
        x = 1.0f;
    } else if (x < eps) {
        x = 0.0f;
    }
    return x;
}

float lerp(float v, float a, float b) {
    return a + (b - a) * v;
}
float inv_lerp(float v, float a, float b) {
    return (v - a) / (b - a);
}
