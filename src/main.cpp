#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <chrono>
#include <string>

using namespace geode::prelude;

namespace {
    using Clock = std::chrono::steady_clock;

    // How long the counter takes to fade out once its delay is over.
    constexpr float kFadeOutSeconds = 0.4f;
    // Gap between the counter and the edge of the screen.
    constexpr float kEdgePadding = 10.f;
}

// A miss-click is a click shortly before you die. How shortly is the
// "Miss Window" setting. Everything is stored on the PlayLayer, so every
// level starts counting from zero.
class $modify(MissClickLayer, PlayLayer) {
    struct Fields {
        Clock::time_point m_lastClick;
        bool m_hasClick = false;
        bool m_wasDown = false;
        int m_misses = 0;
        CCLabelBMFont* m_label = nullptr;
    };

    // Runs every frame. Notes the moment the jump button goes down.
    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        if (!Mod::get()->getSettingValue<bool>("enabled")) return;

        bool down = (m_player1 && m_player1->buttonDown(PlayerButton::Jump)) ||
                    (m_player2 && m_player2->buttonDown(PlayerButton::Jump));

        if (down && !m_fields->m_wasDown) {
            m_fields->m_lastClick = Clock::now();
            m_fields->m_hasClick = true;
        }
        m_fields->m_wasDown = down;
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        PlayLayer::destroyPlayer(player, object);
        if (!Mod::get()->getSettingValue<bool>("enabled")) return;
        if (!m_fields->m_hasClick) return;

        // One click can only cause one miss, even if both players die.
        m_fields->m_hasClick = false;

        auto windowMs = Mod::get()->getSettingValue<int64_t>("miss-window");
        auto sinceClick = std::chrono::duration_cast<std::chrono::milliseconds>(
            Clock::now() - m_fields->m_lastClick
        ).count();

        if (sinceClick <= windowMs) {
            this->showMiss();
        }
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        m_fields->m_hasClick = false;

        if (Mod::get()->getSettingValue<bool>("reset-on-restart")) {
            m_fields->m_misses = 0;
            if (m_fields->m_label) {
                m_fields->m_label->stopAllActions();
                m_fields->m_label->setVisible(false);
            }
        }
    }

    // Adds one miss and shows the counter in the game's own font.
    void showMiss() {
        auto mod = Mod::get();
        m_fields->m_misses++;

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
        label->setString(("Miss Clicks: " + std::to_string(m_fields->m_misses)).c_str());

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
