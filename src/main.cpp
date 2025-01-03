#include <Geode/Geode.hpp>
#include <Geode/modify/CreatorLayer.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/modify/CCLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCDirector.hpp>
#include <Geode/modify/SecretLayer2.hpp>
#include <Geode/modify/CreatorLayer.hpp>
#include <Geode/modify/SecretRewardsLayer.hpp>
#include <Geode/modify/GauntletSelectLayer.hpp>
#define UIHIDE(x,id) if (auto xd = x->getChildByID(id)) { xd->setVisible(false); }; 
static bool appliedPatch = false;
using namespace geode::prelude;
class $modify(CreatorLayer) {
	void onSecretVault(CCObject* sender) {
		appliedPatch = true;
		auto SecretLayer2 = SecretLayer2::scene();
        CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(0.5f, SecretLayer2));
	};
	void onTreasureRoom(CCObject* sender) {
		appliedPatch = true;
		auto SecretRewardsLayer = SecretRewardsLayer::scene(false);
        CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(0.5f, SecretRewardsLayer));
	}
	void onGauntlets(CCObject* sender) {
		appliedPatch = true;
		auto GauntletSelectLayer = GauntletSelectLayer::scene(1);
        CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(0.5f, GauntletSelectLayer));
	}
};
class $modify(SecretLayer2) {
	struct Fields {
			bool m_SafeToPopScene;
		};
	virtual bool init() {
		this->m_fields->m_SafeToPopScene = appliedPatch;
		appliedPatch = false;
		return SecretLayer2::init();
	}
	void onBack(CCObject* sender) {
		if (!this->m_fields->m_SafeToPopScene) return SecretLayer2::onBack(sender);
		GameManager::get()->safePopScene();
	};
};

class $modify(GauntletSelectLayer) {
	struct Fields {
			bool m_SafeToPopScene;
		};
	 bool init(int p0) {
		this->m_fields->m_SafeToPopScene = appliedPatch;
		appliedPatch = false;
		return GauntletSelectLayer::init(p0);
	}
	void onBack(CCObject* sender) {
		if (!this->m_fields->m_SafeToPopScene) return GauntletSelectLayer::onBack(sender);
		appliedPatch = false;
		GameManager::get()->safePopScene();
	};
};

class $modify(SecretRewardsLayer) {
	struct Fields {
			bool m_SafeToPopScene;
		};
	 bool init(bool p0) {
		this->m_fields->m_SafeToPopScene = appliedPatch;
		appliedPatch = false;
		return SecretRewardsLayer::init(p0);
	}
	void onBack(CCObject* sender) {
		if (!this->m_fields->m_SafeToPopScene) return SecretRewardsLayer::onBack(sender);
		appliedPatch = false;
		GameManager::get()->safePopScene();
	};
};
int stack = 0;
class $modify(FixedPlayLayer,PlayLayer) {
	void onQuit() {
		PlayLayer::onQuit();
		Loader::get()->queueInMainThread([=] {
			if (stack == 1) {
				this->release();
				GameManager::sharedState()->m_playLayer = nullptr;
				GameManager::get()->m_gameLayer = nullptr;
				stack = 0;
			};
		});
	};
	 bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
		if (!PlayLayer::init(level,useReplay,dontCreateObjects)) {
			return false;
		}
		//this->m_fields->m_Scenefix = cocos2d::CCScene::get();
		auto creatorLayer = CreatorLayer::create();
		UIHIDE(creatorLayer,"swelvy-background")
		UIHIDE(creatorLayer,"background")
		UIHIDE(creatorLayer,"ninsam.day_and_night_system/Events")
		creatorLayer->setKeyboardEnabled(false);
		creatorLayer->setZOrder(4);
		//creatorLayer->registerScriptTouchHandler(1,true,1200,false);
		this->addChild(creatorLayer);
	
		return true;
	}
	bool isCurrentPlayLayer(){
		auto playLayer = cocos2d::CCScene::get()->getChildByType<PlayLayer>(0);
		return playLayer == this;
	}
	bool isPaused(bool checkCurrent){
		if(checkCurrent && !isCurrentPlayLayer()) return false;

		for(CCNode* child : CCArrayExt<CCNode*>(this->getParent()->getChildren())) {
			if(typeinfo_cast<PauseLayer*>(child)) {
				return true;
			}
		}

		return false;
	}
	void onEnterH(){
		auto weRunningScene = this->getParent() == CCScene::get();

		if(weRunningScene){
			CCLayer::onEnter();
			return;
		}

		Loader::get()->queueInMainThread([self = Ref(this)] {
		        if (!self->isPaused(false)) {
		            self->CCLayer::onEnter();
		        }
		    });
	}

};
class $modify(CCDirector) { 
    static void onModify(auto& self) { 
        (void)self.setHookPriority("CCDirector::willSwitchToScene", 100); 
    } 
    void willSwitchToScene(CCScene* scene) { 
		if (PlayLayer* playerlayr = scene->getChildByType<PlayLayer>(0)) {
			if (PlayLayer::get() != playerlayr) {
				if ( PlayLayer::get() != nullptr ) {
					PlayLayer::get()->release();
				}
				stack-=1;
				GameManager::sharedState()->m_playLayer = playerlayr;
				GameManager::get()->m_gameLayer = playerlayr;
				playerlayr->pauseGame(true);
			}
			if (!playerlayr->getUserObject("addedToStack"_spr)) {
				playerlayr->setUserObject("addedToStack"_spr,CCBool::create(true));
				playerlayr->retain();
				stack+=1;
			}
			//log::debug("stack : {}",stack);
		} else {
			if (PlayLayer* p = PlayLayer::get()) {
				p->pauseGame(true);
			}
		}
	
        CCDirector::willSwitchToScene(scene); 
    }       
};

class $modify(MyCCLayer, CCLayer){
	void onEnter(){
		if(typeinfo_cast<PlayLayer*>(this) && this->getUserObject("addedToStack"_spr)){
				auto pl = reinterpret_cast<FixedPlayLayer*>(static_cast<CCLayer*>(this));
				pl->onEnterH();
		} else {
			CCLayer::onEnter();
		}
	}
};
