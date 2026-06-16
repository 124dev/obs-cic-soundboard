// ============================================================
// AddSoundDialog.cpp
// Implementation of the Add / Edit sound dialog.
// ============================================================

#include "AddSoundDialog.hpp"

#include <QApplication> // QApplication::activeWindow()
#include <QFileDialog>  // QFileDialog::getOpenFileName()
#include <QFileInfo>    // QFileInfo::completeBaseName()

// ── Constructor ──────────────────────────────────────────────

AddSoundDialog::AddSoundDialog(QWidget *parent) : QDialog(parent)
{
	ui.setupUi(this);

	// "Browse…" button → open file picker.
	connect(ui.btnBrowse, &QPushButton::clicked, this, &AddSoundDialog::onBrowseClicked);

	// Auto-fill the display name from the chosen file's base name,
	// but only when the name field is still empty (don't overwrite user input).
	connect(ui.txtFilePath, &QLineEdit::textChanged, this, [this](const QString &path) {
		if (ui.txtName->text().isEmpty() && !path.isEmpty()) {
			QFileInfo info(path);
			// e.g. "my_sound" from "my_sound.mp3"
			ui.txtName->setText(info.completeBaseName());
		}
	});

	// Note: OK / Cancel button-box connections are wired in the .ui file
	// (accepted() → accept(), rejected() → reject()), so nothing extra is
	// needed here.
}

// ── Destructor ───────────────────────────────────────────────

AddSoundDialog::~AddSoundDialog() = default;

// ── Getters ──────────────────────────────────────────────────

QString AddSoundDialog::getDisplayName() const
{
	return ui.txtName->text();
}

QString AddSoundDialog::getFilePath() const
{
	return ui.txtFilePath->text();
}

// ── Setters ──────────────────────────────────────────────────

void AddSoundDialog::setDisplayName(const QString &name)
{
	ui.txtName->setText(name);
}

// Pre-populate the file path field (edit mode).
// The textChanged signal above won't auto-fill the name here because
// the name field is already non-empty when editing.
void AddSoundDialog::setFilePath(const QString &path)
{
	ui.txtFilePath->setText(path);
}

// ── Slots ────────────────────────────────────────────────────

void AddSoundDialog::onBrowseClicked()
{
	// Use the active window as parent so the picker appears above any
	// dock/panel. Using 'this' can sometimes fail when the dialog is
	// embedded in a dock.
	const QString path = QFileDialog::getOpenFileName(QApplication::activeWindow(), tr("Select Audio File"),
							  QString(), // start in last-used directory (Qt default)
							  tr("Audio Files (*.mp3 *.wav *.ogg *.flac *.aac)"));

	if (!path.isEmpty())
		ui.txtFilePath->setText(path);
}