#pragma once


#include <QString>
#include <obs.hpp> 

class SourceManager {
public:
    SourceManager()  = default;
    ~SourceManager() = default; 
                               
    void ensureSource();

    void clearSource();


    void playFile(const QString &path);

    // Stop whatever is currently playing.
    void stop();
    // mute source
    void mute();

    // void source unmute
    void unmute();

    // restart source
    void restart();
    // ── State query ──────────────────────────────────────────
    bool hasSource() const { return mediaSource != nullptr; }

private:
    OBSSource mediaSource = nullptr; // RAII wrapper; null when no source exists
    QString   currentFile;           // Path most recently sent to the source
};
