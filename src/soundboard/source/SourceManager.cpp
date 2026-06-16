// ============================================================
// SourceManager.cpp
// Implementation of OBS source creation, attachment, and playback.
// ============================================================

#include "SourceManager.hpp"

#include <obs-module.h>       // blog(), obs_data_*, obs_source_*
#include <obs-frontend-api.h> // obs_set_output_source()

// ── Source lifecycle ──────────────────────────────────────────

void SourceManager::ensureSource()
{
	blog(LOG_INFO, "[Soundboard] ensureSource()");

	// Already have a live source — just re-attach it to the output slot.
	if (mediaSource) {
		blog(LOG_INFO, "[Soundboard] Reusing existing source");
		obs_set_output_source(63, mediaSource);
		return;
	}

	// Try to adopt a source that was left over from a previous session
	// (e.g. after a scene-collection switch that didn't fully clean up).
	obs_source_t *existing = obs_get_source_by_name("Soundboard");

	if (existing) {
		// obs_get_source_by_name adds a reference; OBSSource takes ownership.
		mediaSource = existing;
		obs_source_release(existing); // balance the extra ref from get_by_name
		blog(LOG_INFO, "[Soundboard] Adopted existing source");
	} else {
		// Create a brand-new hidden ffmpeg_source.
		mediaSource = obs_source_create("ffmpeg_source", "Soundboard", nullptr, nullptr);

		if (!mediaSource) {
			blog(LOG_ERROR, "[Soundboard] Failed to create ffmpeg_source");
			return;
		}

		blog(LOG_INFO, "[Soundboard] Created new source");
	}

	// Route audio both to the desktop monitor and the output mix.
	obs_source_set_monitoring_type(mediaSource, OBS_MONITORING_TYPE_MONITOR_AND_OUTPUT);

	// Hide from the sources panel so it doesn't clutter the scene.
	obs_source_set_hidden(mediaSource, true);

	// Attach to output channel 63 (a spare channel reserved for plugins).
	obs_set_output_source(63, mediaSource);

	blog(LOG_INFO, "[Soundboard] Source attached to output slot 63");
}

void SourceManager::clearSource()
{
	blog(LOG_INFO, "[Soundboard] clearSource()");

	// Detach from the output channel first so OBS stops mixing it.
	obs_set_output_source(63, nullptr);

	if (mediaSource) {
		// OBSSource::Get() returns the raw pointer without adding a ref.
		obs_source_t *raw = mediaSource.Get();
		mediaSource = nullptr; // drop the OBSSource RAII ref

		// Release the ref we held on behalf of the source.
		if (raw)
			obs_source_release(raw);

		blog(LOG_INFO, "[Soundboard] Source completely detached and destroyed");
	}

	currentFile.clear();
}

// ── Playback ──────────────────────────────────────────────────

void SourceManager::playFile(const QString &path)
{
	if (path.isEmpty())
		return;

	// If the same file is already loaded, just restart from the beginning.
	if (currentFile == path && mediaSource) {
		obs_source_media_restart(mediaSource);
		return;
	}

	currentFile = path;
	ensureSource();

	if (!mediaSource)
		return;

	// Push new file settings to the source, then restart.
	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_string(settings, "local_file", path.toUtf8().constData());
	obs_data_set_bool(settings, "is_local_file", true);
	obs_data_set_bool(settings, "looping", false);

	obs_source_update(mediaSource, settings);
	obs_source_media_restart(mediaSource);
}

void SourceManager::stop()
{
	if (mediaSource)
		obs_source_media_stop(mediaSource);
}
