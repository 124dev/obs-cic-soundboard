

#include "SourceManager.hpp"

#include <obs-module.h>     
#include <obs-frontend-api.h>



void SourceManager::ensureSource()
{
	blog(LOG_INFO, "[Soundboard] ensureSource()");
	// if source exist, reuse the source and set the output source to 63
	if (mediaSource) {
		blog(LOG_INFO, "[Soundboard] Reusing existing source");
		obs_set_output_source(63, mediaSource);
		return;
	}
	// Check if a source named "Soundboard" already exists in OBS
	obs_source_t *existing = obs_get_source_by_name("Soundboard");

	if (existing) {
		// Adopt the existing source instead of creating another one
		mediaSource = existing;
		obs_source_release(existing); 
		blog(LOG_INFO, "[Soundboard] Adopted existing source");
	} else {
		// Create a new hidden source
		mediaSource = obs_source_create("ffmpeg_source", "Soundboard", nullptr, nullptr);
		// return if failed
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
	obs_source_set_volume(mediaSource, 0.7f);

	blog(LOG_INFO, "[Soundboard] Source attached to output slot 63");
}

void SourceManager::clearSource()
{
	blog(LOG_INFO, "[Soundboard] clearSource()");

	// Detach from the output channel first so OBS stops mixing it.
	obs_set_output_source(63, nullptr);

	if (mediaSource) {
		obs_source_t *raw = mediaSource.Get();
		mediaSource = nullptr; 

		// Release ownership of the OBS source
		if (raw)
			obs_source_release(raw);

		blog(LOG_INFO, "[Soundboard] Source completely detached and destroyed");
	}
	// Clear the currently loaded file path
	currentFile.clear();
}

// Plackbacks
// Play an audio file through the OBS media source
void SourceManager::playFile(const QString &path)
{
	// Ignore empty file paths
	if (path.isEmpty())
		return;

	// If the same file is already loaded, just restart from the beginning.
	if (currentFile == path && mediaSource) {
		obs_source_media_restart(mediaSource);
		return;
	}
	// store new file path and make sure that media source exist, to recreate it
	currentFile = path;
	ensureSource();
	// if no source, return
	if (!mediaSource)
		return;

	// Configure the media source with the new audio file
	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_string(settings, "local_file", path.toUtf8().constData());
	obs_data_set_bool(settings, "is_local_file", true);
	obs_data_set_bool(settings, "looping", false);
	// Apply the new settings and start playback
	obs_source_update(mediaSource, settings);
	obs_source_media_restart(mediaSource);
}
// Stop the currently playing audio
void SourceManager::stop()
{
	if (mediaSource)
		obs_source_media_stop(mediaSource);
}
// Mute the media source
void SourceManager::mute()
{
    if (mediaSource)
        obs_source_set_muted(mediaSource, true);
}
// Unmute the media source
void SourceManager::unmute()
{
    if (mediaSource)
        obs_source_set_muted(mediaSource, false);
}
// Restart the media source
void SourceManager::restart()
{
    if (mediaSource)
        obs_source_media_restart(mediaSource);
}

