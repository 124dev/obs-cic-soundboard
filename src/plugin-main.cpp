// ============================================================
// plugin-main.cpp
// OBS plugin entry points for the Soundboard plugin.
//
// Lifecycle:
//   obs_module_load()      – called first; minimal setup only
//   obs_module_post_load() – called after all modules are loaded;
//                            safe to access the OBS UI here
//   obs_module_unload()    – called on OBS shutdown
//
// The Soundboard widget is created once and lives for the entire
// OBS session, docked inside the main window.
// ============================================================

#include <obs-module.h>       // OBS_DECLARE_MODULE, blog(), obs_data_*
#include <obs-frontend-api.h> // obs_frontend_add_dock_by_id(), event callbacks

#include <QMainWindow> // Cast from obs_frontend_get_main_window()

#include "Soundboard.hpp"

#include "plugin-support.h" // support file for meta data
// ── OBS module boilerplate ───────────────────────────────────

// Registers the module with OBS (name, description, author, etc.).
OBS_DECLARE_MODULE()
OBS_MODULE_AUTHOR("Juan Carlo Marasigan");
OBS_MODULE_USE_DEFAULT_LOCALE("CIC Soundboard", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return obs_module_text("Description");
}

MODULE_EXPORT const char *obs_module_name(void)
{
	return obs_module_text("CIC Soundboard");
}
// ── Global state ─────────────────────────────────────────────

// Single plugin-wide Soundboard instance.
// Declared extern in Soundboard.cpp so hotkey callbacks can reach it.
Soundboard *g_soundboard = nullptr;

// ── Forward declarations ─────────────────────────────────────

static void onEvent(obs_frontend_event event, void *data);
static void onFrontendSave(obs_data_t *saveData, bool saving, void *data);

// ── Save / load callback ──────────────────────────────────────

// OBS calls this function both when saving AND when loading a scene collection.
// The 'saving' flag distinguishes the two directions.
static void onFrontendSave(obs_data_t *saveData, bool saving, void * /*data*/)
{
	if (!g_soundboard)
		return;

	if (saving) {
		// Serialize the clip list into a sub-object keyed "soundboard".
		OBSDataAutoRelease sbData = obs_data_create();
		g_soundboard->saveData(sbData);
		obs_data_set_obj(saveData, "soundboard", sbData);
	} else {
		// Deserialize from the same sub-object on scene-collection load.
		OBSDataAutoRelease sbData = obs_data_get_obj(saveData, "soundboard");
		g_soundboard->loadData(sbData); // loadData() handles a null sbData gracefully
	}
}

// ── Module lifecycle ──────────────────────────────────────────

// Called immediately after the plugin .so/.dll is loaded.
// The OBS UI is NOT available yet — keep this as minimal as possible.
bool obs_module_load(void)
{
	blog(LOG_INFO, "[Soundboard] Module loaded");
	return true; // Return false to abort loading
}

// Called after ALL modules have loaded and the OBS UI is fully available.
// Safe to access Qt widgets and register docks here.
void obs_module_post_load(void)
{
	// Push the plugin's locale strings so obs_module_text() works below.
	obs_frontend_push_ui_translation(obs_module_get_string);

	// Grab the main OBS window to use as the dock's parent.
	auto *mainWindow = static_cast<QMainWindow *>(obs_frontend_get_main_window());

	if (!mainWindow) {
		blog(LOG_ERROR, "[Soundboard] Could not obtain main window — aborting");
		obs_frontend_pop_ui_translation();
		return;
	}

	// Create the dock widget (parented to the main window so Qt manages lifetime).
	g_soundboard = new Soundboard(mainWindow);

	// Register as a dockable panel. "SoundboardDock" is the internal ID;
	// obs_module_text("CIC Soundboard") is the visible title shown in the menu.
	obs_frontend_add_dock_by_id("SoundboardDock", obs_module_text("CIC Soundboard"), g_soundboard);

	// Register the save/load hook so clip data persists with scene collections.
	obs_frontend_add_save_callback(onFrontendSave, nullptr);

	// Register the frontend event callback to react to load-complete / exit / etc.
	obs_frontend_add_event_callback(onEvent, nullptr);

	obs_frontend_pop_ui_translation();

	blog(LOG_INFO, "[Soundboard] Dock registered");
}

// Called when OBS is about to shut down.
void obs_module_unload(void)
{
	// Deregister the save callback to prevent a use-after-free if OBS
	// flushes one more save after g_soundboard has been destroyed.
	obs_frontend_remove_save_callback(onFrontendSave, nullptr);
	obs_frontend_remove_event_callback(onEvent, nullptr);
	// We null the pointer here so no stale callbacks can fire after this point.
	g_soundboard = nullptr;

	blog(LOG_INFO, "[Soundboard] Module unloaded");
}

// ── Frontend event handler ────────────────────────────────────

static void onEvent(obs_frontend_event event, void * /*data*/)
{
	switch (event) {

	// OBS has finished loading everything (sources, scenes, saved data).
	// Ensure the hidden playback source is attached to the output channel.
	case OBS_FRONTEND_EVENT_FINISHED_LOADING:
		blog(LOG_INFO, "[Soundboard] OBS finished loading");
		if (g_soundboard)
			g_soundboard->ensureSource();
		break;

	// A scene collection switch is cleaning up — nothing extra needed here,
	// but the case is kept for clarity and future use.
	case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CLEANUP:
		blog(LOG_INFO, "[Soundboard] Scene collection cleanup");
		break;

	// OBS is about to exit. The module_unload callback handles cleanup.
	case OBS_FRONTEND_EVENT_EXIT:
		blog(LOG_INFO, "[Soundboard] OBS exit");
		break;

	default:
		break;
	}
}
