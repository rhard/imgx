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

static int mTop;
static int mBottom;
static int mLeft;
static int mRight;
static int mWidth;
static int mHeight;
static float mModelView[16], mProjection[16];
static int mViewport[4];

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

    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    main_viewport->PlatformHandle = (void*)this;

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
    XPLMGetScreenBoundsGlobal(&mLeft, &mTop, &mRight, &mBottom);

    mWidth = mRight - mLeft;
    mHeight = mTop - mBottom;

    io.DisplaySize = ImVec2(static_cast<float>(mWidth), static_cast<float>(mHeight));
    // in boxels, we're always scale 1, 1.
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

    io.DeltaTime = inElapsedSinceLastCall;

    int mouse_x, mouse_y;
    XPLMGetMouseLocationGlobal(&mouse_x, &mouse_y);
    float outX, outY;
    translateToImguiSpace(mouse_x, mouse_y, outX, outY);
    io.MousePos = ImVec2(outX, outY);

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
    auto viewport = reinterpret_cast<ImGuiViewport *>(inRefcon);
    ImGuiIO &io = ImGui::GetIO();
    ImDrawData *draw_data = viewport->DrawData;
    draw_data->ScaleClipRects(io.DisplayFramebufferScale);

    updateMatrices();

    // We are using the OpenGL fixed pipeline because messing with the
    // shader-state in X-Plane is not very well documented, but using the fixed
    // function pipeline is.

    // 1TU + Alpha settings, no depth, no fog.
    XPLMSetGraphicsState(0, 1, 0, 1, 1, 0, 0);
    glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT);
    glDisable(GL_CULL_FACE);
    glEnable(GL_SCISSOR_TEST);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glScalef(1.0f, -1.0f, 1.0f);
    glTranslatef(static_cast<GLfloat>(mLeft), static_cast<GLfloat>(-mTop), 0.0f);

    // Render command lists
    for (int n = 0; n < draw_data->CmdListsCount; n++) {
        const ImDrawList *cmd_list = draw_data->CmdLists[n];
        const ImDrawVert *vtx_buffer = cmd_list->VtxBuffer.Data;
        const ImDrawIdx *idx_buffer = cmd_list->IdxBuffer.Data;
        glVertexPointer(2, GL_FLOAT, sizeof(ImDrawVert),
                        (const GLvoid *) ((const char *) vtx_buffer + IM_OFFSETOF(ImDrawVert, pos)));
        glTexCoordPointer(2, GL_FLOAT, sizeof(ImDrawVert),
                          (const GLvoid *) ((const char *) vtx_buffer + IM_OFFSETOF(ImDrawVert, uv)));
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ImDrawVert),
                       (const GLvoid *) ((const char *) vtx_buffer + IM_OFFSETOF(ImDrawVert, col)));

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
            const ImDrawCmd *pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback) {
                pcmd->UserCallback(cmd_list, pcmd);
            } else {
                glBindTexture(GL_TEXTURE_2D, (GLuint) (intptr_t) pcmd->TextureId);

                // Scissors work in viewport space - must translate the coordinates from ImGui -> Boxels, then Boxels -> Native.
                //FIXME: it must be possible to apply the scale+transform manually to the projection matrix so we don't need to doublestep.
                int bTop, bLeft, bRight, bBottom;
                translateImguiToBoxel(pcmd->ClipRect.x, pcmd->ClipRect.y, bLeft, bTop);
                translateImguiToBoxel(pcmd->ClipRect.z, pcmd->ClipRect.w, bRight, bBottom);
                int nTop, nLeft, nRight, nBottom;
                boxelsToNative(bLeft, bTop, nLeft, nTop);
                boxelsToNative(bRight, bBottom, nRight, nBottom);
                glScissor(nLeft, nBottom, nRight - nLeft, nTop - nBottom);
                glDrawElements(GL_TRIANGLES, (GLsizei) pcmd->ElemCount,
                               sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer);
            }
            idx_buffer += pcmd->ElemCount;
        }
    }

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    // Restore modified state
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glBindTexture(GL_TEXTURE_2D, 0);
    glPopAttrib();
    glPopClientAttrib();
}

int ImgX::handleMouseClickCB(XPLMWindowID inWindowID, int x, int y, XPLMMouseStatus inMouse, void *inRefcon) {
    ImGuiIO &io = ImGui::GetIO();
    static int dX = 0, dY = 0;
    static int gDragging = 0;

    auto viewport = reinterpret_cast<ImGuiViewport *>(inRefcon);

    if (io.WantCaptureMouse) {
        switch (inMouse) {
        case xplm_MouseDown: {
            io.MouseDown[0] = true;
        }
        case xplm_MouseDrag:
            break;
        case xplm_MouseUp:
            io.MouseDown[0] = false;
            gDragging = 0;
            break;
        default:
            // dunno!
            break;
        }
        return 1;
    }
    else
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
    platform_io.Platform_ShowWindow = showWindow;
    platform_io.Platform_SetWindowSize = setWindowSize;
    platform_io.Platform_GetWindowSize = getWindowSize;
    platform_io.Platform_SetWindowPos = setWindowPos;
    platform_io.Platform_GetWindowPos = getWindowPos;
    platform_io.Platform_SetWindowTitle = setWindowTitle;
    platform_io.Platform_SetWindowFocus = setWindowFocus;
    platform_io.Platform_GetWindowFocus = getWindowFocus;
}

void ImgX::shutdownPlatformInterface() {
    ImGui::DestroyPlatformWindows();
}

void ImgX::createWindow(ImGuiViewport *viewport) {

    ImGuiID id = viewport->ID;

    XPLMCreateWindow_t windowParams = {
            sizeof(windowParams),
            50,
            10,
            500,
            1000,
            1,
            drawWindowCB,
            handleMouseClickCB,
            handleKeyFuncCB,
            handleCursorFuncCB,
            handleMouseWheelFuncCB,
            reinterpret_cast<void *>(viewport),
            1,
            xplm_WindowLayerFloatingWindows,
            handleRightClickFuncCB,
    };
    viewport->PlatformHandle = XPLMCreateWindowEx(&windowParams);
}

void ImgX::destroyWindow(ImGuiViewport *viewport) {

}

void ImgX::setWindowSize(ImGuiViewport *viewport, ImVec2 size) {
    if (ImGui::GetMainViewport() != viewport)
    {
        int left, top, right, bottom;
        XPLMGetWindowGeometry(viewport->PlatformHandle, &left, &top, &right, &bottom);
        XPLMSetWindowGeometry(viewport->PlatformHandle, left, top, left + size.x, top - size.y);
    }
}

ImVec2 ImgX::getWindowSize(ImGuiViewport *viewport) { 
    if (ImGui::GetMainViewport() != viewport)
    {
        int left, top, right, bottom;
        XPLMGetWindowGeometry(viewport->PlatformHandle, &left, &top, &right, &bottom);
        return ImVec2(bottom - top, right - left);
    }
}

void ImgX::setWindowPos(ImGuiViewport *viewport, ImVec2 size) {
    if (ImGui::GetMainViewport() != viewport)
    {
        int left, top, right, bottom, width, hight;
        XPLMGetWindowGeometry(viewport->PlatformHandle, &left, &top, &right, &bottom);
        width = right - left;
        hight = top - bottom;
        XPLMSetWindowGeometry(viewport->PlatformHandle, size.x, mHeight - size.y, size.x + width, mHeight - size.y - width);
    }
}

ImVec2 ImgX::getWindowPos(ImGuiViewport *viewport) {
    if (ImGui::GetMainViewport() != viewport)
    {
        int left, top, right, bottom;
        XPLMGetWindowGeometry(viewport->PlatformHandle, &left, &top, &right, &bottom);
        return ImVec2();
    }
}

void ImgX::setWindowTitle(ImGuiViewport *viewport, const char *title) {
    XPLMSetWindowTitle(viewport->PlatformHandle, title);
}

void ImgX::showWindow(ImGuiViewport *viewport) {

}

void ImgX::setWindowFocus(ImGuiViewport *viewport) {

}

bool ImgX::getWindowFocus(ImGuiViewport *viewport) {
    return false;
}

void ImgX::updateMatrices() {
    // Get the current modelview matrix, viewport, and projection matrix from X-Plane
    XPLMGetDatavf(gModelviewMatrixRef, mModelView, 0, 16);
    XPLMGetDatavf(gProjectionMatrixRef, mProjection, 0, 16);
    XPLMGetDatavi(gViewportRef, mViewport, 0, 4);
}

void ImgX::boxelsToNative(int x, int y, int &outX, int &outY) {
//    GLfloat boxelPos[4] = { (GLfloat)x, (GLfloat)y, 0, 1 };
//    GLfloat eye[4], ndc[4];
//
//    multMatrixVec4f(eye, mModelView, boxelPos);
//    multMatrixVec4f(ndc, mProjection, eye);
//    ndc[3] = 1.0f / ndc[3];
//    ndc[0] *= ndc[3];
//    ndc[1] *= ndc[3];
//
//    outX = static_cast<int>((ndc[0] * 0.5f + 0.5f) * mViewport[2] + mViewport[0]);
//    outY = static_cast<int>((ndc[1] * 0.5f + 0.5f) * mViewport[3] + mViewport[1]);
}

void ImgX::translateImguiToBoxel(float inX, float inY, int &outX, int &outY) {
    outX = (int) (mLeft + inX);
    outY = (int) (mTop - inY);
}

void ImgX::translateToImguiSpace(int inX, int inY, float &outX, float &outY) {
    outX = static_cast<float>(inX - mLeft);
    if (outX < 0.0f || outX > (float) (mRight - mLeft)) {
        outX = -FLT_MAX;
        outY = -FLT_MAX;
        return;
    }
    outY = static_cast<float>(mTop - inY);
    if (outY < 0.0f || outY > (float) (mTop - mBottom)) {
        outX = -FLT_MAX;
        outY = -FLT_MAX;
        return;
    }
}
