#pragma once

#include <Arduino.h>
#include <IntervalTimer.h>

class StableTimer {
   public:
    StableTimer();
    void begin(void (*callbackFunc)(), uint32_t intervalMicros = 10000);  // default 100Hz
    void setIntervalMicroseconds(uint32_t usec);
    void start();
    void stop();
    void enterCriticalSection();
    void exitCriticalSection();

   private:
    static void onTimerStatic();
    void onTimer();

    IntervalTimer timer;
    void (*callback)() = nullptr;
    volatile bool inCritical = false;
    volatile bool missedTick = false;
    uint32_t interval = 10000;

    static StableTimer* instance;
};
