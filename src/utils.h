#pragma once

// void serialdebugprintf(const char* format, ...);

float chooseValue(float a, float b, float c, int n);

float faderLog(float x);
float faderLin(float x);
float faderLinSnap(float x, float eps);

float lerp(float v, float a, float b);
float inv_lerp(float v, float a, float b);

int discretizeValue(int analogValue, int steps);
int discretizeSwitch(int pin, int steps);
