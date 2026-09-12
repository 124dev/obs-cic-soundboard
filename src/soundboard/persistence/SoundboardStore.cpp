

#include "SoundboardStore.hpp"
#include "../hotkeys/HotkeyManager.hpp"

#include <QUuid> 
#include <QListWidgetItem>
#include <obs.hpp>
#include <obs-module.h> 

void SoundboardStore::init(QListWidget *list, HotkeyManager *hotkeys)
{
	this->list = list;
	this->hotkeys = hotkeys;
}


void SoundboardStore::save(obs_data_t *data) const
{
	// Create an array that will contain every saved sound
	OBSDataArrayAutoRelease array = obs_data_array_create();
	// Save each sound in the list
	for (int i = 0; i < list->count(); ++i) {
		QListWidgetItem *item = list->item(i);
		OBSDataAutoRelease obj = obs_data_create();
		// Retrieve the sound's unique UID
		const QString uid = item->data(Qt::UserRole + 1).toString();
		// Save the sound's display name, file path, and UUID
		obs_data_set_string(obj, "name", item->text().toUtf8().constData());
		obs_data_set_string(obj, "path", item->data(Qt::UserRole).toString().toUtf8().constData());
		obs_data_set_string(obj, "uid", uid.toUtf8().constData());
		// Save any hotkey bindings assigned to this sound
		const obs_hotkey_id hkId = hotkeys->idForUid(uid);
		if (hkId != OBS_INVALID_HOTKEY_ID) {
			OBSDataArrayAutoRelease bindings = obs_hotkey_save(hkId);
			obs_data_set_array(obj, "hotkey_bindings", bindings);
		}
		// Add the sound to the save array
		obs_data_array_push_back(array, obj);
	}
	// Store the completed sound list
	obs_data_set_array(data, "sounds", array);

	// Save the global Play and Stop hotkeys
	hotkeys->saveGlobalHotkeys(data);
}


void SoundboardStore::load(obs_data_t *data)
{
	// Remove all registered hotkeys before clearing the list
	// so each item's UUID is still available during cleanup
	hotkeys->unregisterAll(list);
	list->clear();
	// Nothing to load
	if (!data)
		return;

	// Restore the global Hotkeys
	hotkeys->loadGlobalHotkeys(data);
	// Retrieve the saved sound list
	OBSDataArrayAutoRelease array = obs_data_get_array(data, "sounds");
	// if array doesnt exist, return
	if (!array)
		return;
	
	const size_t count = obs_data_array_count(array);
	// Restore every saved sound
	for (size_t i = 0; i < count; ++i) {
		OBSDataAutoRelease obj = obs_data_array_item(array, i);

		const QString name = QString::fromUtf8(obs_data_get_string(obj, "name"));
		const QString path = QString::fromUtf8(obs_data_get_string(obj, "path"));
		QString uid = QString::fromUtf8(obs_data_get_string(obj, "uid"));

		// Generate a fresh UUID if one wasn't saved.
		if (uid.isEmpty())
			uid = QUuid::createUuid().toString();
		// Recreate the list item
		auto *item = new QListWidgetItem(name);
		item->setData(Qt::UserRole, path);
		item->setData(Qt::UserRole + 1, uid);
		list->addItem(item);
		// Re-register the sound's OBS hotkey
		hotkeys->registerHotkey(item);

		// Restore per-clip key bindings after re-registering the hotkey.
		OBSDataArrayAutoRelease bindings = obs_data_get_array(obj, "hotkey_bindings");
		if (bindings) {
			const obs_hotkey_id hkId = hotkeys->idForUid(uid);
			if (hkId != OBS_INVALID_HOTKEY_ID)
				obs_hotkey_load(hkId, bindings);
		}
	}

	blog(LOG_INFO, "[Soundboard] Loaded %zu clip(s)", count);
}
