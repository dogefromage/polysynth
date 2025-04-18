#pragma once

#define TABLE_SIZE 64

const float sineLookup[TABLE_SIZE] = {
    0.0, 0.1, 0.2, 0.29, 0.38, 0.47, 0.56, 0.63, 0.71,
    0.77, 0.83, 0.88, 0.92, 0.96, 0.98, 1., 1., 1.,
    0.98, 0.96, 0.92, 0.88, 0.83, 0.77, 0.71, 0.63, 0.56,
    0.47, 0.38, 0.29, 0.2, 0.1, 0., -0.1, -0.19, -0.29,
    -0.38, -0.47, -0.56, -0.63, -0.71, -0.77, -0.83, -0.88, -0.92,
    -0.96, -0.98, -1., -1., -1., -0.98, -0.96, -0.92, -0.88,
    -0.83, -0.77, -0.71, -0.63, -0.56, -0.47, -0.38, -0.29, -0.2, -0.1};

inline float sineApprox(float& x) {

    while (x < 0.0) x += M_TWOPI;
    while (x >= M_TWOPI) x -= M_TWOPI;

    float t = (1.0f / M_TWOPI) * x;

    // lookup
    int a = ((int)(TABLE_SIZE * t)) % TABLE_SIZE;
    int b = (a + 1) % TABLE_SIZE;
    float ya = sineLookup[a];
    float yb = sineLookup[b];

    // interpolate
    float X = TABLE_SIZE * t;
    float f = X - truncf(X);
    float y = (1 - f) * ya + f * yb;
    return y;
}