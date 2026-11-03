#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <chrono>
#include <string>

using namespace geode::prelude;

#if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_ANDROID)
    #define MC_HAS_INPUT_HOOK 1
#else
    #define MC_HAS_INPUT_HOOK 0
#endif

namespace {
    using SteadyClock = std::chrono::steady_clock;
    constexpr int kJumpButton = 1;
    constexpr int kBufferHoldMs = 60;
    constexpr float kFadeOutSeconds = 0.4f;
    constexpr float kEdgePadding = 10.f;

    long long msBetween(SteadyClock::time_point from, SteadyClock::time_point to) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(to - from).count();
    }
}

class $modify(MissClickLayer, PlayLayer) {
public:
    struct Fields {
        SteadyClock::time_point m_lastAccepted[2];
        bool m_hasAccepted[2] = {false, false};
        bool m_held[2] = {false, false};
        bool m_ignoring[2] = {false, false};

        SteadyClock::time_point m_respawn;
        bool m_hasRespawn = false;

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

    bool mcShouldIgnore(bool down, int button, bool isPlayer1) {
        if (button != kJumpButton) return false;
        if (m_fields->m_injecting) return false;

        auto mod = Mod::get();
        if (!mod->getSettingValue<bool>("enabled")) return false;

        int i = isPlayer1 ? 0 : 1;

        if (!down) {
            if (m_fields->m_held[i]) {
                m_fields->m_held[i] = false;
                return false;
            }
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

    void mcRecordAirTap(int i) {
        m_fields->m_hasAirTap[i] = false;
        if (Mod::get()->getSettingValue<int64_t>("early-buffer") <= 0) return;

        PlayerObject* player = i == 0 ? m_player1 : m_player2;
        if (!player || player->m_isDead || player->m_isOnGround) return;

        bool flying = player->m_isShip || player->m_isBird ||
                      player->m_isDart || player->m_isSwing;
        bool onRing = player->m_touchingRings && player->m_touchingRings->count() > 0;
        if (flying || onRing) return;

        m_fields->m_airTap[i] = SteadyClock::now();
        m_fields->m_hasAirTap[i] = true;
    }

#if MC_HAS_INPUT_HOOK
    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        this->mcUpdateBuffer();
    }

    void mcInject(bool down, bool isPlayer1) {
        m_fields->m_injecting = true;
        GJBaseGameLayer::handleButton(down, kJumpButton, isPlayer1);
        m_fields->m_injecting = false;
    }

    void mcUpdateBuffer() {
        auto now = SteadyClock::now();

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
            if (msBetween(m_fields->m_airTap[i], now) > bufferMs) {
                m_fields->m_hasAirTap[i] = false;
                continue;
            }
            if (!landed) continue;

            m_fields->m_hasAirTap[i] = false;
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
