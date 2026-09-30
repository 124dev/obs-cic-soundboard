#include <obs-frontend-api.h>
#include <obs-module.h>

#include <QMainWindow>

#include "soundboard/Soundboard.hpp"

#include "plugin-support.h"

// Registers the module with OBS (name, description, author, etc.).
OBS_DECLARE_MODULE()
OBS_MODULE_AUTHOR("124dev");
OBS_MODULE_USE_DEFAULT_LOCALE("CIC Soundboard", "en-US")
MODULE_EXPORT const char* obs_module_description(void) {
    return obs_module_text("Description");
}

MODULE_EXPORT const char* obs_module_name(void) {
    return obs_module_text("CIC Soundboard");
}

// soundboard instance
Soundboard* g_soundboard = nullptr;

// declarations
static void onEvent(obs_frontend_event event, void* data);
static void onFrontendSave(obs_data_t* saveData, bool saving, void* data);

// SAVE AND LOAD CALLBACK
// functions for both saving and loading a scene collection .
static void onFrontendSave(obs_data_t* saveData, bool saving, void* /*data*/) {
    if (!g_soundboard)
        return;

    if (saving) {
        OBSDataAutoRelease sbData = obs_data_create();
        g_soundboard->saveData(sbData);
        obs_data_set_obj(saveData, "soundboard", sbData);
    } else {
        OBSDataAutoRelease sbData = obs_data_get_obj(saveData, "soundboard");
        g_soundboard->loadData(sbData);
    }
}

// called after the plugin is loaded
// this event doesnt load the UI yet
bool obs_module_load(void) {
    blog(LOG_INFO, "[Soundboard] Module loaded");
    return true;  // continues
}

// this is called after all modules have loaded into OBS
//  show Soundboard UI
void obs_module_post_load(void) {
    obs_frontend_push_ui_translation(obs_module_get_string);
    // grab obs window to use as dock's parent
    auto* mainWindow =
        static_cast<QMainWindow*>(obs_frontend_get_main_window());

    if (!mainWindow) {
        blog(LOG_ERROR, "[Soundboard] Could not obtain main window");
        obs_frontend_pop_ui_translation();
        return;
    }
    // create soundboard dock parented into main window
    g_soundboard = new Soundboard(mainWindow);
    // add the dock using it's ID, then name it CIC Soundboard
    obs_frontend_add_dock_by_id(
        "SoundboardDock", obs_module_text("CIC Soundboard"), g_soundboard);

    // register save/load hook so data persists with scene collections
    obs_frontend_add_save_callback(onFrontendSave, nullptr);

    // register the frontend event callback to react to plugin loading, exiting,
    // etc..
    //  just general event listener
    obs_frontend_add_event_callback(onEvent, nullptr);
    // show ui
    obs_frontend_pop_ui_translation();

    blog(LOG_INFO, "[Soundboard] Dock registered");
}

// MODULE EVENTS
// called when obs is about to shut down
void obs_module_unload(void) {
    // First, delete the soundboard to ensure clean destruction
    if (g_soundboard) {
        delete g_soundboard;
        g_soundboard = nullptr;
    }

    // Remove callbacks
    obs_frontend_remove_event_callback(onEvent, nullptr);
    obs_frontend_remove_save_callback(onFrontendSave, nullptr);

    // output
    blog(LOG_INFO, "[Soundboard] Module unloaded");
}

// FRONTEND EVENTS HANDLER
static void onEvent(obs_frontend_event event, void*) {
    switch (event) {
        case OBS_FRONTEND_EVENT_FINISHED_LOADING:
            if (g_soundboard) {
                // Add and make sure the global audio source is inside scene
                // sources.
                g_soundboard->ensureSource();
            }
            break;

        case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGING:
            if (g_soundboard) {
                // Clear the global audio source before changing collections.
                g_soundboard->clearSource();
            }
            break;

        case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED:
            blog(LOG_INFO,
                 "[Soundboard] Scene collection changed — re-creating source");
            if (g_soundboard) {
                g_soundboard->ensureSource();
            }
            break;

        case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CLEANUP:
            blog(LOG_INFO, "[Soundboard] Scene collection cleanup");
            if (g_soundboard) {
                g_soundboard->clearSource();
            }
            break;

        case OBS_FRONTEND_EVENT_EXIT:
            blog(LOG_INFO, "[Soundboard] OBS exit");
            if (g_soundboard) {
                g_soundboard->clearSource();
            }
            break;

        case OBS_FRONTEND_EVENT_SCENE_COLLECTION_RENAMED:
            blog(LOG_INFO, "[Soundboard] Scene Collection Renamed");
            if (g_soundboard) {
                g_soundboard->ensureSource();
            }
            break;

        default:
            break;
    }
}
