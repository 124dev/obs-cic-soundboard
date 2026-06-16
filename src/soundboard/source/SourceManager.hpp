#pragma once

// ============================================================
// SourceManager.hpp
// Manages the hidden OBS ffmpeg_source used for audio playback.
//
// Responsibilities:
//   - Create or adopt the "Soundboard" ffmpeg_source on demand
//   - Attach / detach the source from output channel 63
//   - Configure the source and trigger playback of a given file
//   - Stop playback
//   - Release all source references cleanly on teardown
//
// The source is kept hidden from the OBS scene list and is
// permanently routed through output channel 63 while active.
// ============================================================

#include <QString>
#include <obs.hpp> // OBSSource RAII wrapper

class SourceManager {
public:
    SourceManager()  = default;
    ~SourceManager() = default; 
                               
    // ── Source lifecycle ─────────────────────────────────────
    // Create (or adopt an existing) "Soundboard" ffmpeg_source and
    // attach it to output channel 63.  Safe to call multiple times.
    void ensureSource();

    // Detach from output channel 63, release the source reference,
    // and reset the current-file tracker.
    void clearSource();

    // ── Playback ─────────────────────────────────────────────
    // Configure the source with the given file and start playback.
    // If the file is already loaded, just restart it.
    void playFile(const QString &path);

    // Stop whatever is currently playing.
    void stop();

    // ── State query ──────────────────────────────────────────
    bool hasSource() const { return mediaSource != nullptr; }

private:
    OBSSource mediaSource = nullptr; // RAII wrapper; null when no source exists
    QString   currentFile;           // Path most recently sent to the source
};
