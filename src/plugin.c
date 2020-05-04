#if defined(WIN32) || defined(__WIN32__) || defined(_WIN32)
#pragma warning (disable : 4100)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "teamspeak/public_errors.h"
#include "teamspeak/public_definitions.h"
#include "ts3_functions.h"
#include "plugin.h"

static struct TS3Functions ts3Functions;

#ifdef _WIN32
#define _strcpy(dest, destSize, src) strcpy_s(dest, destSize, src)
#define snprintf sprintf_s
#else
#define _strcpy(dest, destSize, src) { strncpy(dest, src, destSize-1); (dest)[destSize-1] = '\0'; }
#endif

#define PLUGIN_API_VERSION 23

#define INFODATA_BUFSIZE 128
#define RETURNCODE_BUFSIZE 128

static char *pluginID = NULL;
static uint64 loverID = 0;

/*********************************** Required functions ************************************/
/*
 * If any of these required functions is not implemented, TS3 will refuse to load the plugin
 */

/* Unique name identifying this plugin */
const char *ts3plugin_name() {
    return "Liebe";
}

/* Plugin version */
const char *ts3plugin_version() {
    return "1.0";
}

/* Plugin API version. Must be the same as the clients API major version, else the plugin fails to load. */
int ts3plugin_apiVersion() {
    return PLUGIN_API_VERSION;
}

/* Plugin author */
const char *ts3plugin_author() {
    return "vsnry.dev & syntaax.dev";
}

/* Plugin description */
const char *ts3plugin_description() {
    return "Brings love to the world of TeamSpeak.";
}

/* Set TeamSpeak 3 callback functions */
void ts3plugin_setFunctionPointers(const struct TS3Functions funcs) {
    ts3Functions = funcs;
}

/*
 * Custom code called right after loading the plugin. Returns 0 on success, 1 on failure.
 * If the function returns 1 on failure, the plugin will be unloaded again.
 */
int ts3plugin_init() {
    return 0;
}

/* Custom code called right before the plugin is unloaded */
void ts3plugin_shutdown() {
    /* Free pluginID */
    if (pluginID) {
        free(pluginID);
        pluginID = NULL;
    }
}

void ts3plugin_freeMemory(void *data) {
    free(data);
}

/*
 * If the plugin wants to use error return codes, plugin commands, hotkeys or menu items, it needs to register a command ID. This function will be
 * automatically called after the plugin was initialized. This function is optional. If you don't use these features, this function can be omitted.
 * Note the passed pluginID parameter is no longer valid after calling this function, so you must copy it and store it in the plugin.
 */
void ts3plugin_registerPluginID(const char *id) {
    const size_t sz = strlen(id) + 1;
    pluginID = (char *) malloc(sz * sizeof(char));
    _strcpy(pluginID, sz, id);  /* The id buffer will invalidate after exiting this function */
    printf("PLUGIN: registerPluginID: %s\n", pluginID);
}

/*
 * Plugin requests to be always automatically loaded by the TeamSpeak 3 client unless
 * the user manually disabled it in the plugin dialog.
 * This function is optional. If missing, no autoload is assumed.
 */
int ts3plugin_requestAutoload() {
    return 0;  /* 1 = request autoloaded, 0 = do not request autoload */
}

/*
 * Implement the following two functions when the plugin should display a line in the server/channel/client info.
 * If any of ts3plugin_infoTitle, ts3plugin_infoData or ts3plugin_freeMemory is missing, the info text will not be displayed.
 */

/* Static title shown in the left column in the info frame */
const char *ts3plugin_infoTitle() {
    return "Liebe";
}

/*
 * Dynamic content shown in the right column in the info frame. Memory for the data string needs to be allocated in this
 * function. The client will call ts3plugin_freeMemory once done with the string to release the allocated memory again.
 * Check the parameter "type" if you want to implement this feature only for specific item types. Set the parameter
 * "data" to NULL to have the client ignore the info data.
 */
void ts3plugin_infoData(uint64 serverConnectionHandlerID, uint64 id, enum PluginItemType type, char **data) {
    char *name;

    if ((type == PLUGIN_CLIENT || type == PLUGIN_SERVER || type == PLUGIN_CHANNEL) && loverID > 0) {
        if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID) loverID, CLIENT_NICKNAME,
                                                   &name) != ERROR_ok) {
            printf("Error getting client nickname\n");
            return;
        }
    } else {
        data = NULL;
        return;
    }

    *data = (char *) malloc(INFODATA_BUFSIZE * sizeof(char));
    snprintf(*data, INFODATA_BUFSIZE, "You're loving [B]%s[/B]", name);
    ts3Functions.freeMemory(name);
}


/* Helper function to create a menu item */
static struct PluginMenuItem *createMenuItem(enum PluginMenuType type, int id, const char *text, const char *icon) {
    struct PluginMenuItem *menuItem = (struct PluginMenuItem *) malloc(sizeof(struct PluginMenuItem));
    menuItem->type = type;
    menuItem->id = id;
    _strcpy(menuItem->text, PLUGIN_MENU_BUFSZ, text);
    _strcpy(menuItem->icon, PLUGIN_MENU_BUFSZ, icon);
    return menuItem;
}

/* Some makros to make the code to create menu items a bit more readable */
#define BEGIN_CREATE_MENUS(x) const size_t sz = x + 1; size_t n = 0; *menuItems = (struct PluginMenuItem**)malloc(sizeof(struct PluginMenuItem*) * sz);
#define CREATE_MENU_ITEM(a, b, c, d) (*menuItems)[n++] = createMenuItem(a, b, c, d);
#define END_CREATE_MENUS (*menuItems)[n++] = NULL; assert(n == sz);

/*
 * Initialize plugin menus.
 * This function is called after ts3plugin_init and ts3plugin_registerPluginID. A pluginID is required for plugin menus to work.
 * Both ts3plugin_registerPluginID and ts3plugin_freeMemory must be implemented to use menus.
 * If plugin menus are not used by a plugin, do not implement this function or return NULL.
 */
void ts3plugin_initMenus(struct PluginMenuItem ***menuItems, char **menuIcon) {
    BEGIN_CREATE_MENUS(2);
    CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CLIENT, 0, "Love", "love.png");
    CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CLIENT, 1, "Hate", "hate.png");
    END_CREATE_MENUS;

    *menuIcon = (char *) malloc(PLUGIN_MENU_BUFSZ * sizeof(char));
    _strcpy(*menuIcon, PLUGIN_MENU_BUFSZ, "love.png");
}

/************************** TeamSpeak callbacks ***************************/
uint64 getOwnClientID(uint64 serverConnectionHandlerID) {
    anyID myID;

    if (ts3Functions.getClientID(serverConnectionHandlerID, &myID) != ERROR_ok) {
        ts3Functions.logMessage("Error querying client ID", LogLevel_ERROR, "Plugin", serverConnectionHandlerID);
        return -1;
    }

    return myID;
}

void follow(uint64 serverConnectionHandlerID, anyID clientID, uint64 channelID, uint64 oldChanneID) {
    if (clientID == loverID && channelID != oldChanneID) {
        char returnCode[RETURNCODE_BUFSIZE];
        ts3Functions.createReturnCode(pluginID, returnCode, RETURNCODE_BUFSIZE);

        if (ts3Functions.requestClientMove(serverConnectionHandlerID, getOwnClientID(serverConnectionHandlerID), channelID, "", returnCode) != ERROR_ok) {
            ts3Functions.logMessage("Error requesting client move", LogLevel_INFO, "Plugin", serverConnectionHandlerID);
        }
    }
}

void ts3plugin_onClientMoveEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID,
                                 uint64 newChannelID, int visibility, const char *moveMessage) {
    follow(serverConnectionHandlerID, clientID, newChannelID, oldChannelID);
}

void ts3plugin_onClientMoveMovedEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID,
                                      uint64 newChannelID, int visibility, anyID moverID, const char *moverName,
                                      const char *moverUniqueIdentifier, const char *moveMessage) {
    follow(serverConnectionHandlerID, clientID, newChannelID, oldChannelID);
}

void ts3plugin_onClientKickFromChannelEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID,
                                            uint64 newChannelID, int visibility, anyID kickerID, const char *kickerName,
                                            const char *kickerUniqueIdentifier, const char *kickMessage) {
    follow(serverConnectionHandlerID, clientID, newChannelID, oldChannelID);
}

void ts3plugin_onClientKickFromServerEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID,
                                           uint64 newChannelID, int visibility, anyID kickerID, const char *kickerName,
                                           const char *kickerUniqueIdentifier, const char *kickMessage) {
    loverID = 0;
}

void ts3plugin_onClientBanFromServerEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID,
                                          uint64 newChannelID, int visibility, anyID kickerID, const char *kickerName,
                                          const char *kickerUniqueIdentifier, uint64 time, const char *kickMessage) {
    loverID = 0;
}

void ts3plugin_onMenuItemEvent(uint64 serverConnectionHandlerID, enum PluginMenuType type, int menuItemID,
                               uint64 selectedItemID) {
    if (menuItemID == 0) {
        if (selectedItemID != getOwnClientID(serverConnectionHandlerID)) {
            loverID = selectedItemID;
        }
    } else if (menuItemID == 1) {
        loverID = 0;
    }
}