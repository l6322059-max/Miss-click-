#include <Geode/Bindings.hpp>
#include <Geode/modify/PlayerObject.hpp>

using namespace geode::prelude;

// Starting point for the miss-click manager.
// Right now this counts every button press on the player and logs the total.
// Replace the counting logic once we decide exactly what counts as a miss.

class $modify(MissClickPlayer, PlayerObject) {
    struct Fields {
        int m_presses = 0;
    };

    void pushButton(PlayerButton button) {
        m_fields->m_presses++;
        log::info("Button presses this attempt: {}", m_fields->m_presses);
        PlayerObject::pushButton(button);
    }
};
