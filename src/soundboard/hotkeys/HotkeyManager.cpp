// ============================================================
// HotkeyManager.cpp
// Implementation of per-clip and global OBS hotkey management.
// ============================================================

#include "HotkeyManager.hpp"
#include "../Soundboard.hpp" // full definition needed for callback cast

#include <obs-module.h> // blog()

// g_soundboard is defined in plugin-main.cpp and used inside the
// per-clip hotkey lambda so the callback always routes through the
// current live instance.
extern Soundboard *g_soundboard;

// ── Construction / destruction ───────────────────────────────

HotkeyManager::HotkeyManager(Soundboard *sb) : soundboard(sb)
{
	// ── Global: Stop Playback ────────────────────────────────
	stopHotkeyId = obs_hotkey_register_frontend(
		"soundboard.stop", "Soundboard: Stop Playback",
		[](void *data, obs_hotkey_id, obs_hotkey_t *, bool pressed) {
			if (!pressed)
				return;
			auto *sb = static_cast<Soundboard *>(data);
			sb->getSourceManager().stop();
		},
		soundboard);

	// ── Global: Play Selected ────────────────────────────────
	// Now plays the last played item directly
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

	// Per-clip hotkeys should already be cleared by unregisterAll()
	// before the destructor runs, but clean up anything remaining.
	for (auto it = hotkeyMap.begin(); it != hotkeyMap.end(); ++it)
		obs_hotkey_unregister(it.value());

	for (auto *copy : hotkeyPathCopies)
		delete copy;
}

// ── Per-clip registration ─────────────────────────────────────

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

// ── Persistence ───────────────────────────────────────────────

void HotkeyManager::saveGlobalHotkeys(obs_data_t *data) const
{
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
