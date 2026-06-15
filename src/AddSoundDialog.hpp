#pragma once

// ============================================================
// AddSoundDialog.hpp
// Modal dialog for adding OR editing a soundboard clip entry.
//
// Usage (add):
//   AddSoundDialog dlg(this);
//   if (dlg.exec() == QDialog::Accepted) { … }
//
// Usage (edit – pre-populate fields):
//   AddSoundDialog dlg(this);
//   dlg.setDisplayName(item->text());
//   dlg.setFilePath(item->data(Qt::UserRole).toString());
//   if (dlg.exec() == QDialog::Accepted) { … }
// ============================================================

#include <QDialog>
#include <QString>
#include "ui_AddSoundDialog.h"

class AddSoundDialog : public QDialog {
	Q_OBJECT

public:
	// ── Construction / destruction ──────────────────────────
	explicit AddSoundDialog(QWidget *parent = nullptr);
	~AddSoundDialog();

	// ── Getters (read result after Accepted) ────────────────
	QString getDisplayName() const; // Contents of the "Display Name" field
	QString getFilePath() const;    // Contents of the "Audio File" field

	// ── Setters (pre-populate for edit mode) ────────────────
	void setDisplayName(const QString &name); // Pre-fill the name field
	void setFilePath(const QString &path);    // Pre-fill the file-path field

private slots:
	// Opens a file-picker dialog; populates txtFilePath (and auto-fills txtName).
	void onBrowseClicked();

private:
	Ui::AddSoundDialog ui; // Generated form class from ui_AddSoundDialog.h
};
