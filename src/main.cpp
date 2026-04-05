#include <vector>
#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/loader/Event.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/modify/LevelBrowserLayer.hpp>
#include <Geode/modify/LevelCell.hpp>

using namespace geode::prelude;

static void fetchAdLevels(std::function<void()> onComplete = nullptr);
static bool isAdLevel(int levelID);

static std::vector<int> s_adLevelIDs = {};
static TaskHolder<web::WebResponse> s_fetchTask;


class $modify(MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        s_adLevelIDs = Mod::get()->getSavedValue<std::vector<int>>("ad-level-ids", {});
        fetchAdLevels();

        return true;
    }
};

class $modify(LevelBrowserLayer) {
    void onRefresh(CCObject* sender) {
        LevelBrowserLayer::onRefresh(sender);
        fetchAdLevels();
    }
};

class $modify(LevelCell) {
    void loadCustomLevelCell() {
        LevelCell::loadCustomLevelCell();

        auto badge = CCSprite::createWithSpriteFrameName("badge.png"_spr);
        badge->setScale(1.0f);
        badge->setZOrder(5);
        badge->setID("adblock-badge");
        badge->setVisible(isAdLevel(m_level->m_levelID));
        badge->setAnchorPoint({ 0.f, 0.5f });

        auto creatorLabel = m_mainLayer->getChildByIDRecursive("creator-name");
        auto copyIndicator = m_mainLayer->getChildByID("copy-indicator");
        auto highObjIndicator = m_mainLayer->getChildByID("high-object-indicator");

        if (!copyIndicator && !highObjIndicator) {
            auto mainMenu = m_mainLayer->getChildByID("main-menu");
            if (mainMenu) {
                copyIndicator = mainMenu->getChildByID("copy-indicator");
                highObjIndicator = mainMenu->getChildByID("high-object-indicator");
            }
        }

        float baseX = 182.425f;
        float baseY = 52.f;

        if (highObjIndicator) {
            auto worldPos = highObjIndicator->getParent()->convertToWorldSpace(highObjIndicator->getPosition());
            auto localPos = m_mainLayer->convertToNodeSpace(worldPos);
            baseX = localPos.x + 15.f;
            baseY = localPos.y;
        } else if (copyIndicator) {
            auto worldPos = copyIndicator->getParent()->convertToWorldSpace(copyIndicator->getPosition());
            auto localPos = m_mainLayer->convertToNodeSpace(worldPos);
            baseX = localPos.x + 15.f;
            baseY = localPos.y;
        }

        badge->setPosition({ baseX, baseY });
        m_mainLayer->addChild(badge);
    }
};

class $modify(LevelInfoLayer) {
    bool init(GJGameLevel* level, bool challenge) {
        if (!LevelInfoLayer::init(level, challenge)) return false;

        auto badge = CCSprite::createWithSpriteFrameName("badge.png"_spr);
        badge->setScale(1.f);
        badge->setZOrder(5);
        badge->setID("adblock-badge");
        badge->setVisible(isAdLevel(m_level->m_levelID));
        badge->setAnchorPoint({ 0.f, 0.5f });

        auto creatorLabel = this->getChildByID("creator-info-menu");
        auto copyIndicator = this->getChildByID("copy-indicator");
        auto highObjIndicator = this->getChildByID("high-object-indicator");

        float baseX = creatorLabel->getPositionX() + creatorLabel->getContentWidth() / 2 + 9.0f;
        float baseY = 276.0f;

        if (highObjIndicator) {
            baseX = highObjIndicator->getPositionX() + 18.f;
            baseY = highObjIndicator->getPositionY();
        } else if (copyIndicator) {
            baseX = copyIndicator->getPositionX() + 18.f;
            baseY = copyIndicator->getPositionY();
        }

        badge->setPosition({ baseX, baseY });
        this->addChild(badge);

        return true;
    }

    void onPlay(CCObject* sender) {
        auto mod = Mod::get();
        int levelID = m_level->m_levelID;
        auto checkedIDs = mod->getSavedValue<std::vector<int>>("checked-ids", {});

        bool alreadyDismissed = std::find(checkedIDs.begin(), checkedIDs.end(), levelID) != checkedIDs.end();

        if (alreadyDismissed || !isAdLevel(levelID)) {
            LevelInfoLayer::onPlay(sender);
            return;
        }

        geode::createQuickPopup(
            "AdBlock",
            "<cr>Warning:</c> This level contains ads.",
            "Cancel", "Play",
            [this, sender, mod, levelID, checkedIDs](auto, bool btn2) mutable {
                if (btn2) {
                    checkedIDs.push_back(levelID);
                    mod->setSavedValue("checked-ids", checkedIDs);
                    LevelInfoLayer::onPlay(sender);
                }
            }
        );
    }

    void onUpdate(CCObject* sender) {
        LevelInfoLayer::onUpdate(sender);
        fetchAdLevels([this]() {
            if (auto badge = this->getChildByID("adblock-badge")) {
                badge->setVisible(isAdLevel(m_level->m_levelID));
            }
        });
    }
};


static void fetchAdLevels(std::function<void()> onComplete) {
    auto req = web::WebRequest();
    s_fetchTask.spawn(
        req.get("https://gist.githubusercontent.com/einXYZ/1126ee331f86eaafa959f0e67575ceb0/raw/adlevels.json"),
        [onComplete](web::WebResponse res) {
            if (res.ok()) {
                auto json = res.json().unwrapOrDefault();
                if (json.isArray()) {
                    s_adLevelIDs.clear();
                    for (auto& item : json.asArray().unwrap()) {
                        s_adLevelIDs.push_back(static_cast<int>(item.asDouble().unwrap()));
                    }
                    Mod::get()->setSavedValue("ad-level-ids", s_adLevelIDs);
                    log::info("Fetched and saved {} IDs", s_adLevelIDs.size());
                }
            } else {
                log::warn("Fetch failed, using {} cached IDs", s_adLevelIDs.size());
            }
            if (onComplete) onComplete();
        }
    );
}

static bool isAdLevel(int levelID) {
    return std::find(s_adLevelIDs.begin(), s_adLevelIDs.end(), levelID) != s_adLevelIDs.end();
}