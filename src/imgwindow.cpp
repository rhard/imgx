/*
 * imgwindow.cpp
 *
 * Integration for dear imgui into X-Plane.
 *
 * Copyright (C) 2018, Christopher Collins
*/

/// Another ImGui port for X-Plane
/// created by Christopher Collins
/// Modified by Roman Liubich

#include "XPLMDataAccess.h"
#include "XPLMGraphics.h"
#include "XPLMProcessing.h"
#include "XPLMUtilities.h"

#include "imgwindow.h"
#include "imgui_internal.h"

#if LIN
#include <GL/gl.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#elif IBM

#include <gl/GL.h>

#else
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl.h>
#endif

/// \file
/// This file contains the definition of the ImgWindow class, which is the
/// base class for all ImGui driven X-Plane windows

// Dataref's for OpenGL scene data
static XPLMDataRef gVrEnabledRef = nullptr;
static XPLMDataRef gModelViewMatrixRef = nullptr;
static XPLMDataRef gViewportRef = nullptr;
static XPLMDataRef gProjectionMatrixRef = nullptr;

// OpenGL scene data
static float mModelView[16], mProjection[16];
static int mViewport[4];

#if APL
bool GetOSXClipboard(std::string& outText);
bool SetOSXClipboard(const std::string& inText);
#endif

static bool getTextFromClipboard(std::string &outText) {
#if IBM
    HGLOBAL hglb;
    LPSTR lptstr;
    bool retVal = false;
    static XPLMDataRef hwndDataRef = XPLMFindDataRef(
                "sim/operation/windows/system_window");
    HWND hwndMain = (HWND) (uintptr_t) XPLMGetDatai(hwndDataRef);

    if (!IsClipboardFormatAvailable(CF_TEXT))
        return false;

    if (!OpenClipboard(hwndMain))
        return false;

    hglb = GetClipboardData(CF_TEXT);
    if (hglb != nullptr) {
        lptstr = (LPSTR) GlobalLock(hglb);
        if (lptstr != nullptr) {
            outText = lptstr;
            GlobalUnlock(hglb);
            retVal = true;
        }
    }

    CloseClipboard();

    return retVal;
#endif
#if APL
    return GetOSXClipboard(outText);
#endif
#if LIN
    Display* display = XOpenDisplay(nullptr);
    if(display == nullptr) {
        return false;
    }
    // Need a window to pass into the conversion request.
    int black = BlackPixel (display, DefaultScreen (display));
    Window root = XDefaultRootWindow (display);
    Window window = XCreateSimpleWindow (display, root, 0, 0, 1, 1, 0, black, black);
    // Get/create the CLIPBOARD atom.
    // TODO(jason-watkins): Should we really ever create this atom? My limited knowledge of Linux is failing me here.
    Atom clipboard = XInternAtom (display, "CLIPBOARD", False);
    // Get/create an atom to represent datareftool
    Atom prop = XInternAtom(display, "DRT_DATA", False);

    // Request that the clipboard value be converted to a string
    XConvertSelection(display, clipboard, XA_STRING, prop, window, CurrentTime);
    XSync (display, False);

    // Spin several times waiting for the conversion to trigger a SelectionNotify
    // event.
    Bool keep_waiting = True;
    for(int i = 0; keep_waiting && i < 200; ++i)
    {
        XEvent event;
        XNextEvent(display, &event);
        switch (event.type) {
        case SelectionNotify:
            if (event.xselection.selection != clipboard) {
                break;
            }
            if (event.xselection.property == None) {
                keep_waiting = False;
            }
            else {
                int format;
                Atom target;
                unsigned char * value;
                unsigned long length;
                unsigned long bytesafter;
                XGetWindowProperty (event.xselection.display,
                                    event.xselection.requestor,
                                    event.xselection.property, 0L, 1000000,
                                    False, (Atom)AnyPropertyType, &target,
                                    &format, &length, &bytesafter, &value);
                outText = (char *)value;
                XFree(value);
                keep_waiting = False;
                XDeleteProperty (event.xselection.display,
                                 event.xselection.requestor,
                                 event.xselection.property);
            }
            break;
        default:
            break;
        }
    }
    XCloseDisplay(display);
    return true;
#endif
}

static bool setTextToClipboard(const std::string &inText) {
#if IBM
    LPSTR lptstrCopy;
    HGLOBAL hglbCopy;
    static XPLMDataRef hwndDataRef = XPLMFindDataRef(
                "sim/operation/windows/system_window");
    HWND hwndMain = (HWND) (uintptr_t) XPLMGetDatai(hwndDataRef);

    if (!OpenClipboard(hwndMain))
        return false;
    EmptyClipboard();

    hglbCopy = GlobalAlloc(GMEM_MOVEABLE,
                           sizeof(TCHAR) * (inText.length() + 1));
    if (hglbCopy == nullptr) {
        CloseClipboard();
        return false;
    }

    lptstrCopy = (LPSTR) GlobalLock(hglbCopy);
    strcpy(lptstrCopy, inText.c_str());
    GlobalUnlock(hglbCopy);

    SetClipboardData(CF_TEXT, hglbCopy);
    CloseClipboard();
    return true;
#endif
#if APL
    return SetOSXClipboard(inText);
#endif
#if LIN
    std::string command = "echo -n " + inText + " | xclip -sel c";
    if(0 != system(command.c_str())) {
        return false;
    }
    return true;
#endif
}

const char *ImgWindow::GetClipboardImGuiWrapper(void *user_data) {
    static std::string text;
    if (getTextFromClipboard(text))
        return text.c_str();
    else
        return "";
}

void ImgWindow::SetClipboardImGuiWrapper(void *user_data, const char
                                         *text) {
    std::string _text(text);
    setTextToClipboard(_text);
}

static void
multMatrixVec4f(GLfloat dst[4], const GLfloat m[16], const GLfloat v[4]) {
    dst[0] = v[0] * m[0] + v[1] * m[4] + v[2] * m[8] + v[3] * m[12];
    dst[1] = v[0] * m[1] + v[1] * m[5] + v[2] * m[9] + v[3] * m[13];
    dst[2] = v[0] * m[2] + v[1] * m[6] + v[2] * m[10] + v[3] * m[14];
    dst[3] = v[0] * m[3] + v[1] * m[7] + v[2] * m[11] + v[3] * m[15];
}

static void updateMatrices() {
    // Get the current modelview matrix, viewport, and projection matrix from X-Plane
    XPLMGetDatavf(gModelViewMatrixRef, mModelView, 0, 16);
    XPLMGetDatavf(gProjectionMatrixRef, mProjection, 0, 16);
    XPLMGetDatavi(gViewportRef, mViewport, 0, 4);
}

ImgWindow::ImgWindow(ImFontAtlas *fontAtlas) {
    mHasPrebuildFont = fontAtlas != nullptr;
    mImGuiContext = ImGui::CreateContext(fontAtlas);
    ImGui::SetCurrentContext(mImGuiContext);
}

void ImgWindow::Init(int width, int height, int x, int y, Anchor anchor,
                     XPLMWindowDecoration decoration,
                     XPLMWindowLayer layer,
                     XPLMWindowPositioningMode mode) {
    mIsInVR = false;
    mPreferredLayer = layer;
    mPreferredPositioningMode = mode;
    mSelfDestruct = false;
    mSelfHide = false;
    mSelfResize = false;
    mSelfPositioning = false;
    mFirstRender = true;
    mDecoration = decoration;
    mWindowTitle = "Default window title";

    auto &io = ImGui::GetIO();

    static bool first_init = false;
    if (!first_init) {
        gVrEnabledRef = XPLMFindDataRef("sim/graphics/VR/enabled");
        gModelViewMatrixRef = XPLMFindDataRef(
                    "sim/graphics/view/modelview_matrix");
        gViewportRef = XPLMFindDataRef("sim/graphics/view/viewport");
        gProjectionMatrixRef = XPLMFindDataRef(
                    "sim/graphics/view/projection_matrix");
        first_init = true;
    }
    // clipboard data
    io.SetClipboardTextFn = SetClipboardImGuiWrapper;
    io.GetClipboardTextFn = GetClipboardImGuiWrapper;

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
    io.KeyMap[ImGuiKey_Enter] = XPLM_VK_RETURN;
    io.KeyMap[ImGuiKey_Escape] = XPLM_VK_ESCAPE;
    io.KeyMap[ImGuiKey_KeyPadEnter] = XPLM_VK_ENTER;
    io.KeyMap[ImGuiKey_A] = XPLM_VK_A;
    io.KeyMap[ImGuiKey_C] = XPLM_VK_C;
    io.KeyMap[ImGuiKey_V] = XPLM_VK_V;
    io.KeyMap[ImGuiKey_X] = XPLM_VK_X;
    io.KeyMap[ImGuiKey_Y] = XPLM_VK_Y;
    io.KeyMap[ImGuiKey_Z] = XPLM_VK_Z;

    // disable window rounding since we're not rendering the frame anyway.
    auto &style = ImGui::GetStyle();
    if (decoration == xplm_WindowDecorationRoundRectangle)
        style.WindowRounding = 0;
    else
        style.WindowBorderSize = 0;

    ConfigureImGuiContext();

    // bind default font if not use shared font
    if (!mHasPrebuildFont) {
        // bind default font
        unsigned char *pixels;
        int fWidth, fHeight;
        io.Fonts->GetTexDataAsAlpha8(&pixels, &fWidth, &fHeight);

        // slightly stupid dance around the texture number due to XPLM not using GLint here.
        int texNum = 0;
        XPLMGenerateTextureNumbers(&texNum, 1);

        // upload texture.
        XPLMBindTexture2d(texNum, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, fWidth, fHeight, 0, GL_ALPHA,
                     GL_UNSIGNED_BYTE, pixels);
        io.Fonts->TexID = (void *) (uintptr_t) texNum;
    }
    // disable OSX-like keyboard behaviours always - we don't have the keymapping for it.
    io.ConfigMacOSXBehaviors = false; // io.OptMacOSXBehaviors = false;

    mWidth = width;
    mHeight = height;

    switch (anchor) {
    case TopLeft:
        mLeft = x;
        mRight = mLeft + width;
        mTop = y;
        mBottom = mTop - height;
        break;
    case TopRight:
        mRight = x;
        mLeft = mRight - width;
        mTop = y;
        mBottom = mTop - height;
        break;
    case BottomLeft:
        mLeft = x;
        mRight = mLeft + width;
        mBottom = y;
        mTop = mBottom + height;
        break;
    case BottomRight:
        mRight = x;
        mLeft = mRight - width;
        mBottom = y;
        mTop = mBottom + height;
        break;
    case Center:
        mLeft = x - width / 2;
        mRight = mLeft + width;
        mTop = y + height / 2;
        mBottom = mTop - height;
        break;
    default:
        break;
    }

    // check the window is within the screen size (for self decorated)
    if (mDecoration == xplm_WindowDecorationSelfDecorated) {
        int sLeft = 0, sTop = 0, sRight = 0, sBottom = 0;
        XPLMGetScreenBoundsGlobal(&sLeft, &sTop, &sRight, &sBottom);
        if (mLeft < sLeft || mLeft > sRight) {
            mLeft = sLeft;
            mRight = sLeft + mWidth;
        }
        if (mRight > sRight || mRight < sLeft) {
            mRight = sRight;
            mLeft = sRight - mWidth;
        }
        if (mBottom < sBottom || mBottom > sTop) {
            mBottom = sBottom;
            mTop = sBottom + mHeight;
        }
        if (mTop > sTop || mTop < sBottom) {
            mTop = sTop;
            mBottom = sTop - mHeight;
        }
    }

    XPLMCreateWindow_t windowParams = {
        sizeof(windowParams),
        mLeft,
        mTop,
        mRight,
        mBottom,
        0,
        drawWindowCB,
        handleMouseClickCB,
        handleKeyFuncCB,
        handleCursorFuncCB,
        handleMouseWheelFuncCB,
        reinterpret_cast<void *>(this),
        mDecoration,
        layer,
        handleRightClickFuncCB,
    };
    mWindowID = XPLMCreateWindowEx(&windowParams);
    XPLMSetWindowPositioningMode(mWindowID, mPreferredPositioningMode, -1);

    XPLMCreateFlightLoop_t flightLoopParameters = {
        sizeof(flightLoopParameters),
        xplm_FlightLoop_Phase_AfterFlightModel,
        flightLoopHandler,
        reinterpret_cast<void *>(this)
    };

    flightLoopID = XPLMCreateFlightLoop(&flightLoopParameters);
    XPLMScheduleFlightLoop(flightLoopID, -1.0f, true);
}

void
ImgWindow::ConfigureImGuiContext() {
}

ImgWindow::~ImgWindow() {
    ImGui::SetCurrentContext(mImGuiContext);
    auto &io = ImGui::GetIO();
    if (!mHasPrebuildFont) {
        auto t = (GLuint) (uintptr_t) io.Fonts->TexID;
        glDeleteTextures(1, &t);
    }
    ImGui::DestroyContext();
    XPLMDestroyFlightLoop(flightLoopID);
    XPLMDestroyWindow(mWindowID);
}

void
ImgWindow::boxelsToNative(int x, int y, int &outX, int &outY) {
    GLfloat boxelPos[4] = {(GLfloat) x, (GLfloat) y, 0, 1};
    GLfloat eye[4], ndc[4];

    multMatrixVec4f(eye, mModelView, boxelPos);
    multMatrixVec4f(ndc, mProjection, eye);
    ndc[3] = 1.0f / ndc[3];
    ndc[0] *= ndc[3];
    ndc[1] *= ndc[3];

    outX = static_cast<int>((ndc[0] * 0.5f + 0.5f) * mViewport[2] +
            mViewport[0]);
    outY = static_cast<int>((ndc[1] * 0.5f + 0.5f) * mViewport[3] +
            mViewport[1]);
}

void
ImgWindow::renderImGui() {
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    ImGui::SetCurrentContext(mImGuiContext);
    ImGuiIO &io = ImGui::GetIO();
    ImDrawData *draw_data = ImGui::GetDrawData();
    draw_data->ScaleClipRects(io.DisplayFramebufferScale);

    updateMatrices();

    // We are using the OpenGL fixed pipeline because messing with the
    // shader-state in X-Plane is not very well documented, but using the fixed
    // function pipeline is.

    // 1TU + Alpha settings, no depth, no fog.
    XPLMSetGraphicsState(0, 1, 0, 1, 1, 0, 0);
    GLint last_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
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
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawVert* vtx_buffer = cmd_list->VtxBuffer.Data;
        const ImDrawIdx* idx_buffer = cmd_list->IdxBuffer.Data;
        glVertexPointer(2, GL_FLOAT, sizeof(ImDrawVert), (const GLvoid*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, pos)));
        glTexCoordPointer(2, GL_FLOAT, sizeof(ImDrawVert), (const GLvoid*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, uv)));
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ImDrawVert), (const GLvoid*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, col)));

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback) {
                pcmd->UserCallback(cmd_list, pcmd);
            } else {
                glBindTexture(GL_TEXTURE_2D, (GLuint)(uintptr_t)pcmd->TextureId);

                // Scissors work in viewport space - must translate the coordinates from ImGui -> Boxels, then Boxels -> Native.
                //FIXME: it must be possible to apply the scale+transform manually to the projection matrix so we don't need to doublestep.
                int bTop, bLeft, bRight, bBottom;
                translateImGuiToBoxel(pcmd->ClipRect.x, pcmd->ClipRect.y, bLeft, bTop);
                translateImGuiToBoxel(pcmd->ClipRect.z, pcmd->ClipRect.w, bRight, bBottom);
                int nTop, nLeft, nRight, nBottom;
                boxelsToNative(bLeft, bTop, nLeft, nTop);
                boxelsToNative(bRight, bBottom, nRight, nBottom);
                glScissor(nLeft, nBottom, nRight-nLeft, nTop-nBottom);
                glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer);
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
    glBindTexture(GL_TEXTURE_2D, last_texture);
    glPopAttrib();
    glPopClientAttrib();
}

void
ImgWindow::translateToImGuiSpace(int inX, int inY, float &outX, float &outY) {
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

bool ImgWindow::checkScreenAndPlace()
{
    if (mDecoration == xplm_WindowDecorationSelfDecorated && !mIsInVR && !XPLMWindowIsPoppedOut(mWindowID)) {
        bool needResize = false;
        int sLeft, sTop, sRight, sBotoom;
        XPLMGetScreenBoundsGlobal(&sLeft, &sTop, &sRight, &sBotoom);
        if (mLeft < sLeft) {
            mLeft = sLeft;
            mRight = sLeft + mWidth;
            needResize = true;
        }
        if (mRight > sRight) {
            mRight = sRight;
            mLeft = sRight - mWidth;
            needResize = true;
        }
        if (mBottom < sBotoom) {
            mBottom = sBotoom;
            mTop = sBotoom + mHeight;
            needResize = true;
        }
        if (mTop > sTop) {
            mTop = sTop;
            mBottom = sTop - mHeight;
            needResize = true;
        }
        if (needResize) {
            XPLMSetWindowGeometry(mWindowID, mLeft, mTop, mRight, mBottom);
            return true;
        }
        return false;
    }
    return false;
}

void
ImgWindow::translateImGuiToBoxel(float inX, float inY, int &outX, int &outY) {
    outX = (int) (mLeft + inX);
    outY = (int) (mTop - inY);
}


void
ImgWindow::updateImGui() {

    ImGui::SetCurrentContext(mImGuiContext);
    auto &io = ImGui::GetIO();

    // check the window is within the screen size (for self decorated)
    checkScreenAndPlace();

    // transfer the window geometry to ImGui
    XPLMGetWindowGeometry(mWindowID, &mLeft, &mTop, &mRight, &mBottom);

    mWidth = mRight - mLeft;
    mHeight = mTop - mBottom;

    io.DisplaySize = ImVec2(static_cast<float>(mWidth),
                            static_cast<float>(mHeight));
    // in boxels, we're always scale 1, 1.
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

    // get mouse position and update imgui
    // do not update mouse coordinates when window is not in front
    if (XPLMIsWindowInFront(mWindowID)) {
        int mouse_x, mouse_y;
        XPLMGetMouseLocationGlobal(&mouse_x, &mouse_y);
        float outX, outY;
        translateToImGuiSpace(mouse_x, mouse_y, outX, outY);
        io.MousePos = ImVec2(outX, outY);
    }

    float time = XPLMGetElapsedTime();
    io.DeltaTime = time - mLastTimeDrawn;
    mLastTimeDrawn = time;

    ImGui::NewFrame();

    PreBuildInterface();

    BuildInterface();

    PostBuildInterface();

    mFirstRender = false;
}

void ImgWindow::PostBuildInterface() {
    ImGui::End();
}

void ImgWindow::PreBuildInterface() {
    // set windows position and size
    ImGui::SetNextWindowPos(ImVec2((float) 0.0, (float) 0.0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(
                ImVec2(static_cast<float>(mWidth), static_cast<float>(mHeight)),
                ImGuiCond_Always);

    // and construct the window
    ImGui::Begin(mWindowTitle.c_str(), nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoCollapse |
                 ImGuiWindowFlags_NoMove);
}

/// Main loop function to update ImGui and render the window
void ImgWindow::drawWindowCB(XPLMWindowID inWindowID, void *inRefcon) {
    auto *thisWindow = reinterpret_cast<ImgWindow *>(inRefcon);

    ImGui::SetCurrentContext(thisWindow->mImGuiContext);

    thisWindow->updateImGui();

    ImGui::Render();

    thisWindow->renderImGui();

    thisWindow->keyboardFocusHandler();
}

void ImgWindow::keyboardFocusHandler() {
    // Handle window focus.
    auto &io = ImGui::GetIO();
    int hasKeyboardFocus = XPLMHasKeyboardFocus(mWindowID);
    if (io.WantCaptureKeyboard && !hasKeyboardFocus) {
        XPLMTakeKeyboardFocus(mWindowID);
    } else if (!io.WantCaptureKeyboard && hasKeyboardFocus) {
        XPLMTakeKeyboardFocus(nullptr);
        // reset keysdown otherwise we'll think any keys used to defocus the keyboard are still down!
        for (auto &key : io.KeysDown) {
            key = false;
        }
        // Fix SHIFT remains active
        io.KeyShift = false;
        ImGui::ClearActiveID();
    }
}

int ImgWindow::handleMouseClickCB(XPLMWindowID inWindowID, int x, int y,
                                  XPLMMouseStatus inMouse, void *inRefcon) {
    auto *thisWindow = reinterpret_cast<ImgWindow *>(inRefcon);
    return thisWindow->handleMouseClickGeneric(x, y, inMouse, 0);
}

int ImgWindow::handleMouseClickGeneric(int x, int y, XPLMMouseStatus inMouse,
                                       int button) {
    ImGui::SetCurrentContext(mImGuiContext);
    ImGuiIO &io = ImGui::GetIO();
    static int lastX = x, lastY = y;
    int dx, dy;
    static int gDragging = 0;

    switch (inMouse) {
    case xplm_MouseDown:
        if ((mDecoration != xplm_WindowDecorationRoundRectangle) &&
                !ImGui::IsAnyItemHovered()) {
            gDragging = 1;
        }
        io.MouseDown[button] = true;
        break;
    case xplm_MouseDrag:
        // Drag only if we use window without X-Plane decorations
        // and only if the mouse coordinates really changed!
        // Otherwise the window could not be resized
        // FIXME: fix resizing for different anchor points
        dx = x - lastX;
        dy = y - lastY;
        if (mDecoration != xplm_WindowDecorationRoundRectangle &&
                gDragging && (dx || dy) && !mIsInVR && !XPLMWindowIsPoppedOut(mWindowID)) {
            mLeft += dx;
            mRight += dx;
            mTop += dy;
            mBottom += dy;
            XPLMSetWindowGeometry(mWindowID, mLeft, mTop, mRight,
                                  mBottom);
        }
        io.MouseDown[button] = true;
        break;
    case xplm_MouseUp:
        io.MouseDown[button] = false;
        gDragging = 0;
        break;
    default:
        // dunno!
        break;
    }
    lastX = x;
    lastY = y;
    return 1;
}

float ImgWindow::flightLoopHandler(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void * inRefcon)
{
    auto *thisWindow = reinterpret_cast<ImgWindow *>(inRefcon);

    if (thisWindow->mSelfHide) {
        XPLMSetWindowIsVisible(thisWindow->mWindowID, false);
        thisWindow->mSelfHide = false;
    }

    if (thisWindow->mSelfResize) {
        thisWindow->Resize(thisWindow->mResizeWidth, thisWindow->mResizeHeight,
                           thisWindow->mResizeAnchor);
        thisWindow->mSelfResize = false;
    }

    if (thisWindow->mSelfPositioning) {
        XPLMSetWindowPositioningMode(thisWindow->mWindowID, thisWindow->tempPositioningMode,
                                     thisWindow->tempMonitorIndex);
        thisWindow->mSelfPositioning = false;
    }

    if (thisWindow->mSelfPlace) {
        thisWindow->Place(thisWindow->mX, thisWindow->mY, thisWindow->mPlaceAnchor);
        thisWindow->mSelfPlace = false;
    }

    if (thisWindow->mSelfDestruct) {
        delete thisWindow;
    }
    return -1.0f;
}

void ImgWindow::handleKeyFuncCB(XPLMWindowID inWindowID, char inKey,
                                XPLMKeyFlags inFlags, char inVirtualKey,
                                void *inRefcon, int losingFocus) {
    auto *thisWindow = reinterpret_cast<ImgWindow *>(inRefcon);
    ImGui::SetCurrentContext(thisWindow->mImGuiContext);
    ImGuiIO &io = ImGui::GetIO();
    if (io.WantCaptureKeyboard) {
        auto vk = static_cast<unsigned char>(inVirtualKey);
        io.KeysDown[vk] = (inFlags & xplm_DownFlag) == xplm_DownFlag;
        io.KeyShift = (inFlags & xplm_ShiftFlag) == xplm_ShiftFlag;
        io.KeyAlt = (inFlags & xplm_OptionAltFlag) == xplm_OptionAltFlag;
        io.KeyCtrl = (inFlags & xplm_ControlFlag) == xplm_ControlFlag;
        if ((inFlags & xplm_DownFlag) == xplm_DownFlag
                && !(io.KeyCtrl && !io.KeyAlt)
                && isprint(inKey)) {
            char smallStr[2] = {inKey, 0};
            io.AddInputCharactersUTF8(smallStr);
        }
    }
    if(losingFocus){
        ImGui::ClearActiveID();
        ImGui::CaptureKeyboardFromApp(false);
    }
}

XPLMCursorStatus ImgWindow::handleCursorFuncCB(XPLMWindowID inWindowID,
                                               int x, int y, void *inRefcon) {
    auto *thisWindow = reinterpret_cast<ImgWindow *>(inRefcon);
    ImGui::SetCurrentContext(thisWindow->mImGuiContext);
    ImGuiIO &io = ImGui::GetIO();
    //float outX, outY;
    //thisWindow->translateToImGuiSpace(x, y, outX, outY);
    //io.MousePos = ImVec2(outX, outY);
    //FIXME: Maybe we can support imgui's cursors a bit better?
    return xplm_CursorDefault;
}

int ImgWindow::handleMouseWheelFuncCB(XPLMWindowID inWindowID, int x, int y,
                                      int wheel, int clicks, void *inRefcon) {
    auto *thisWindow = reinterpret_cast<ImgWindow *>(inRefcon);
    ImGui::SetCurrentContext(thisWindow->mImGuiContext);
    ImGuiIO &io = ImGui::GetIO();

    float outX, outY;
    thisWindow->translateToImGuiSpace(x, y, outX, outY);
    io.MousePos = ImVec2(outX, outY);
    switch (wheel) {
    case 0:
        io.MouseWheel = static_cast<float>(clicks);
        break;
    case 1:
        io.MouseWheelH = static_cast<float>(clicks);
        break;
    default:
        // unknown wheel
        break;
    }
    return 1;
}

int ImgWindow::handleRightClickFuncCB(XPLMWindowID inWindowID, int x, int y,
                                      XPLMMouseStatus inMouse, void *inRefcon) {
    auto *thisWindow = reinterpret_cast<ImgWindow *>(inRefcon);
    return thisWindow->handleMouseClickGeneric(x, y, inMouse, 1);
}

void ImgWindow::SetWindowTitle(const std::string &title) {
    mWindowTitle = title;
    XPLMSetWindowTitle(mWindowID, mWindowTitle.c_str());
}

void ImgWindow::SetResizingLimits(int inMinWidthBoxels,
                                  int inMinHeightBoxels,
                                  int inMaxWidthBoxels,
                                  int inMaxHeightBoxels) {
    XPLMSetWindowResizingLimits(mWindowID, inMinWidthBoxels, inMinHeightBoxels,
                                inMaxWidthBoxels, inMaxHeightBoxels);
}

void ImgWindow::SetPositioningMode(XPLMWindowPositioningMode mode, int inMonitorIndex)
{
    XPLMSetWindowPositioningMode(mWindowID, mode, inMonitorIndex);
}

void ImgWindow::SafePositioningModeSet(XPLMWindowPositioningMode mode,
                                       int inMonitorIndex) {
    tempPositioningMode = mode;
    tempMonitorIndex = inMonitorIndex;
    mSelfPositioning = true;
}

void ImgWindow::SetGravity(float inLeftGravity, float inTopGravity,
                           float inRightGravity, float inBottomGravity) {
    XPLMSetWindowGravity(mWindowID, inLeftGravity, inTopGravity, inRightGravity,
                         inBottomGravity);
}

void ImgWindow::Resize(int width, int height, ImgWindow::Anchor anchor) {
    if (!XPLMWindowIsPoppedOut(mWindowID)) {
        mWidth = width;
        mHeight = height;
        switch (anchor) {
        case TopLeft:
            mRight = mLeft + width;
            mBottom = mTop - height;
            break;
        case TopRight:
            mLeft = mRight - width;
            mBottom = mTop - height;
            break;
        case BottomLeft:
            mRight = mLeft + width;
            mTop = mBottom + height;
            break;
        case BottomRight:
            mLeft = mRight - width;
            mTop = mBottom + height;
            break;
        case Center:
            mLeft = (2 * mLeft + mWidth - width) / 2;
            mRight = mLeft + width;
            mBottom = (2 * mBottom + mHeight - height) / 2;
            mTop = mBottom + height;
            break;
        default:
            break;
        }
        if (IsInVR())
            XPLMSetWindowGeometryVR(mWindowID, mWidth, mHeight);
        else
            XPLMSetWindowGeometry(mWindowID, mLeft, mTop, mRight, mBottom);
    }
}

void ImgWindow::Place(int x, int y, ImgWindow::Anchor anchor) {
    if (!XPLMWindowIsPoppedOut(mWindowID)) {
        switch (anchor) {
        case TopLeft:
            mLeft = x;
            mRight = mLeft + mWidth;
            mTop = y;
            mBottom = mTop - mHeight;
            break;
        case TopRight:
            mRight = x;
            mLeft = mRight - mWidth;
            mTop = y;
            mBottom = mTop - mHeight;
            break;
        case BottomLeft:
            mLeft = x;
            mRight = mLeft + mWidth;
            mBottom = y;
            mTop = mBottom + mHeight;
            break;
        case BottomRight:
            mRight = x;
            mLeft = mRight - mWidth;
            mBottom = y;
            mTop = mBottom + mHeight;
            break;
        case Center:
            mLeft = x - mWidth / 2;
            mRight = mLeft + mWidth;
            mTop = y + mHeight / 2;
            mBottom = mTop - mHeight;
            break;
        default:
            break;
        }
        if (!checkScreenAndPlace())
            XPLMSetWindowGeometry(mWindowID, mLeft, mTop, mRight, mBottom);
    }
}

void ImgWindow::SafePlace(int x, int y, ImgWindow::Anchor anchor)
{
    mX = x;
    mY = y;
    mPlaceAnchor = anchor;
    mSelfPlace = true;
}

void ImgWindow::SetVisible(bool inIsVisible) {
    if (inIsVisible)
        MoveForVR();
    if (GetVisible() == inIsVisible) {
        // if the state is already correct, no-op.
        return;
    }
    if (inIsVisible) {
        if (!OnShow()) {
            // chance to early abort.
            return;
        }
    }
    XPLMSetWindowIsVisible(mWindowID, inIsVisible);
}

void ImgWindow::MoveForVR() {
    // if we're trying to display the window, check the state of the VR flag
    // - if we're VR enabled, explicitly move the window to the VR world.
    if (XPLMGetDatai(gVrEnabledRef)) {
        XPLMSetWindowPositioningMode(mWindowID, xplm_WindowVR, 0);
        mIsInVR = true;
    } else {
        if (mIsInVR) {
            XPLMSetWindowPositioningMode(mWindowID, mPreferredPositioningMode, -1);
            mIsInVR = false;
        }
    }
}

bool ImgWindow::IsInVR() const
{
    return mIsInVR;
}

bool ImgWindow::GetVisible() const {
    return XPLMGetWindowIsVisible(mWindowID) != 0;
}

bool ImgWindow::OnShow() {
    return true;
}

void ImgWindow::SafeDelete() {
    mSelfDestruct = true;
}

void ImgWindow::SafeHide() {
    mSelfHide = true;
}

void ImgWindow::SafeResize(int width, int height, Anchor anchor) {
    mSelfResize = true;
    mResizeWidth = width;
    mResizeHeight = height;
    mResizeAnchor = anchor;
}
