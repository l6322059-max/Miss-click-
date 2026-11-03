#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <chrono>
#include <string>

using namespace geode::prelude;

namespace {
    using SteadyClock = std::chrono::steady_clock;

    // PlayerButton::Jump is 1. Left and right (platformer) are never filtered.
    constexpr int kJumpButton = 1;
    // How long the counter takes to fade out once its delay is over.
    constexpr float kFadeOutSeconds = 0.4f;
    // Gap between the counter and the edge of the screen.
    constexpr float kEdgePadding = 10.f;

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

        int m_ignored = 0;
        CCLabelBMFont* m_label = nullptr;
    };

    void resetLevel() {
        PlayLayer::resetLevel();

        // A new attempt starts with a clean slate.
        for (int i = 0; i < 2; i++) {
            m_fields->m_hasAccepted[i] = false;
            m_fields->m_held[i] = false;
            m_fields->m_ignoring[i] = false;
        }
        m_fields->m_respawn = SteadyClock::now();
        m_fields->m_hasRespawn = true;

        if (Mod::get()->getSettingValue<bool>("reset-on-restart")) {
            m_fields->m_ignored = 0;
            if (m_fields->m_label) {
                m_fields->m_label->stopAllActions();
                m_fields->m_label->setVisible(false);
            }
        }
    }

    // Returns true when this button event is a miss-click and should be dropped.
    bool mcShouldIgnore(bool down, int button, bool isPlayer1) {
        if (button != kJumpButton) return false;

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
            m_fields->m_ignored++;
            this->mcShowCounter();
            return true;
        }

        m_fields->m_ignoring[i] = false;
        m_fields->m_held[i] = true;
        m_fields->m_lastAccepted[i] = now;
        m_fields->m_hasAccepted[i] = true;
        return false;
    }

    // Shows "Ignored Clicks: N" in the game's own font, then fades it away.
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
        label->setString(("Ignored Clicks: " + std::to_string(m_fields->m_ignored)).c_str());

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

// Geode has no handleButton hook on macOS, so the filter only exists on
// Windows and Android. On a Mac the mod still builds and just does nothing.
#if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_ANDROID)
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
