/*
 *   Imgx test plugin for X-Plane
 *   Created by Roman Liubich
 *   Inspired by Christopher Collins
 *
 *   This is also an example how to use imgx in the final project
 */

#define VERSION_NUMBER "0.1.0+" __DATE__ "." __TIME__

#include "XPLMDisplay.h"
#include "XPLMGraphics.h"
#include "XPLMPlugin.h"
#include "XPLMUtilities.h"

#include "gui.h"

#include <cstring>
#include <memory>

std::shared_ptr<Gui> gui;

PLUGIN_API int XPluginStart(char * outName, char * outSig, char * outDesc) {
    XPLMDebugString("imgx_test: ver " VERSION_NUMBER  "\n");
    strcpy(outName, "imgx_test: ver " VERSION_NUMBER);
    strcpy(outSig, "rhard.plugin.imgx_test");
    strcpy(outDesc, "Another ImGui port for X-Plane");

    XPLMEnableFeature("XPLM_USE_NATIVE_PATHS", 1);

    int left, top, right, bot;
    XPLMGetScreenBoundsGlobal(&left, &top, &right, &bot);

    int width = 800;
    int height = 550;
    int left_pad = 175;
    int top_pad = 75;
    int x = left + left_pad;
    int y = top - top_pad;
    // WindowDecoration decorate
    // WindowDecorationNone = 0
    // WindowDecorationRoundRectangle = 1
    // WindowDecorationSelfDecorated = 2
    // WindowDecorationSelfDecoratedResizable = 3
    int decorate = 1;

    gui = std::make_shared<Gui>();

    return 1;
}

PLUGIN_API void	XPluginStop(void) {
}

PLUGIN_API void XPluginDisable(void) {
}

PLUGIN_API int XPluginEnable(void) {
    return 1;
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID, intptr_t inMessage, void * inParam) {
}