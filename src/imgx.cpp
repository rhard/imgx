/*
 *   Another ImGui port for X-Plane
 *   Created by Roman Liubich
 *   Inspired by Christopher Collins
 */

#include "XPLMDataAccess.h"
#include "XPLMGraphics.h"

#include "imgx.h"

#include <Gl/GL.h>

static XPLMDataRef gVrEnabledRef = nullptr;
static XPLMDataRef gModelviewMatrixRef = nullptr;
static XPLMDataRef gViewportRef = nullptr;
static XPLMDataRef gProjectionMatrixRef = nullptr;

struct ViewportData
{
    ImgX * thisImgx;
    ViewportData(){thisImgx = nullptr;}
};

ImgX::ImgX(XPLMWindowLayer layer) : mPreferredLayer(layer) {

    static bool first_init = false;
    if (!first_init) {
        gVrEnabledRef = XPLMFindDataRef("sim/graphics/VR/enabled");
        gModelviewMatrixRef = XPLMFindDataRef("sim/graphics/view/modelview_matrix");
        gViewportRef = XPLMFindDataRef("sim/graphics/view/viewport");
        gProjectionMatrixRef = XPLMFindDataRef("sim/graphics/view/projection_matrix");
        first_init = true;
    }

    ImGui::CreateContext();
    auto &io = ImGui::GetIO();

    // We can create multi-viewports
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;
    io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        initPlatformInterface();

    // we render ourselves, we don't use the DrawListsFunc
    io.RenderDrawListsFn = nullptr;
    // set up the Keymap
    io.KeyMap[ImGuiKey_Tab] = XPLM_VK_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = XPLM_VK_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = XPLM_VK_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = XPLM_VK_UP;
    io.KeyMap[ImGuiKey_DownArrow] = XPLM_VK_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = XPLM_VK_PRIOR;
    io.KeyMap[ImGuiKey_PageDown] = XPLM_VK_NEXT;
    io.KeyMap[ImGuiKey_Home] = XPLM_VK_HOME;
    io.KeyMap[ImGuiKey_End] = XPLM_VK_END;
    io.KeyMap[ImGuiKey_Insert] = XPLM_VK_INSERT;
    io.KeyMap[ImGuiKey_Delete] = XPLM_VK_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = XPLM_VK_BACK;
    io.KeyMap[ImGuiKey_Space] = XPLM_VK_SPACE;
    io.KeyMap[ImGuiKey_Enter] = XPLM_VK_ENTER;
    io.KeyMap[ImGuiKey_Escape] = XPLM_VK_ESCAPE;
    io.KeyMap[ImGuiKey_A] = XPLM_VK_A;
    io.KeyMap[ImGuiKey_C] = XPLM_VK_C;
    io.KeyMap[ImGuiKey_V] = XPLM_VK_V;
    io.KeyMap[ImGuiKey_X] = XPLM_VK_X;
    io.KeyMap[ImGuiKey_Y] = XPLM_VK_Y;
    io.KeyMap[ImGuiKey_Z] = XPLM_VK_Z;

    // Register flight loop for ImGui processing
    XPLMCreateFlightLoop_t flParams = {
            sizeof(flParams),
            xplm_FlightLoop_Phase_AfterFlightModel,
            flightCB,
            reinterpret_cast<void *>(this)
    };

    mFlightLoop = XPLMCreateFlightLoop(&flParams);
}

ImgX::~ImgX() {
    XPLMDestroyFlightLoop(mFlightLoop);
    shutdownPlatformInterface();
    ImGui::DestroyContext();
}

void ImgX::configureImguiContext() {

}

void ImgX::Draw() {
    XPLMScheduleFlightLoop(mFlightLoop, -1.0f, 1);
}

void ImgX::init() {

    auto &io = ImGui::GetIO();

    // disable window rounding since we're not rendering the frame anyway.
    auto &style = ImGui::GetStyle();
    style.WindowRounding = 0;

    // call user defined settings
    configureImguiContext();

    // bind the font
    unsigned char *pixels;
    int width, height;
    io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);

    // slightly stupid dance around the texture number due to XPLM not using GLint here.
    int texNum = 0;
    XPLMGenerateTextureNumbers(&texNum, 1);

    // upload texture.
    XPLMBindTexture2d(texNum, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, pixels);
    io.Fonts->TexID = (void *) (intptr_t) (texNum);

    // disable OSX-like keyboard behaviours always - we don't have the keymapping for it.
    io.ConfigMacOSXBehaviors = false;
}

float
ImgX::flightCB(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void *inRefcon) {

    auto thisImgx = reinterpret_cast<ImgX *>(inRefcon);
    auto &io = ImGui::GetIO();

    // transfer the window geometry to ImGui
    XPLMGetScreenBoundsGlobal(&thisImgx->mLeft, &thisImgx->mTop, &thisImgx->mRight, &thisImgx->mBottom);

    thisImgx->mWidth = thisImgx->mRight - thisImgx->mLeft;
    thisImgx->mHeight = thisImgx->mTop - thisImgx->mBottom;

    io.DisplaySize = ImVec2(static_cast<float>(thisImgx->mWidth), static_cast<float>(thisImgx->mHeight));
    // in boxels, we're always scale 1, 1.
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

    io.DeltaTime = inElapsedSinceLastCall;

    ImGui::NewFrame();
    // Call user defined ImGui interface
    thisImgx->buildInterface();
    ImGui::EndFrame();
    ImGui::UpdatePlatformWindows();

    //Update ImGui but not draw. Actual drawing will happen in window draw callbacks
    ImGui::Render();

    return -1.0f;
}

void ImgX::drawWindowCB(XPLMWindowID inWindowID, void *inRefcon) {

}

int ImgX::handleMouseClickCB(XPLMWindowID inWindowID, int x, int y, XPLMMouseStatus inMouse, void *inRefcon) {
    return 0;
}

void ImgX::handleKeyFuncCB(XPLMWindowID inWindowID, char inKey, XPLMKeyFlags inFlags, char inVirtualKey, void *inRefcon,
                           int losingFocus) {

}

XPLMCursorStatus ImgX::handleCursorFuncCB(XPLMWindowID inWindowID, int x, int y, void *inRefcon) {
    return 0;
}

int ImgX::handleMouseWheelFuncCB(XPLMWindowID inWindowID, int x, int y, int wheel, int clicks, void *inRefcon) {
    return 0;
}

int ImgX::handleRightClickFuncCB(XPLMWindowID inWindowID, int x, int y, XPLMMouseStatus inMouse, void *inRefcon) {
    return 0;
}

int ImgX::handleMouseClickGeneric(int x, int y, XPLMMouseStatus inMouse, int button) {
    return 0;
}

void ImgX::initPlatformInterface() {
    auto &platform_io = ImGui::GetPlatformIO();
    platform_io.Platform_CreateWindow = createWindow;
    platform_io.Platform_DestroyWindow = destroyWindow;
    platform_io.Platform_SetWindowSize = setWindowSize;
    platform_io.Platform_GetWindowSize = createWindow;
    platform_io.Platform_DestroyWindow = destroyWindow;
    platform_io.Platform_SetWindowSize = setWindowSize;
}

void ImgX::shutdownPlatformInterface() {
    ImGui::DestroyPlatformWindows();
}

void ImgX::createWindow(ImGuiViewport *viewport) {

    XPLMCreateWindow_t windowParams = {
            sizeof(windowParams),
            50,
            10,
            500,
            1000,
            0,
            drawWindowCB,
            handleMouseClickCB,
            handleKeyFuncCB,
            handleCursorFuncCB,
            handleMouseWheelFuncCB,
            reinterpret_cast<void *>(viewport),
            3,
            xplm_WindowLayerFloatingWindows,
            handleRightClickFuncCB,
    };
    viewport->PlatformHandle = XPLMCreateWindowEx(&windowParams);
}

void ImgX::destroyWindow(ImGuiViewport *viewport) {

}

void ImgX::setWindowSize(ImGuiViewport *viewport, ImVec2 size) {

}
