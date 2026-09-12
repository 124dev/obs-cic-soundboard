#pragma once


#include <QWidget>
#include <QListWidgetItem>
#include <QMap>
#include <QString>
#include <QMenu>
#include <QAction>

#include <obs.hpp>
#include "ui_Soundboard.h"


#include "hotkeys/HotkeyManager.hpp"
#include "source/SourceManager.hpp"
#include "persistence/SoundboardStore.hpp"  



class Soundboard : public QWidget {
    Q_OBJECT

public:
    explicit Soundboard(QWidget*parent = nullptr);
    ~Soundboard();


    QListWidget *getList() { return ui.list; }
    SourceManager &getSourceManager() { return source; }
    
    QListWidgetItem *getLastPlayedItem() const { return lastPlayedItem; }
    void setLastPlayedItem(QListWidgetItem *item) { lastPlayedItem = item; }
    QListWidgetItem *findItemByPath(const QString &path) const;
 
    void saveData(obs_data_t *data); 
    void loadData(obs_data_t *data); 
 
    void ensureSource(); 
    void clearSource();  

private slots:
    void onItemClicked(QListWidgetItem *item);      // click → play
    void onAddClicked();                            // Add button
    void onPlayClicked();                           // Play button
    void onStopClicked();                           // Stop button
    void onRemoveClicked();                         // Remove button
    void onContextMenuRequested(const QPoint &pos); // right-click list
    void onEditItem(QListWidgetItem *item);         // Edit from context menu
 
private:
    // User interface generated from Qt Designer
    Ui::Soundboard ui; 
 
    HotkeyManager  hotkeys; // per-clip + global hotkey registration
    SourceManager  source;  // Handles OBS media source creation and playback
    SoundboardStore store;  // clip list serialization / deserialization
    
    // track last played item
    QListWidgetItem *lastPlayedItem = nullptr;
};
