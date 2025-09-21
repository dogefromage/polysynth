#include <cstdint>
#include <map>

enum class KeyStates {
    OPEN,
    TRAVELLING,
    PRESSED,
};

struct ActiveKey {
    KeyStates state;
    uint32_t travellingMillis;
};

class Keybed {
    // keys which are travelling, pressed, (maybe add sustained here as well)
    std::map<int, ActiveKey> activeKeys;
    void updateKey(int key, KeyStates currentState, bool sustaining);
    void (*handleKeyDown)(int key, int velocity) = nullptr;
    void (*handleKeyUp)(int key) = nullptr;

   public:
    void read();
    void test();

    void setHandleKeyDown(void (*callback)(int key, int velocity));
    void setHandleKeyUp(void (*callback)(int key));

    const std::map<int, ActiveKey>& getActiveKeys() const;
};
