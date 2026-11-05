#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/UILayer.hpp>
#include "./PauseButton.cpp"
#include <chrono>

using namespace geode::prelude;

bool cmpint(int first, int second, std::string lockSetting)
{
	return ((first != second) && Mod::get()->getSettingValue<bool>(lockSetting));
}

bool cmpfloat(float first, float second, std::string lockSetting)
{
	return ((first != second) && Mod::get()->getSettingValue<bool>(lockSetting));
}

bool placeCheckpointNextFrame = false;

auto lastChk = std::chrono::high_resolution_clock::now();

// removal delay
auto lastRemove = std::chrono::high_resolution_clock::now();
bool waitingAfterRemove = false;

class $modify(PlayLayer)
{
	// this needs to be placed here so that there's a reference to a PlayLayer
	void postUpdate(float dt)
	{
		PlayLayer::postUpdate(dt);
		if (!this->m_isPracticeMode) return;
		if (!Mod::get()->getSettingValue<bool>("enabled")) return;

		auto now = std::chrono::high_resolution_clock::now();

		// after removing a checkpoint, wait 1 second before placing another
		if (waitingAfterRemove)
		{
			auto sinceRemove = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastRemove).count();
			if (sinceRemove < 1000)
			{
				placeCheckpointNextFrame = false;
				return;
			}
		}

		// difference between now and last checkpoint time (seconds)
		auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - lastChk).count();

		if (placeCheckpointNextFrame && diff >= Mod::get()->getSettingValue<double>("min-delay") && !this->m_player1->m_isDashing)
		{
			lastChk = std::chrono::high_resolution_clock::now();
			waitingAfterRemove = false;
			PlayLayer::markCheckpoint();
		}

		if (diff >= Mod::get()->getSettingValue<double>("max-delay") && !this->m_player1->m_isDashing)
		{
			log::info("been too long! ({})", std::to_string(diff));
			lastChk = std::chrono::high_resolution_clock::now();
			waitingAfterRemove = false;
			PlayLayer::markCheckpoint();
		}

		placeCheckpointNextFrame = false;
	}
};

class $modify(UILayer)
{
	// runs when you tap the remove checkpoint button
	void onDeleteCheckpoint(CCObject* sender)
	{
		UILayer::onDeleteCheckpoint(sender);
		lastRemove = std::chrono::high_resolution_clock::now();
		waitingAfterRemove = true;
		placeCheckpointNextFrame = false;
	}
};

class $modify(GJBaseGameLayer)
{
	void toggleDualMode(GameObject * p0, bool p1, PlayerObject * p2, bool p3)
	{
		if (Mod::get()->getSettingValue<bool>("dual"))
			placeCheckpointNextFrame = true;

		GJBaseGameLayer::toggleDualMode(p0, p1, p2, p3);
	}
};

class $modify(PlayerObject)
{
	void switchedToMode(GameObjectType p0)
	{
		PlayerObject::switchedToMode(p0);
		if (Mod::get()->getSettingValue<bool>("gamemode"))
			placeCheckpointNextFrame = true;
	}

	void reversePlayer(EffectGameObject* p0)
	{
		PlayerObject::reversePlayer(p0);
		if (Mod::get()->getSettingValue<bool>("direction"))
			placeCheckpointNextFrame = true;
	}

	void flipGravity(bool upsideDown, bool somethingThatsAlwaysFalse)
	{
		PlayerObject::flipGravity(upsideDown, somethingThatsAlwaysFalse);
		if (Mod::get()->getSettingValue<bool>("gravity"))
			placeCheckpointNextFrame = true;
	}

	void update(float dt)
	{
		float speed = this->m_playerSpeed;
		float size = this->m_vehicleSize;
		
		PlayerObject::update(dt);

		if (cmpfloat(speed, this->m_playerSpeed, "speed"))
			placeCheckpointNextFrame = true;

		if (cmpfloat(size, this->m_vehicleSize, "size"))
			placeCheckpointNextFrame = true;
	}
};
