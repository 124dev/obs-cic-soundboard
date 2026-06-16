#include "Soundboard.hpp"
#include "ui/AddSoundDialog.hpp"

#include <obs-module.h> // blog()

#include <QUuid> // QUuid::createUuid()

// ── Construction / destruction ───────────────────────────────

Soundboard::Soundboard(QWidget *parent)
	: QWidget(parent)
	  // HotkeyManager needs 'this' to pass as callback data for the
	  // global Stop / Play-Selected hotkeys, so it is constructed after
	  // QWidget but before the store (which only needs list + hotkeys).
	  ,
	  hotkeys(this),
	  source(),
	  store(),
	  lastPlayedItem(nullptr)
// NOTE: ui.list is valid only after setupUi(), so the store's
// list pointer is set correctly once setupUi() runs below.
// Because store is constructed before setupUi(), we rely on the
// fact that store only dereferences ui.list during save/load calls,
// never during construction. This is safe.
// UPDATE: We now call store.init() after setupUi() to ensure
// ui.list is properly initialized.
{
	ui.setupUi(this);

	// Initialize store with valid pointers now that ui is set up
	store.init(ui.list, &hotkeys);

	// ── Button connections ───────────────────────────────────
	connect(ui.btnAdd, &QPushButton::clicked, this, &Soundboard::onAddClicked);
	connect(ui.btnPlay, &QPushButton::clicked, this, &Soundboard::onPlayClicked);
	connect(ui.btnStop, &QPushButton::clicked, this, &Soundboard::onStopClicked);
	connect(ui.btnRemove, &QPushButton::clicked, this, &Soundboard::onRemoveClicked);

	// ── List connections ─────────────────────────────────────
	ui.list->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui.list, &QListWidget::customContextMenuRequested, this, &Soundboard::onContextMenuRequested);
	connect(ui.list, &QListWidget::itemClicked, this, &Soundboard::onItemClicked);
}

Soundboard::~Soundboard()
{
	// HotkeyManager destructor unregisters all hotkeys.
	// SourceManager's OBSSource RAII wrapper releases the source ref.
	// Explicit clearSource() here gives us the log message and output-
	// channel detach before the OBSSource wrapper fires.
	source.clearSource();
}

// ── Public delegates ──────────────────────────────────────────

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

// ── UI slots ──────────────────────────────────────────────────

void Soundboard::onItemClicked(QListWidgetItem *item)
{
	if (!item)
		return;

	lastPlayedItem = item;
	source.playFile(item->data(Qt::UserRole).toString());
}

void Soundboard::onAddClicked()
{
	AddSoundDialog dialog(this);

	if (dialog.exec() != QDialog::Accepted) {
		blog(LOG_INFO, "[Soundboard] Add dialog cancelled");
		return;
	}

	const QString name = dialog.getDisplayName();
	const QString path = dialog.getFilePath();

	if (name.isEmpty() || path.isEmpty())
		return;

	auto *item = new QListWidgetItem(name);
	item->setData(Qt::UserRole, path);
	item->setData(Qt::UserRole + 1, QUuid::createUuid().toString());

	ui.list->addItem(item);
	hotkeys.registerHotkey(item);

	blog(LOG_INFO, "[Soundboard] Added clip: %s", name.toUtf8().constData());
}

void Soundboard::onPlayClicked()
{
	auto *item = lastPlayedItem;
	if (!item)
		return;

	source.playFile(item->data(Qt::UserRole).toString());
}

void Soundboard::onStopClicked()
{
	source.stop();
}

void Soundboard::onRemoveClicked()
{
	auto *item = ui.list->currentItem();
	if (!item)
		return;

	if (lastPlayedItem == item) {
		lastPlayedItem = nullptr;
	}

	hotkeys.unregisterHotkey(item);
	delete ui.list->takeItem(ui.list->row(item));
}

// ── Context menu ─────────────────────────────────────────────

void Soundboard::onContextMenuRequested(const QPoint &pos)
{
	QListWidgetItem *item = ui.list->itemAt(pos);
	if (!item)
		return;

	QMenu menu(this);
	QAction *editAction = menu.addAction(tr("Edit"));
	QAction *removeAction = menu.addAction(tr("Remove"));

	QAction *chosen = menu.exec(ui.list->mapToGlobal(pos));

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

// ── Edit existing clip ───────────────────────────────────────

void Soundboard::onEditItem(QListWidgetItem *item)
{
	if (!item)
		return;

	AddSoundDialog dialog(this);
	dialog.setDisplayName(item->text());
	dialog.setFilePath(item->data(Qt::UserRole).toString());

	if (dialog.exec() != QDialog::Accepted)
		return;

	const QString newName = dialog.getDisplayName();
	const QString newPath = dialog.getFilePath();

	if (newName.isEmpty() || newPath.isEmpty())
		return;

	// Save the current key binding so we can restore it after
	// re-registering the hotkey under the new name / path.
	const QString uid = item->data(Qt::UserRole + 1).toString();
	const obs_hotkey_id oldId = hotkeys.idForUid(uid);

	OBSDataArrayAutoRelease savedBindings = nullptr;
	if (oldId != OBS_INVALID_HOTKEY_ID)
		savedBindings = obs_hotkey_save(oldId);

	item->setText(newName);
	item->setData(Qt::UserRole, newPath);

	hotkeys.unregisterHotkey(item);
	hotkeys.registerHotkey(item);

	if (savedBindings) {
		const obs_hotkey_id newId = hotkeys.idForUid(uid);
		if (newId != OBS_INVALID_HOTKEY_ID)
			obs_hotkey_load(newId, savedBindings);
	}
}

// ── Helper to find item by path ─────────────────────────────

QListWidgetItem *Soundboard::findItemByPath(const QString &path) const
{
	for (int i = 0; i < ui.list->count(); ++i) {
		auto *item = ui.list->item(i);
		if (item->data(Qt::UserRole).toString() == path) {
			return item;
		}
	}
	return nullptr;
}