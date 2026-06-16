#pragma once

// ============================================================
// SoundboardStore.hpp
// Handles serialization and deserialization of the clip list.
//
// Responsibilities:
//   - saveData(): write clip names, paths, UIDs, and per-clip
//                 hotkey bindings into an obs_data_t object
//   - loadData(): clear the list, rebuild it from obs_data_t,
//                 re-register hotkeys, and restore bindings
//
// This class holds non-owning pointers to the list widget and
// the two managers; it must not outlive them.
// ============================================================

#include <QListWidget>
#include <obs-module.h> // obs_data_t

class HotkeyManager;

class SoundboardStore {
public:
	// All three pointers are required and must remain valid for the
	// lifetime of this object.

	SoundboardStore() = default;

	// Initialize with valid pointers
	void init(QListWidget *list, HotkeyManager *hotkeys);

	// Serialize the current clip list (including per-clip hotkey
	// bindings and the global Stop/Play hotkeys) into data.
	void save(obs_data_t *data) const;

	// Clear the list, then rebuild it from data.
	// Passing nullptr is safe and results in an empty list.
	void load(obs_data_t *data);

private:
	QListWidget *list = nullptr;
	HotkeyManager *hotkeys = nullptr;

	QListWidgetItem *lastPlayedItem = nullptr;
};