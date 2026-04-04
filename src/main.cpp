#include <vector>
#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>

using namespace geode::prelude;
using namespace cocos2d;
using namespace cocos2d::extension;
using namespace geode::utils::web;

class $modify(LevelInfoLayer) {
    bool init(GJGameLevel* level, bool challenge) {
		if (!LevelInfoLayer::init(level, challenge))
			return false;

        

        auto req = WebRequest();
        // auto response = req.get("https://api.aredl.net/api/aredl/list");
        // log::info("{}", response);
		return true;
	}

    void onPlay(CCObject* sender) {
        auto mod = Mod::get();
        std::vector<int> checkedIDs = mod->getSavedValue<std::vector<int>>("checked-id", {});
        int levelID = m_level->m_levelID;

        if (std::find(checkedIDs.begin(), checkedIDs.end(), levelID) != checkedIDs.end()) {
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
                    mod->setSavedValue("checked-id", checkedIDs);
                    LevelInfoLayer::onPlay(sender);
                }
            }
        );
    }
};