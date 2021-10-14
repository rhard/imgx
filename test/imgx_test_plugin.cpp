/*
 *   Imgx test plugin for X-Plane
 *   Created by Roman Liubich
 *   Inspired by  William Good and Christopher Collins
 *
 *   This is also an example how to use imgx in the final project
 */

#define VERSION_NUMBER "0.1.1"

#include "XPLMDisplay.h"
#include "XPLMGraphics.h"
#include "XPLMPlugin.h"
#include "XPLMUtilities.h"

#include "testwindow.h"

#if LIN
#include <GL/gl.h>
#include <cstring>
#elif IBM
#include <gl/GL.h>
#else
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl.h>
#endif

#include <memory>

std::shared_ptr<TestWindow> window, window2;
ImFontAtlas *fontAtlas = nullptr;

static void setupImGuiFonts() {
    // bind default font
    unsigned char *pixels;
    int width, height;

    fontAtlas = new ImFontAtlas();
    fontAtlas->GetTexDataAsAlpha8(&pixels, &width, &height);

    // slightly stupid dance around the texture number due to XPLM not using GLint here.
    int texNum = 0;
    XPLMGenerateTextureNumbers(&texNum, 1);

    // upload texture.
    XPLMBindTexture2d(texNum, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA,
                 GL_UNSIGNED_BYTE, pixels);
    fontAtlas->TexID = (void *) (uintptr_t) texNum;
}

static void deleteImGuiFonts() {
    auto t = (GLuint) (uintptr_t) fontAtlas->TexID;
    glDeleteTextures(1, &t);
}

PLUGIN_API int XPluginStart(char *outName, char *outSig, char *outDesc) {
    XPLMDebugString("imgx_test: ver " VERSION_NUMBER  "\n");
    strcpy(outName, "imgx_test: ver " VERSION_NUMBER);
    strcpy(outSig, "rhard.plugin.imgx_test");
    strcpy(outDesc, "Another ImGui port for X-Plane");

    XPLMEnableFeature("XPLM_USE_NATIVE_PATHS", 1);

    setupImGuiFonts();
    window = std::make_shared<TestWindow>(fontAtlas);
    window->SetVisible(true);
    window2 = std::make_shared<TestWindow>(fontAtlas);
    window2->SetVisible(true);

    return 1;
}

PLUGIN_API void XPluginStop(void) {
    deleteImGuiFonts();
}

PLUGIN_API void XPluginDisable(void) {
}

PLUGIN_API int XPluginEnable(void) {
    return 1;
}

PLUGIN_API void
XPluginReceiveMessage(XPLMPluginID, intptr_t inMessage, void *inParam) {
}

