// ============================================================
// SoundboardStore.cpp
// Serialize and deserialize the soundboard clip list.
// ============================================================

#include "SoundboardStore.hpp"
#include "../hotkeys/HotkeyManager.hpp"

#include <QUuid> // QUuid::createUuid()
#include <QListWidgetItem>
#include <obs.hpp>
#include <obs-module.h> // blog(), obs_data_array_*

// ── Constructor ───────────────────────────────────────────────

void SoundboardStore::init(QListWidget *list, HotkeyManager *hotkeys)
{
	this->list = list;
	this->hotkeys = hotkeys;
}

// ── Save ──────────────────────────────────────────────────────

void SoundboardStore::save(obs_data_t *data) const
{
	OBSDataArrayAutoRelease array = obs_data_array_create();

	for (int i = 0; i < list->count(); ++i) {
		QListWidgetItem *item = list->item(i);
		OBSDataAutoRelease obj = obs_data_create();

		const QString uid = item->data(Qt::UserRole + 1).toString();

		obs_data_set_string(obj, "name", item->text().toUtf8().constData());
		obs_data_set_string(obj, "path", item->data(Qt::UserRole).toString().toUtf8().constData());
		obs_data_set_string(obj, "uid", uid.toUtf8().constData());

		// Per-clip hotkey bindings
		const obs_hotkey_id hkId = hotkeys->idForUid(uid);
		if (hkId != OBS_INVALID_HOTKEY_ID) {
			OBSDataArrayAutoRelease bindings = obs_hotkey_save(hkId);
			obs_data_set_array(obj, "hotkey_bindings", bindings);
		}

		obs_data_array_push_back(array, obj);
	}

	obs_data_set_array(data, "sounds", array);

	// Global Stop / Play-Selected bindings
	hotkeys->saveGlobalHotkeys(data);
}

// ── Load ──────────────────────────────────────────────────────

void SoundboardStore::load(obs_data_t *data)
{
	// Always start clean: clear hotkeys before wiping the list so
	// unregisterAll() can still find each item's UUID.
	hotkeys->unregisterAll(list);
	list->clear();

	if (!data)
		return;

	// Restore global Stop / Play-Selected bindings first.
	hotkeys->loadGlobalHotkeys(data);

	OBSDataArrayAutoRelease array = obs_data_get_array(data, "sounds");
	if (!array)
		return;

	const size_t count = obs_data_array_count(array);

	for (size_t i = 0; i < count; ++i) {
		OBSDataAutoRelease obj = obs_data_array_item(array, i);

		const QString name = QString::fromUtf8(obs_data_get_string(obj, "name"));
		const QString path = QString::fromUtf8(obs_data_get_string(obj, "path"));
		QString uid = QString::fromUtf8(obs_data_get_string(obj, "uid"));

		// Generate a fresh UUID if one wasn't saved (backwards compatibility).
		if (uid.isEmpty())
			uid = QUuid::createUuid().toString();

		auto *item = new QListWidgetItem(name);
		item->setData(Qt::UserRole, path);
		item->setData(Qt::UserRole + 1, uid);
		list->addItem(item);

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