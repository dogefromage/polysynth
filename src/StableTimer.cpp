#include "StableTimer.h"

StableTimer::StableTimer() {}

StableTimer* StableTimer::instance = nullptr;

void StableTimer::begin(void (*callbackFunc)(), uint32_t intervalMicros) {
    callback = callbackFunc;
    interval = intervalMicros;
    instance = this;
    timer.begin(onTimerStatic, interval);
}

void StableTimer::setIntervalMicroseconds(uint32_t usec) {
    interval = usec;
    timer.update(interval);
}

void StableTimer::start() {
    timer.begin(onTimerStatic, interval);
}

void StableTimer::stop() {
    timer.end();
}

void StableTimer::enterCriticalSection() {
    noInterrupts();
    inCritical = true;
    interrupts();
}

void StableTimer::exitCriticalSection() {
    noInterrupts();
    inCritical = false;
    bool runNow = missedTick;
    missedTick = false;
    interrupts();

    if (runNow && callback) {
        callback();  // run immediately after exiting
    }
}

void StableTimer::onTimerStatic() {
    if (instance) instance->onTimer();
}

void StableTimer::onTimer() {
    if (inCritical) {
        missedTick = true;
    } else if (callback) {
        callback();
    }
}
