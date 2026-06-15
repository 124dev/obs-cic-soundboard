// ============================================================
// Soundboard.cpp
// Implementation of the Soundboard dock widget.
//
// Item data role convention (QListWidgetItem):
//   Qt::UserRole     → absolute file path  (QString)
//   Qt::UserRole + 1 → stable UUID string  (QString)  ← hotkey map key
// ============================================================

#include "Soundboard.hpp"
#include "AddSoundDialog.hpp"

#include <obs-module.h>       // blog(), obs_data_*, obs_source_*
#include <obs-frontend-api.h> // obs_hotkey_register_frontend(), obs_set_output_source()

#include <QFileDialog>
#include <QListWidgetItem>
#include <QUuid> // QUuid::createUuid() – generate stable clip IDs

// Forward-declare the global pointer defined in plugin-main.cpp.
// Hotkey callbacks use it to reach the Soundboard instance.
extern Soundboard *g_soundboard;

// ── Construction / destruction ───────────────────────────────

Soundboard::Soundboard(QWidget *parent) : QWidget(parent)
{
	// Build the UI from the Qt Designer-generated class.
	ui.setupUi(this);

	// ── Button connections ───────────────────────────────────
	connect(ui.btnAdd, &QPushButton::clicked, this, &Soundboard::onAddClicked);
	connect(ui.btnPlay, &QPushButton::clicked, this, &Soundboard::onPlayClicked);
	connect(ui.btnStop, &QPushButton::clicked, this, &Soundboard::onStopClicked);
	connect(ui.btnRemove, &QPushButton::clicked, this, &Soundboard::onRemoveClicked);

	// ── List widget connections ──────────────────────────────
	// Enable right-click context menu on the list.
	ui.list->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui.list, &QListWidget::customContextMenuRequested, this, &Soundboard::onContextMenuRequested);

	// Single-clicking a clip immediately plays it.
	connect(ui.list, &QListWidget::itemClicked, this, &Soundboard::onItemClicked);
}

Soundboard::~Soundboard()
{
	// Clean up all registered OBS hotkeys before the widget is destroyed.
	unregisterAllHotkeys();
}

// ── Context menu ─────────────────────────────────────────────

void Soundboard::onContextMenuRequested(const QPoint &pos)
{
	// Find which item was right-clicked; do nothing if the click was on empty space.
	QListWidgetItem *item = ui.list->itemAt(pos);
	if (!item)
		return;

	// Build and display a small context menu with "Edit" and "Remove".
	QMenu menu(this);
	QAction *editAction = menu.addAction(tr("Edit"));
	QAction *removeAction = menu.addAction(tr("Remove"));

	// exec() blocks until the user picks an action (or dismisses the menu).
	QAction *chosen = menu.exec(ui.list->mapToGlobal(pos));

	if (chosen == editAction) {
		onEditItem(item);
	} else if (chosen == removeAction) {
		// Unregister the hotkey first, then remove the item from the list.
		unregisterHotkey(item);
		delete ui.list->takeItem(ui.list->row(item));
	}
	// If chosen == nullptr the user dismissed the menu; do nothing.
}

// ── Edit existing clip ───────────────────────────────────────

void Soundboard::onEditItem(QListWidgetItem *item)
{
	if (!item)
		return;

	// Open the dialog pre-populated with the existing clip's name and path.
	AddSoundDialog dialog(this);
	dialog.setDisplayName(item->text());
	dialog.setFilePath(item->data(Qt::UserRole).toString());

	// Bail out if the user cancelled.
	if (dialog.exec() != QDialog::Accepted)
		return;

	QString newName = dialog.getDisplayName();
	QString newPath = dialog.getFilePath();

	// Guard against the user clearing either field before accepting.
	if (newName.isEmpty() || newPath.isEmpty())
		return;

	// ── Preserve hotkey bindings across the re-register ─────
	// The UID stays the same so existing key bindings remain valid.
	// We save them now, before unregistering, and restore them after.
	const QString uid = item->data(Qt::UserRole + 1).toString();
	const obs_hotkey_id oldHkId = hotkeyMap.value(uid, OBS_INVALID_HOTKEY_ID);

	// obs_hotkey_save() returns nullptr if the hotkey has no bindings.
	OBSDataArrayAutoRelease savedBindings = nullptr;
	if (oldHkId != OBS_INVALID_HOTKEY_ID)
		savedBindings = obs_hotkey_save(oldHkId);

	// ── Update the list item ─────────────────────────────────
	item->setText(newName);
	item->setData(Qt::UserRole, newPath);
	// Qt::UserRole + 1 (the UID) is intentionally left unchanged.

	// Re-register the hotkey under the same UID but with the new name/path.
	unregisterHotkey(item);
	registerHotkey(item);

	// ── Restore previously bound keys ───────────────────────
	if (savedBindings) {
		const obs_hotkey_id newHkId = hotkeyMap.value(uid, OBS_INVALID_HOTKEY_ID);
		if (newHkId != OBS_INVALID_HOTKEY_ID)
			obs_hotkey_load(newHkId, savedBindings);
	}
}

// ── Playback ─────────────────────────────────────────────────

// Called when the user single-clicks a clip in the list.
void Soundboard::onItemClicked(QListWidgetItem *item)
{
	if (!item)
		return;

	playFile(item->data(Qt::UserRole).toString());
}

void Soundboard::playFile(const QString &path)
{
	if (path.isEmpty())
		return;

	// If the same file is already loaded, just restart it instead of
	// reconfiguring the source (avoids unnecessary source updates).
	if (currentFile == path && mediaSource) {
		obs_source_media_restart(mediaSource);
		return;
	}

	currentFile = path;

	// Create the OBS source if it doesn't exist yet.
	ensureSource();

	if (!mediaSource)
		return; // ensureSource() already logged the error

	// Configure the ffmpeg_source to play the chosen local file.
	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_string(settings, "local_file", path.toUtf8().constData());
	obs_data_set_bool(settings, "is_local_file", true);
	obs_data_set_bool(settings, "looping", false);

	obs_source_update(mediaSource, settings); // Apply settings
	obs_source_media_restart(mediaSource);    // Start/restart playback
}

// ── Button slots ─────────────────────────────────────────────

void Soundboard::onAddClicked()
{
	// Open a blank AddSoundDialog so the user can enter a name and pick a file.
	AddSoundDialog dialog(this);

	if (dialog.exec() != QDialog::Accepted) {
		blog(LOG_INFO, "[Soundboard] Add dialog cancelled");
		return;
	}

	const QString name = dialog.getDisplayName();
	const QString path = dialog.getFilePath();

	// Silently ignore if either field was left empty.
	if (name.isEmpty() || path.isEmpty())
		return;

	// Create the list item and attach the path and a fresh stable UUID.
	auto *item = new QListWidgetItem(name);
	item->setData(Qt::UserRole, path);
	item->setData(Qt::UserRole + 1, QUuid::createUuid().toString());

	ui.list->addItem(item);
	registerHotkey(item); // Register an OBS hotkey for this new clip

	blog(LOG_INFO, "[Soundboard] Added clip: %s", name.toUtf8().constData());
}

void Soundboard::onPlayClicked()
{
	// Play whichever clip is currently highlighted in the list.
	auto *item = ui.list->currentItem();
	if (!item)
		return;

	playFile(item->data(Qt::UserRole).toString());
}

void Soundboard::onStopClicked()
{
	// Immediately stop the media source (if one exists).
	if (mediaSource)
		obs_source_media_stop(mediaSource);
}

void Soundboard::onRemoveClicked()
{
	auto *item = ui.list->currentItem();
	if (!item)
		return;

	// Unregister the hotkey before removing the item to avoid dangling pointers.
	unregisterHotkey(item);
	delete ui.list->takeItem(ui.list->row(item));
}

// ── Hotkey registration ───────────────────────────────────────

void Soundboard::registerHotkey(QListWidgetItem *item)
{
	if (!item)
		return;

	const QString uid = item->data(Qt::UserRole + 1).toString();
	const QString name = item->text();
	const QString path = item->data(Qt::UserRole).toString();

	// Store the path in hotkeyPaths BEFORE passing its address to OBS,
	// so the QString lives as long as the hotkey is registered.
	hotkeyPaths.insert(uid, path);

	// OBS requires plain std::string for the hotkey name/description.
	const std::string hotkeyName = ("soundboard.play." + uid).toStdString();
	const std::string hotkeyDesc = ("Soundboard: " + name).toStdString();

	// Register a frontend hotkey. The callback receives a pointer to the
	// path QString stored in hotkeyPaths (stable address, no heap alloc needed).
	obs_hotkey_id id = obs_hotkey_register_frontend(
		hotkeyName.c_str(), hotkeyDesc.c_str(),
		[](void *data, obs_hotkey_id /*id*/, obs_hotkey_t * /*hotkey*/, bool pressed) {
			// OBS fires the callback on both key-down and key-up; only act on press.
			if (!pressed)
				return;

			// g_soundboard is the global plugin instance (plugin-main.cpp).
			if (g_soundboard)
				g_soundboard->playFile(*static_cast<QString *>(data));
		},
		&hotkeyPaths[uid] // Pointer into the QMap value — stable until removed
	);

	hotkeyMap.insert(uid, id);
}

void Soundboard::unregisterHotkey(QListWidgetItem *item)
{
	if (!item)
		return;

	const QString uid = item->data(Qt::UserRole + 1).toString();

	auto it = hotkeyMap.find(uid);
	if (it == hotkeyMap.end())
		return; // Hotkey was never registered (or already removed)

	// Unregister with OBS — after this the callback will never fire again,
	// so it is safe to remove the path from hotkeyPaths immediately.
	obs_hotkey_unregister(it.value());
	hotkeyMap.erase(it);
	hotkeyPaths.remove(uid);
}

void Soundboard::unregisterAllHotkeys()
{
	// Iterate the visible list; each item has a UID that maps to a hotkey.
	for (int i = 0; i < ui.list->count(); ++i)
		unregisterHotkey(ui.list->item(i));
}

// ── OBS source lifecycle ──────────────────────────────────────

void Soundboard::ensureSource()
{
	blog(LOG_INFO, "[Soundboard] ensureSource()");

	if (mediaSource) {
		// Source already exists — just make sure it is on output slot 63.
		blog(LOG_INFO, "[Soundboard] Reusing existing source");
		obs_set_output_source(63, mediaSource);
		return;
	}

	// Try to find an already-registered source with our well-known name
	// (e.g. if a previous scene collection left it behind).
	obs_source_t *existing = obs_get_source_by_name("Soundboard");

	if (existing) {
		// obs_get_source_by_name() adds a reference; OBSSource takes ownership.
		mediaSource = existing;
		blog(LOG_INFO, "[Soundboard] Adopted existing source");
	} else {
		// Create a new hidden ffmpeg_source for audio-only playback.
		mediaSource = obs_source_create("ffmpeg_source", "Soundboard", nullptr, nullptr);

		if (!mediaSource) {
			blog(LOG_ERROR, "[Soundboard] Failed to create ffmpeg_source");
			return;
		}

		blog(LOG_INFO, "[Soundboard] Created new source");
	}

	// Route audio to both the monitoring device and the stream/recording output.
	obs_source_set_monitoring_type(mediaSource, OBS_MONITORING_TYPE_MONITOR_AND_OUTPUT);

	// Hide from the scene tree — it is an internal-use source only.
	obs_source_set_hidden(mediaSource, true);

	// Attach to output channel 63 (a spare channel conventionally used by plugins).
	obs_set_output_source(63, mediaSource);

	blog(LOG_INFO, "[Soundboard] Source attached to output slot 63");
}

void Soundboard::clearSource()
{
	blog(LOG_INFO, "[Soundboard] clearSource()");

	if (mediaSource) {
		// Detach from the output channel first, then release our reference.
		obs_set_output_source(63, nullptr);
		mediaSource = nullptr; // OBSSource RAII destructor releases the ref
		blog(LOG_INFO, "[Soundboard] Source detached and released");
	}

	// Reset the current-file tracker so the next playFile() call won't
	// take the "same file, just restart" shortcut with a dead source.
	currentFile.clear();
}

// ── Save / Load ──────────────────────────────────────────────

// Serialize the full clip list (names, paths, UIDs, hotkey bindings) into
// an obs_data object that OBS will embed in its scene-collection JSON.
void Soundboard::saveData(obs_data_t *data)
{
	OBSDataArrayAutoRelease array = obs_data_array_create();

	for (int i = 0; i < ui.list->count(); ++i) {
		QListWidgetItem *item = ui.list->item(i);

		OBSDataAutoRelease obj = obs_data_create();

		// Store the display name.
		obs_data_set_string(obj, "name", item->text().toUtf8().constData());

		// Store the absolute path.
		obs_data_set_string(obj, "path", item->data(Qt::UserRole).toString().toUtf8().constData());

		// Store the stable UID so hotkey bindings survive restarts.
		obs_data_set_string(obj, "uid", item->data(Qt::UserRole + 1).toString().toUtf8().constData());

		// Persist the key bindings (if any are configured for this clip).
		const obs_hotkey_id hkId =
			hotkeyMap.value(item->data(Qt::UserRole + 1).toString(), OBS_INVALID_HOTKEY_ID);

		if (hkId != OBS_INVALID_HOTKEY_ID) {
			OBSDataArrayAutoRelease bindings = obs_hotkey_save(hkId);
			obs_data_set_array(obj, "hotkey_bindings", bindings);
		}

		obs_data_array_push_back(array, obj);
	}

	// Write the array under the "sounds" key in the parent data object.
	obs_data_set_array(data, "sounds", array);
}

// Reconstruct the clip list from the obs_data written by saveData().
// Called by the OBS save/load callback in plugin-main.cpp.
void Soundboard::loadData(obs_data_t *data)
{
	// Clear any existing clips and their hotkeys before loading fresh data.
	unregisterAllHotkeys();
	ui.list->clear();

	OBSDataArrayAutoRelease array = obs_data_get_array(data, "sounds");
	if (!array)
		return; // No saved sounds yet (e.g. first run)

	const size_t count = obs_data_array_count(array);

	for (size_t i = 0; i < count; ++i) {
		OBSDataAutoRelease obj = obs_data_array_item(array, i);

		const QString name = QString::fromUtf8(obs_data_get_string(obj, "name"));
		const QString path = QString::fromUtf8(obs_data_get_string(obj, "path"));
		QString uid = QString::fromUtf8(obs_data_get_string(obj, "uid"));

		// Generate a fresh UID for entries that pre-date the UID field
		// (e.g. data saved by an older version of the plugin).
		if (uid.isEmpty())
			uid = QUuid::createUuid().toString();

		// Rebuild the list item with the same roles used when adding.
		auto *item = new QListWidgetItem(name);
		item->setData(Qt::UserRole, path);
		item->setData(Qt::UserRole + 1, uid);
		ui.list->addItem(item);

		// Re-register the hotkey so the UID mapping is live again.
		registerHotkey(item);

		// Restore previously saved key bindings (if any).
		OBSDataArrayAutoRelease bindings = obs_data_get_array(obj, "hotkey_bindings");
		if (bindings) {
			const obs_hotkey_id hkId = hotkeyMap.value(uid, OBS_INVALID_HOTKEY_ID);
			if (hkId != OBS_INVALID_HOTKEY_ID)
				obs_hotkey_load(hkId, bindings);
		}
	}

	blog(LOG_INFO, "[Soundboard] Loaded %zu clip(s)", count);
}
