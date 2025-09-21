#include <cstdint>
#include <map>
#include <vector>

constexpr int NUM_KEYS = 61;

enum class MatrixLevel {
    OPEN,
    FIRST,
    BOTH,
};

enum class KeyStates {
    OPEN,
    PRESSED,
    SUSTAINED,
};

struct KeyDownSchedule {
    int key, velocity;
    uint32_t timeMillis;
};

class Keybed {
    MatrixLevel lastMatrixLevels[NUM_KEYS];
    uint32_t travelStarts[NUM_KEYS];
    KeyStates keyStates[NUM_KEYS];
    std::vector<KeyDownSchedule> scheduledKeyDowns;
    void updateKey(int key, MatrixLevel currentLevel);
    void (*handleKeyDown)(int key, int velocity) = nullptr;
    void (*handleKeyUp)(int key) = nullptr;
    bool isSustaining = false;

   public:
    void init();
    void update();
    void test();

    void setHandleKeyDown(void (*callback)(int key, int velocity));
    void setHandleKeyUp(void (*callback)(int key));

    const KeyStates* Keybed::getKeyStates() const;
};
