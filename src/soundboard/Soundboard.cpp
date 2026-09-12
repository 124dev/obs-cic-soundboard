#include "Soundboard.hpp"
#include "ui/AddSoundDialog.hpp"

#include <obs-module.h> 

#include <QUuid> 


Soundboard::Soundboard(QWidget *parent)
	: QWidget(parent)
	  ,
	  hotkeys(this),
	  source(),
	  store(),
	  lastPlayedItem(nullptr)
{
	// Initialize the user interface
	ui.setupUi(this);

	// Initialize the save/load manager
	store.init(ui.list, &hotkeys);
	// Connect button signals to their respective slots
	connect(ui.btnAdd, &QPushButton::clicked, this, &Soundboard::onAddClicked);
	connect(ui.btnPlay, &QPushButton::clicked, this, &Soundboard::onPlayClicked);
	connect(ui.btnStop, &QPushButton::clicked, this, &Soundboard::onStopClicked);
	connect(ui.btnRemove, &QPushButton::clicked, this, &Soundboard::onRemoveClicked);
	// Enable right-click context menu for the list
	ui.list->setContextMenuPolicy(Qt::CustomContextMenu);

	// Connect list events (context menu and item click)
	connect(ui.list, &QListWidget::customContextMenuRequested, this, &Soundboard::onContextMenuRequested);
	connect(ui.list, &QListWidget::itemClicked, this, &Soundboard::onItemClicked);
}

Soundboard::~Soundboard()
{
	    // Remove the OBS media source when the soundboard is destroyed
	source.clearSource();
}

// Wrapper functions that delegate work to helper classes

void Soundboard::saveData(obs_data_t *data)
{
	store.save(data);
}

void Soundboard::loadData(obs_data_t *data)
{
	store.load(data);
}

void Soundboard::ensureSource()
{
	source.ensureSource();
}

void Soundboard::clearSource()
{
	source.clearSource();
}

// ui events

// if item is clicked, immedietly play the saved file 
void Soundboard::onItemClicked(QListWidgetItem *item)
{
	if (!item)
		return;

	lastPlayedItem = item;
	source.playFile(item->data(Qt::UserRole).toString());
}

// add button
void Soundboard::onAddClicked()
{

	// show dialog (ui)
	AddSoundDialog dialog(this);

	if (dialog.exec() != QDialog::Accepted) {
		blog(LOG_INFO, "[Soundboard] Add dialog cancelled");
		return;
	}

	const QString name = dialog.getDisplayName();
	const QString path = dialog.getFilePath();
	// check if either is empty
	if (name.isEmpty() || path.isEmpty())
		return;
	// create a new item in the list using the name
	auto *item = new QListWidgetItem(name);
	// store the item data
	item->setData(Qt::UserRole, path);
	// add a unique Uuid for each item
	item->setData(Qt::UserRole + 1, QUuid::createUuid().toString());
	// add the item in the list
	ui.list->addItem(item);
	// Register  and make its own Hotkey
	hotkeys.registerHotkey(item);

	blog(LOG_INFO, "[Soundboard] Added clip: %s", name.toUtf8().constData());
}

void Soundboard::onPlayClicked()
{

	// check if last played item
	auto *item = lastPlayedItem;
	// if not return
	if (!item)
		return;

	// play the item
	source.playFile(item->data(Qt::UserRole).toString());
}

// stop playing if stopped button is clicked
void Soundboard::onStopClicked()
{
	source.stop();
}

// yeah, remove
void Soundboard::onRemoveClicked()
{
	// get selected item
	auto *item = ui.list->currentItem();
	// if not, return
	if (!item)
		return;
	// Check if it is currently the last played sound, then remove it from it
	if (lastPlayedItem == item) {
		lastPlayedItem = nullptr;
	}
	// unregister the item hotkey
	hotkeys.unregisterHotkey(item);

	// delete the item from the list
	delete ui.list->takeItem(ui.list->row(item));
}



//  right click event
void Soundboard::onContextMenuRequested(const QPoint &pos)
{
	// get clicked item
	QListWidgetItem *item = ui.list->itemAt(pos);
	// if no item, return
	if (!item)
		return;
	// create and show a menu
	QMenu menu(this);
	QAction *editAction = menu.addAction(tr("Edit"));
	QAction *removeAction = menu.addAction(tr("Remove"));
	QAction *chosen = menu.exec(ui.list->mapToGlobal(pos));
	// if chosen(s)
	if (chosen == editAction) {
		onEditItem(item);
	} else if (chosen == removeAction) {
		if (lastPlayedItem == item) {
			lastPlayedItem = nullptr;
		}
		hotkeys.unregisterHotkey(item);
		delete ui.list->takeItem(ui.list->row(item));
	}
}

// edit existing clips

void Soundboard::onEditItem(QListWidgetItem *item)
{
	// return if no item selected
	if (!item)
		return;
	// show add  sound dialog with dialog contents of the selected item
	AddSoundDialog dialog(this);
	dialog.setDisplayName(item->text());
	dialog.setFilePath(item->data(Qt::UserRole).toString());
	// Stop if the user cancels the dialog
	if (dialog.exec() != QDialog::Accepted)
		return;
	// Get the updated values entered by the user
	const QString newName = dialog.getDisplayName();
	const QString newPath = dialog.getFilePath();
	// Ignore the update if either field is empty
	if (newName.isEmpty() || newPath.isEmpty())
		return;

	// Get the unique ID associated with this sound
	const QString uid = item->data(Qt::UserRole + 1).toString();
	// Retrieve the current OBS hotkey ID for this sound
	const obs_hotkey_id oldId = hotkeys.idForUid(uid);
	// Save the user's existing hotkey bindings before re-registering
	OBSDataArrayAutoRelease savedBindings = nullptr;
	if (oldId != OBS_INVALID_HOTKEY_ID)
		savedBindings = obs_hotkey_save(oldId);
	// Update the item's displayed name and stored file path
	item->setText(newName);
	item->setData(Qt::UserRole, newPath);
	// Re-register the hotkey so it points to the updated sound
	hotkeys.unregisterHotkey(item);
	hotkeys.registerHotkey(item);
	// Restore the user's previous hotkey bindings
	if (savedBindings) {
		const obs_hotkey_id newId = hotkeys.idForUid(uid);
		if (newId != OBS_INVALID_HOTKEY_ID)
			obs_hotkey_load(newId, savedBindings);
	}
}

QListWidgetItem *Soundboard::findItemByPath(const QString &path) const
{
	// Search through every item in the list
	for (int i = 0; i < ui.list->count(); ++i) {
		auto *item = ui.list->item(i);
		// Return the matching item if its stored path matches
		if (item->data(Qt::UserRole).toString() == path) {
			return item;
		}
	}
	// No matching item was found
	return nullptr;
}
