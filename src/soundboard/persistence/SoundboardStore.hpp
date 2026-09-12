#pragma once



#include <QListWidget>
#include <obs-module.h> // obs_data_t

class HotkeyManager;

class SoundboardStore {
public:

	SoundboardStore() = default;
	void init(QListWidget *list, HotkeyManager *hotkeys);


	void save(obs_data_t *data) const;


	void load(obs_data_t *data);

private:
	QListWidget *list = nullptr;
	HotkeyManager *hotkeys = nullptr;

	QListWidgetItem *lastPlayedItem = nullptr;
};
