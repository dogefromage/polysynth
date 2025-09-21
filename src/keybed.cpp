#include "keybed.h"

#include "config.h"

/**
 * [Bank]: first or second keyboard matrix
 * [Row]: Shift register output address from 0 to 7 in specified bank
 * [Column]: Mux input ABC address from 0 to 7
 * Value: index of key from 0 to NUM_KEYS-1. if above 100 then
 *          second pushbutton at index (key-100) is meant
 */
int16_t keyMatrices[2][8][8] = {
    {{32, 36, 34, 38, 33, 37, 35, 39},
     {132, 136, 134, 138, 133, 137, 135, 139},
     {140, 144, 142, 146, 141, 145, 143, 147},
     {148, 152, 150, 154, 149, 153, 151, 155},
     {156, 160, 158, -1, 157, -1, 159, -1},
     {40, 44, 42, 46, 41, 45, 43, 47},
     {48, 52, 50, 54, 49, 53, 51, 55},
     {56, 60, 58, -1, 57, -1, 59, -1}},
    {{8, 12, 10, 14, 9, 13, 11, 15},
     {16, 20, 18, 22, 17, 21, 19, 23},
     {0, 4, 2, 6, 1, 5, 3, 7},
     {100, 104, 102, 106, 101, 105, 103, 107},
     {108, 112, 110, 114, 109, 113, 111, 115},
     {116, 120, 118, 122, 117, 121, 119, 123},
     {124, 128, 126, 130, 125, 129, 127, 131},
     {24, 28, 26, 30, 25, 29, 27, 31}}};

// per bank input from mux
int keyMatrixInputs[2] = {PIN_KYBD_MUX_1, PIN_KYBD_MUX_2};

static int calculateVelocity(uint32_t millis) {
    return std::min(127, 1000 / (int)millis);
}

void Keybed::updateKey(const int key, const MatrixLevel currentLevel) {
    const MatrixLevel& lastLevel = lastMatrixLevels[key];

    if (currentLevel == MatrixLevel::OPEN) {
        travelStarts[key] = 0;  // invalidate
    }

    if (currentLevel == MatrixLevel::FIRST && lastLevel == MatrixLevel::OPEN) {
        travelStarts[key] = millis();
    }

    if (currentLevel == MatrixLevel::BOTH && lastLevel != currentLevel) {
        // key is pressed
        int velocity = 127;
        if (travelStarts[key] > 0) {
            // is valid
            uint32_t deltaMillis = millis() - travelStarts[key];
            velocity = calculateVelocity(deltaMillis);
        }

        if (isSustaining) {
            // retrigger key
            if (handleKeyUp) {
                handleKeyUp(key);
            }
            uint32_t retriggerTime = 10;
            scheduledKeyDowns.push_back({key, velocity, millis() + retriggerTime});
        } else {
            // play now
            scheduledKeyDowns.push_back({key, velocity, millis()});
        }

        keyStates[key] = KeyStates::PRESSED;
    }

    if (currentLevel == MatrixLevel::OPEN) {
        if (isSustaining && keyStates[key] == KeyStates::PRESSED) {
            keyStates[key] = KeyStates::SUSTAINED;
        }
        if (!isSustaining && keyStates[key] != KeyStates::OPEN) {
            keyStates[key] = KeyStates::OPEN;
            if (handleKeyUp) {
                handleKeyUp(key);
            }
        }
    }

    lastMatrixLevels[key] = currentLevel;
}

void Keybed::init() {
    for (int i = 0; i < NUM_KEYS; i++) {
        lastMatrixLevels[i] = MatrixLevel::OPEN;
    }
    for (int i = 0; i < NUM_KEYS; i++) {
        keyStates[i] = KeyStates::OPEN;
    }
}

void Keybed::update() {
    isSustaining = !digitalRead(PIN_FTSW);

    MatrixLevel levels[NUM_KEYS];
    for (int i = 0; i < NUM_KEYS; i++) {
        levels[i] = MatrixLevel::OPEN;
    }

    for (int row = 0; row < 8; row++) {
        enterCritical();
        spiWrapper.beginTransaction(keyboardSPISettings);
        digitalWrite(PIN_KYBD_CS, LOW);
        delayMicroseconds(1);
        // send two hot bits for both matrices of the keyboard through the SRs
        uint16_t twoHotRow = (uint16_t)(((1 << 8) | 1) << row);
        spiWrapper.transfer16(twoHotRow);
        digitalWrite(PIN_KYBD_CS, HIGH);
        spiWrapper.endTransaction();
        exitCritical();

        // read inputs
        for (int column = 0; column < 8; column++) {
            digitalWrite(PIN_KYBD_MUX_A, column & 4);
            digitalWrite(PIN_KYBD_MUX_B, column & 2);
            digitalWrite(PIN_KYBD_MUX_C, column & 1);

            delayMicroseconds(20);
            // delayMicroseconds(5);

            for (int bank = 0; bank < 2; bank++) {
                int button = keyMatrices[bank][row][column];
                if (button < 0) {
                    // unused address in button matrix
                    continue;
                }

                if (digitalRead(keyMatrixInputs[bank])) {
                    if (button >= 100) {
                        // is second trigger
                        levels[button - 100] = MatrixLevel::BOTH;
                    } else if (levels[button] != MatrixLevel::BOTH) {
                        // is first trigger (ensure not to clear previous set level 2)
                        levels[button] = MatrixLevel::FIRST;
                    }
                }
            }
        }
    }

    for (int i = 0; i < NUM_KEYS; i++) {
        updateKey(i, levels[i]);
    }

    // triggering
    uint32_t currentMillis = millis();
    for (auto iter = scheduledKeyDowns.begin(); iter != scheduledKeyDowns.end();) {
        if (iter->timeMillis <= currentMillis) {
            // trigger
            if (handleKeyDown) {
                handleKeyDown(iter->key, iter->velocity);
            }
            iter = scheduledKeyDowns.erase(iter);
        } else {
            iter++;
        }
    }
}

void Keybed::test() {
    bool somethingPressed = false;

    for (int row = 0; row < 8; row++) {
        enterCritical();
        spiWrapper.beginTransaction(keyboardSPISettings);
        digitalWrite(PIN_KYBD_CS, LOW);
        delayMicroseconds(1);
        // send two hot bits for both matrices of the keyboard through the SRs
        uint16_t twoHotRow = (uint16_t)(((1 << 8) | 1) << row);
        spiWrapper.transfer16(twoHotRow);
        digitalWrite(PIN_KYBD_CS, HIGH);
        spiWrapper.endTransaction();
        exitCritical();

        // read inputs
        for (int column = 0; column < 8; column++) {
            digitalWrite(PIN_KYBD_MUX_A, column & 4);
            digitalWrite(PIN_KYBD_MUX_B, column & 2);
            digitalWrite(PIN_KYBD_MUX_C, column & 1);

            delayMicroseconds(200);

            if (digitalRead(PIN_KYBD_MUX_1)) {
                debugprintf("(1,%d,%d) ", row, column);
                somethingPressed = true;
            }

            if (digitalRead(PIN_KYBD_MUX_2)) {
                debugprintf("(2,%d,%d) ", row, column);
                somethingPressed = true;
            }
        }
    }

    if (somethingPressed) {
        debugprintf("\n");
    }
}

void Keybed::setHandleKeyDown(void (*callback)(int key, int velocity)) {
    handleKeyDown = callback;
}

void Keybed::setHandleKeyUp(void (*callback)(int key)) {
    handleKeyUp = callback;
}

const KeyStates* Keybed::getKeyStates() const {
    return keyStates;
}
