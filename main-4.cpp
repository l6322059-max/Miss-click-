#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <chrono>
#include <string>

using namespace geode::prelude;

// handleButton is the one place every tap goes through, but Geode only has it
// on Windows and Android. On a Mac the mod still builds and does nothing.
#if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_ANDROID)
    #define MC_HAS_INPUT_HOOK 1
#else
    #define MC_HAS_INPUT_HOOK 0
#endif

namespace {
    using SteadyClock = std::chrono::steady_clock;

    // PlayerButton::Jump is 1. Left and right (platformer) are never touched.
    constexpr int kJumpButton = 1;
    // How long a saved tap stays held down after it fires on landing.
    constexpr int kBufferHoldMs = 60;
    // How long the counter takes to fade out once its delay is over.
    constexpr float kFadeOutSeconds = 0.4f;
    // Gap between the counter and the edge of the screen.
    constexpr float kEdgePadding = 10.f;

    // The beta warning shows once. Change the number in the key whenever the
    // text changes, so everyone sees the new text once.
    constexpr char const* kBetaWarningKey = "beta-warning-shown-v1";
    constexpr char const* kBetaWarningTitle = "Warning:";
    constexpr char const* kBetaWarningText =
        "This is a BETA test it will be buggy I'm working on it please don't hate "
        "the mode hate the mode maker.\n\n"
        "Texted by: yapetape";

    long long msBetween(SteadyClock::time_point from, SteadyClock::time_point to) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(to - from).count();
    }
}

// Everything is stored on the PlayLayer, so every level starts fresh.
class $modify(MissClickLayer, PlayLayer) {
public:
    struct Fields {
        // Index 0 is player 1, index 1 is player 2.
        SteadyClock::time_point m_lastAccepted[2];
        bool m_hasAccepted[2] = {false, false};
        bool m_held[2] = {false, false};
        bool m_ignoring[2] = {false, false};

        SteadyClock::time_point m_respawn;
        bool m_hasRespawn = false;

        // Early tap buffer.
        SteadyClock::time_point m_airTap[2];
        bool m_hasAirTap[2] = {false, false};
        bool m_wasOnGround[2] = {false, false};
        SteadyClock::time_point m_releaseAt[2];
        bool m_pendingRelease[2] = {false, false};
        bool m_injecting = false;

        int m_fixed = 0;
        CCLabelBMFont* m_label = nullptr;
    };

    void resetLevel() {
        PlayLayer::resetLevel();

        // A new attempt starts with a clean slate.
        for (int i = 0; i < 2; i++) {
            m_fields->m_hasAccepted[i] = false;
            m_fields->m_held[i] = false;
            m_fields->m_ignoring[i] = false;
            m_fields->m_hasAirTap[i] = false;
            m_fields->m_wasOnGround[i] = false;
            m_fields->m_pendingRelease[i] = false;
        }
        m_fields->m_respawn = SteadyClock::now();
        m_fields->m_hasRespawn = true;

        if (Mod::get()->getSettingValue<bool>("reset-on-restart")) {
            m_fields->m_fixed = 0;
            if (m_fields->m_label) {
                m_fields->m_label->stopAllActions();
                m_fields->m_label->setVisible(false);
            }
        }
    }

    // Returns true when this button event should be dropped before the game sees it.
    bool mcShouldIgnore(bool down, int button, bool isPlayer1) {
        if (button != kJumpButton) return false;
        // Taps the mod pressed itself always go through.
        if (m_fields->m_injecting) return false;

        auto mod = Mod::get();
        if (!mod->getSettingValue<bool>("enabled")) return false;

        int i = isPlayer1 ? 0 : 1;

        if (!down) {
            // A release the game is waiting for always goes through.
            if (m_fields->m_held[i]) {
                m_fields->m_held[i] = false;
                return false;
            }
            // The release of a click we ignored is dropped too.
            if (m_fields->m_ignoring[i]) {
                m_fields->m_ignoring[i] = false;
                return true;
            }
            return false;
        }

        auto now = SteadyClock::now();
        bool ignore = false;

        auto grace = mod->getSettingValue<int64_t>("respawn-grace");
        if (grace > 0 && m_fields->m_hasRespawn &&
            msBetween(m_fields->m_respawn, now) < grace) {
            ignore = true;
        }

        auto gap = mod->getSettingValue<int64_t>("min-click-gap");
        if (!ignore && gap > 0 && m_fields->m_hasAccepted[i] &&
            msBetween(m_fields->m_lastAccepted[i], now) < gap) {
            ignore = true;
        }

        if (ignore) {
            m_fields->m_ignoring[i] = true;
            m_fields->m_fixed++;
            this->mcShowCounter();
            return true;
        }

        m_fields->m_ignoring[i] = false;
        m_fields->m_held[i] = true;
        m_fields->m_lastAccepted[i] = now;
        m_fields->m_hasAccepted[i] = true;

        this->mcRecordAirTap(i);
        return false;
    }

    // Remembers a tap made in the air so it can fire when the player lands.
    void mcRecordAirTap(int i) {
        m_fields->m_hasAirTap[i] = false;
        if (Mod::get()->getSettingValue<int64_t>("early-buffer") <= 0) return;

        PlayerObject* player = i == 0 ? m_player1 : m_player2;
        if (!player || player->m_isDead || player->m_isOnGround) return;

        // Flying modes, and taps used on a ring, are left alone.
        bool flying = player->m_isShip || player->m_isBird ||
                      player->m_isDart || player->m_isSwing;
        bool onRing = player->m_touchingRings && player->m_touchingRings->count() > 0;
        if (flying || onRing) return;

        m_fields->m_airTap[i] = SteadyClock::now();
        m_fields->m_hasAirTap[i] = true;
    }

#if MC_HAS_INPUT_HOOK
    // Runs every frame. Fires a saved tap the moment the player lands.
    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        this->mcUpdateBuffer();
    }

    // Presses or releases the jump button as if a finger did it.
    void mcInject(bool down, bool isPlayer1) {
        m_fields->m_injecting = true;
        GJBaseGameLayer::handleButton(down, kJumpButton, isPlayer1);
        m_fields->m_injecting = false;
    }

    void mcUpdateBuffer() {
        auto now = SteadyClock::now();

        // Let go of a saved tap once it has been held for a moment, unless a
        // real finger is down. This runs even if the mod was just switched off.
        for (int i = 0; i < 2; i++) {
            if (m_fields->m_pendingRelease[i] && now >= m_fields->m_releaseAt[i]) {
                m_fields->m_pendingRelease[i] = false;
                if (!m_fields->m_held[i]) this->mcInject(false, i == 0);
            }
        }

        auto mod = Mod::get();
        if (!mod->getSettingValue<bool>("enabled")) return;
        auto bufferMs = mod->getSettingValue<int64_t>("early-buffer");

        for (int i = 0; i < 2; i++) {
            PlayerObject* player = i == 0 ? m_player1 : m_player2;
            if (!player) continue;

            bool onGround = player->m_isOnGround;
            bool landed = onGround && !m_fields->m_wasOnGround[i];
            m_fields->m_wasOnGround[i] = onGround;

            if (!m_fields->m_hasAirTap[i]) continue;

            // Too old to count as an early tap.
            if (msBetween(m_fields->m_airTap[i], now) > bufferMs) {
                m_fields->m_hasAirTap[i] = false;
                continue;
            }
            if (!landed) continue;

            m_fields->m_hasAirTap[i] = false;

            // If the finger is still down, the game already jumps on landing.
            if (player->m_isDead ||
                player->buttonDown(static_cast<PlayerButton>(kJumpButton))) {
                continue;
            }

            this->mcInject(true, i == 0);
            m_fields->m_pendingRelease[i] = true;
            m_fields->m_releaseAt[i] = now + std::chrono::milliseconds(kBufferHoldMs);
            m_fields->m_fixed++;
            this->mcShowCounter();
        }
    }
#endif

    // The level screen is up: show the beta warning if this is the first time.
    void onEnterTransitionDidFinish() {
        PlayLayer::onEnterTransitionDidFinish();
        this->mcMaybeShowBetaWarning();
    }

    void mcMaybeShowBetaWarning() {
        // setSavedValue hands back the old value, so this only passes once.
        if (Mod::get()->setSavedValue(kBetaWarningKey, true)) return;

        // A normal GD popup, so the OK button sits at the bottom.
        auto alert = FLAlertLayer::create(kBetaWarningTitle, kBetaWarningText, "OK");
        // Attach it to this layer so it can't end up on the previous scene.
        alert->m_scene = this;
        alert->show();
    }

    // Shows "Fixed Clicks: N" in the game's own font, then fades it away.
    void mcShowCounter() {
        auto mod = Mod::get();
        if (!mod->getSettingValue<bool>("show-counter")) return;

        if (!m_fields->m_label) {
            auto created = CCLabelBMFont::create("", "bigFont.fnt");
            created->setID("miss-click-label"_spr);
            this->addChild(created, 1000);
            m_fields->m_label = created;
        }
        auto label = m_fields->m_label;

        // Corner and size come from the settings.
        auto corner = mod->getSettingValue<std::string>("position");
        bool right = corner.find("Right") != std::string::npos;
        bool top = corner.find("Top") != std::string::npos;
        auto win = CCDirector::get()->getWinSize();
        label->setAnchorPoint({right ? 1.f : 0.f, top ? 1.f : 0.f});
        label->setPosition({
            right ? win.width - kEdgePadding : kEdgePadding,
            top ? win.height - kEdgePadding : kEdgePadding
        });
        label->setScale(static_cast<float>(mod->getSettingValue<double>("scale")));
        label->setString(("Fixed Clicks: " + std::to_string(m_fields->m_fixed)).c_str());

        // Show it, then fade it away unless "Always Show" is on.
        auto opacity = static_cast<unsigned char>(
            255 * mod->getSettingValue<int64_t>("opacity") / 100
        );
        label->stopAllActions();
        label->setVisible(true);
        label->setOpacity(opacity);

        if (!mod->getSettingValue<bool>("always-show")) {
            auto delay = static_cast<float>(mod->getSettingValue<double>("fade-time"));
            label->runAction(CCSequence::createWithTwoActions(
                CCDelayTime::create(delay),
                CCFadeTo::create(kFadeOutSeconds, 0)
            ));
        }
    }
};

#if MC_HAS_INPUT_HOOK
class $modify(MissClickInput, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        auto layer = PlayLayer::get();
        if (layer && static_cast<GJBaseGameLayer*>(layer) == this) {
            auto self = static_cast<MissClickLayer*>(layer);
            if (self->mcShouldIgnore(down, button, isPlayer1)) return;
        }
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }
};
#endif
