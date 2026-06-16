#pragma once

// ============================================================
// HotkeyManager.hpp
// Owns all OBS frontend hotkey registration for the Soundboard.
//
// Responsibilities:
//   - Register / unregister per-clip hotkeys
//   - Register / unregister the global Stop and Play-Selected hotkeys
//   - Save and load all hotkey bindings to/from obs_data_t
//   - Manage the heap-allocated path copies used by hotkey callbacks
//
// Lifetime note:
//   The hotkey callbacks capture a raw g_soundboard pointer.  The
//   manager must be destroyed (and all hotkeys unregistered) before
//   g_soundboard becomes invalid.
// ============================================================

#include <QListWidget>
#include <QMap>
#include <QString>

#include <obs-frontend-api.h> // obs_hotkey_id, obs_hotkey_register_frontend …
#include <obs-module.h>       // obs_data_t, obs_data_array_t

class Soundboard; // forward-declare to avoid circular include

class HotkeyManager {
public:
	// sb is the Soundboard instance passed as callback data for the
	// global Stop / Play-Selected hotkeys.
	explicit HotkeyManager(Soundboard *sb);
	~HotkeyManager();

	// ── Per-clip hotkeys ─────────────────────────────────────────
	// Register a new play hotkey for the list item.
	// The item must already have Qt::UserRole (path) and
	// Qt::UserRole+1 (UUID) set before calling this.
	void registerHotkey(QListWidgetItem *item);

	// Unregister the play hotkey for the list item.
	void unregisterHotkey(QListWidgetItem *item);

	// Unregister every per-clip hotkey for all items in the list.
	void unregisterAll(QListWidget *list);

	// ── Persistence ──────────────────────────────────────────────
	// Save global (Stop / Play-Selected) hotkey bindings into data.
	void saveGlobalHotkeys(obs_data_t *data) const;

	// Load global hotkey bindings from data.
	void loadGlobalHotkeys(obs_data_t *data);

	// Return the registered obs_hotkey_id for a clip UUID,
	// or OBS_INVALID_HOTKEY_ID if not found.
	obs_hotkey_id idForUid(const QString &uid) const;

private:
	// Owning Soundboard — used as callback data for global hotkeys.
	Soundboard *soundboard = nullptr;

	// Global hotkey IDs (registered once in the constructor).
	obs_hotkey_id stopHotkeyId = OBS_INVALID_HOTKEY_ID;
	obs_hotkey_id playHotkeyId = OBS_INVALID_HOTKEY_ID;

	// Per-clip maps, all keyed by clip UUID (Qt::UserRole+1).
	QMap<QString, obs_hotkey_id> hotkeyMap;        // uid → hotkey id
	QMap<QString, QString> hotkeyPaths;            // uid → file path
	QMap<QString, std::string *> hotkeyPathCopies; // uid → heap path copy
						       //  (kept alive for callback)
};