

#include "HotkeyManager.hpp"
#include "../Soundboard.hpp" 
#include <obs-module.h> 

extern Soundboard *g_soundboard;


HotkeyManager::HotkeyManager(Soundboard *sb) : soundboard(sb)
{
	// mute and unmute source
	restartHotkeyId = obs_hotkey_register_frontend(
		"soundboard.restart", "Soundboard: Restart Source",
		[](void *data, obs_hotkey_id, obs_hotkey_t *, bool pressed) {
			if (!pressed)
				return;
			auto *sb = static_cast<Soundboard *>(data);
			sb->getSourceManager().restart();
		},
		soundboard);

	muteHotkeyId = obs_hotkey_register_frontend(
		"soundboard.mute", "Soundboard: Mute Source",
		[](void *data, obs_hotkey_id, obs_hotkey_t *, bool pressed) {
			if (!pressed)
				return;
			auto *sb = static_cast<Soundboard *>(data);
			sb->getSourceManager().mute();
		},
		soundboard);

	unmuteHotkeyId = obs_hotkey_register_frontend(
		"soundboard.unmute", "Soundboard: Unmute Source",
		[](void *data, obs_hotkey_id, obs_hotkey_t *, bool pressed) {
			if (!pressed)
				return;
			auto *sb = static_cast<Soundboard *>(data);
			sb->getSourceManager().unmute();
		},
		soundboard);

	//  GLOBAL HOTKEYS
	// stop playback
	stopHotkeyId = obs_hotkey_register_frontend(
		"soundboard.stop", "Soundboard: Stop Playback",
		[](void *data, obs_hotkey_id, obs_hotkey_t *, bool pressed) {
			if (!pressed)
				return;
			auto *sb = static_cast<Soundboard *>(data);
			sb->getSourceManager().stop();
		},
		soundboard);


	// play last item
	playHotkeyId = obs_hotkey_register_frontend(
		"soundboard.play_selected", "Soundboard: Play Selected",
		[](void *data, obs_hotkey_id, obs_hotkey_t *, bool pressed) {
			if (!pressed)
				return;
			auto *sb = static_cast<Soundboard *>(data);

			// Directly use the last played item
			auto *item = sb->getLastPlayedItem();
			if (item) {
				sb->getSourceManager().playFile(item->data(Qt::UserRole).toString());
			}
		},
		soundboard);
}

HotkeyManager::~HotkeyManager()
{
	if (stopHotkeyId != OBS_INVALID_HOTKEY_ID)
		obs_hotkey_unregister(stopHotkeyId);

	if (playHotkeyId != OBS_INVALID_HOTKEY_ID)
		obs_hotkey_unregister(playHotkeyId);

	for (auto it = hotkeyMap.begin(); it != hotkeyMap.end(); ++it)
		obs_hotkey_unregister(it.value());

	for (auto *copy : hotkeyPathCopies)
		delete copy;
}


void HotkeyManager::registerHotkey(QListWidgetItem *item)
{
	if (!item)
		return;

	const QString uid = item->data(Qt::UserRole + 1).toString();
	const QString name = item->text();
	const QString path = item->data(Qt::UserRole).toString();

	hotkeyPaths.insert(uid, path);

	const std::string hotkeyName = ("soundboard.play." + uid).toStdString();
	const std::string hotkeyDesc = ("Soundboard: " + name).toStdString();

	// Heap-allocate the path so the lambda can safely capture it by pointer
	// across the hotkey's entire lifetime.
	auto *pathCopy = new std::string(path.toStdString());
	hotkeyPathCopies.insert(uid, pathCopy);

	obs_hotkey_id id = obs_hotkey_register_frontend(
		hotkeyName.c_str(), hotkeyDesc.c_str(),
		[](void *data, obs_hotkey_id, obs_hotkey_t *, bool pressed) {
			if (!pressed)
				return;
			if (g_soundboard) {
				// Find the item by path and update last played
				const QString &path = QString::fromStdString(*static_cast<std::string *>(data));

				// Find the item in the list
				QListWidgetItem *item = g_soundboard->findItemByPath(path);
				if (item) {
					g_soundboard->setLastPlayedItem(item);
				}

				g_soundboard->getSourceManager().playFile(path);
			}
		},
		pathCopy);

	hotkeyMap.insert(uid, id);
}

void HotkeyManager::unregisterHotkey(QListWidgetItem *item)
{
	if (!item)
		return;

	const QString uid = item->data(Qt::UserRole + 1).toString();

	auto it = hotkeyMap.find(uid);
	if (it == hotkeyMap.end())
		return;

	obs_hotkey_unregister(it.value());
	hotkeyMap.erase(it);
	hotkeyPaths.remove(uid);

	auto pathIt = hotkeyPathCopies.find(uid);
	if (pathIt != hotkeyPathCopies.end()) {
		delete pathIt.value();
		hotkeyPathCopies.erase(pathIt);
	}
}

void HotkeyManager::unregisterAll(QListWidget *list)
{
	if (!list)
		return;

	for (int i = 0; i < list->count(); ++i)
		unregisterHotkey(list->item(i));
}


void HotkeyManager::saveGlobalHotkeys(obs_data_t *data) const
{
	if (restartHotkeyId != OBS_INVALID_HOTKEY_ID) {
		OBSDataArrayAutoRelease bindings = obs_hotkey_save(restartHotkeyId);
	}
	if (muteHotkeyId != OBS_INVALID_HOTKEY_ID) {
		OBSDataArrayAutoRelease bindings = obs_hotkey_save(muteHotkeyId);
		obs_data_set_array(data, "mute_hotkey", bindings);
	}
	if (unmuteHotkeyId != OBS_INVALID_HOTKEY_ID) {
		OBSDataArrayAutoRelease bindings = obs_hotkey_save(unmuteHotkeyId);
		obs_data_set_array(data, "unmute_hotkey", bindings);
	}
	if (stopHotkeyId != OBS_INVALID_HOTKEY_ID) {
		OBSDataArrayAutoRelease bindings = obs_hotkey_save(stopHotkeyId);
		obs_data_set_array(data, "stop_hotkey", bindings);
	}

	if (playHotkeyId != OBS_INVALID_HOTKEY_ID) {
		OBSDataArrayAutoRelease bindings = obs_hotkey_save(playHotkeyId);
		obs_data_set_array(data, "play_hotkey", bindings);
	}
}

void HotkeyManager::loadGlobalHotkeys(obs_data_t *data)
{
	if (!data)
		return;

	if (restartHotkeyId != OBS_INVALID_HOTKEY_ID) {
		OBSDataArrayAutoRelease bindings = obs_data_get_array(data, "restart_hotkey");
		if (bindings)
			obs_hotkey_load(restartHotkeyId, bindings);
	}
	if (muteHotkeyId != OBS_INVALID_HOTKEY_ID) {
		OBSDataArrayAutoRelease bindings = obs_data_get_array(data, "mute_hotkey");
		if (bindings)
			obs_hotkey_load(muteHotkeyId, bindings);
	}
	if (unmuteHotkeyId != OBS_INVALID_HOTKEY_ID) {
		OBSDataArrayAutoRelease bindings = obs_data_get_array(data, "unmute_hotkey");
		if (bindings)
			obs_hotkey_load(unmuteHotkeyId, bindings);
	}
	if (stopHotkeyId != OBS_INVALID_HOTKEY_ID) {
		OBSDataArrayAutoRelease bindings = obs_data_get_array(data, "stop_hotkey");
		if (bindings)
			obs_hotkey_load(stopHotkeyId, bindings);
	}

	if (playHotkeyId != OBS_INVALID_HOTKEY_ID) {
		OBSDataArrayAutoRelease bindings = obs_data_get_array(data, "play_hotkey");
		if (bindings)
			obs_hotkey_load(playHotkeyId, bindings);
	}
}

obs_hotkey_id HotkeyManager::idForUid(const QString &uid) const
{
	return hotkeyMap.value(uid, OBS_INVALID_HOTKEY_ID);
}
