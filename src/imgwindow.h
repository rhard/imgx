/*
 * imgwindow.h
 *
 * Integration for dear imgui into X-Plane.
 *
 * Copyright (C) 2018, Christopher Collins
*/

/// Another ImGui port for X-Plane
/// created by Christopher Collins
/// Modified by Roman Liubich

#ifndef IMGWINDOW_H
#define IMGWINDOW_H

#include "XPLMDisplay.h"
#include "XPLMProcessing.h"
#include "imgui.h"

#include <string>

/// \file
/// This file contains the declaration of the ImgWindow class, which is the
/// base class for all ImGui driven X-Plane windows.
/// \brief ImgWindow is a Window for creating dear ImGui widgets within.
///
/// There's a few traps to be aware of when using dear ImGui with X-Plane:
///
/// 1. The Dear ImGui coordinate scheme is inverted in the Y axis vs the
/// X-Plane (and OpenGL default) scheme. You must be careful if you're trying
/// to directly manipulate positioning of widgets rather than letting ImGui
/// self-layout. There are (private) functions in ImgWindow to do the
/// coordinate mapping.
///
/// 2. The Dear ImGui rendering space is only as big as the window - this means
/// popup elements cannot be larger than the parent window. This was unavoidable
/// on XP11 because of how popup windows work and the possibility for
/// negative  coordinates (which ImGui doesn't like).
///
/// 3. There is no way to detect if the window is hidden without a per-frame
/// processing loop or similar.
///
/// \note It should be possible to map globally on XP9 & XP10 letting you run
/// popups as large as you need, or to use the ImGui native titlebars instead of
/// the XP10 ones - source for this may be provided later, but could also be
/// trivially adapted from this one by adjusting the way the space is translated
/// and mapped in the DrawWindowCB and constructor.
class ImgWindow {
public:
    /// Anchor point used to place the window in X-Plane world
    enum Anchor {
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight,
        Center
    };

    virtual ~ImgWindow();

    /// Makes the window visible after making the onShow() call.
    /// It is also at this time that the window will be relocated onto the VR
    /// display if the VR headset is in use.
    /// \param inIsVisible True to be displayed, false if the window is to be hidden.
    virtual void SetVisible(bool inIsVisible);

    /// Returns current window visibility.
    /// \return true if the window is visible, false otherwise
    bool GetVisible() const;

    /// Resize the window relative to anchor point
    /// \param width width of the window to resize
    /// \param height height of the window to resize
    /// \param anchor anchor point to resize
    void Resize(int width, int height, Anchor anchor = TopLeft);

    /// Place the window anchor point
    /// \param x x coordinate to place the window
    /// \param y y coordinate to place the window
    /// \param anchor anchor point to place the window
    void Place(int x, int y, Anchor anchor = TopLeft);

    /// Same as previous but could be called from the UI loop
    /// \param x x coordinate to place the window
    /// \param y y coordinate to place the window
    /// \param anchor anchor point to place the window
    void SafePlace(int x, int y, Anchor anchor = TopLeft);

    /// Get a text from clipboard
    /// \param user_data - not used here
    /// \return clipboard text
    static const char *GetClipboardImGuiWrapper(void *user_data);

    /// Set a text to clipboard
    /// \param user_data - not used here
    /// \param text text to set
    static void SetClipboardImGuiWrapper(void *user_data, const char *text);
protected:
    /// Constructs a window with optional FontAtlas
    /// \param fontAtlas shared ImFontAtlas
    explicit ImgWindow(ImFontAtlas *fontAtlas = nullptr);

    /// Initialise a window with the specified parameters. Call this function
    /// in derived class constructor.
    /// \param width width of the window
    /// \param height height of the window
    /// \param x x coordinate to place the window anchor point
    /// \param y y coordinate to place the window anchor point
    /// \param anchor anchor point to place the window (default to TopLeft)
    /// \param decoration decoration style to use
    /// \param layer preferred layer to present this window in
    /// \param mode preferred position mode to present this window
    void Init(int width, int height, int x, int y, Anchor anchor = TopLeft,
              XPLMWindowDecoration decoration = xplm_WindowDecorationRoundRectangle,
              XPLMWindowLayer layer = xplm_WindowLayerFloatingWindows,
              XPLMWindowPositioningMode mode = xplm_WindowPositionFree);

    /// can be used to customise ImGui context for user needs
    virtual void ConfigureImGuiContext();

    /// Override this method if you want to define your own ImGui window
    /// creation. It might be used to setup ImGui window flags, adjust window
    /// background color, push style variables etc. It is called every frame
    /// the window is drawn.
    /// \note It must contain ImGui::Begin() command.
    /// \code
    ///     TODO: write an example here
    /// \endcode
    virtual void PreBuildInterface();

    /// Main method for GUI definition and event handling. It is called every
    /// frame the window is drawn.
    /// \note You must NOT delete, hide, resize the window object inside
    /// buildInterface(). Use SafeDelete(), SafeHide(), SafeResize() for that.
    virtual void BuildInterface() = 0;

    /// Override this if you want to define your own window finalize method.
    /// It might be used to pop pushed style flags etc. It is called every
    /// frame the window is drawn.
    /// \note It must contain ImGui::End() command.
    virtual void PostBuildInterface();

    /// Sets the title of the window both in the ImGui layer and in the XPLM
    /// layer
    /// \param title title to set
    void SetWindowTitle(const std::string &title);

    /// Can be used within buildInterface() to get the object to self-delete
    /// once it's finished rendering this frame.
    void SafeDelete();

    /// Can be used within buildInterface() to hide the window once it's
    /// finished rendering this frame.
    void SafeHide();

    /// Can be used within buildInterface() to resize the window once it's
    /// finished rendering this frame.
    /// \param width width of the window to resize
    /// \param height height of the window to resize
    /// \param anchor anchor point to resize
    void SafeResize(int width, int height, Anchor anchor = TopLeft);

    /// Set window resizing limits
    /// \param inMinWidthBoxels min windows width in boxels
    /// \param inMinHeightBoxels min windows height in boxels
    /// \param inMaxWidthBoxels max windows width in boxels
    /// \param inMaxHeightBoxels max windows height in boxels
    void SetResizingLimits(int inMinWidthBoxels,
                           int inMinHeightBoxels,
                           int inMaxWidthBoxels,
                           int inMaxHeightBoxels);

    /// An internal helper for moving the window to either it's preferred
    /// layer or the VR layer depending on if the headset is in use.
    void MoveForVR();

    /// Check if the windows is in VR
    /// \return true if window is in VR
    bool IsInVR();

    /// Called before making the window visible. It provides an opportunity
    /// to prevent the window being shown.
    /// \note the implementation in the base-class is a null handler. You can
    /// safely override this without chaining.
    /// \return true if the window should be shown, false if the attempt to show
    /// should be suppressed.
    virtual bool OnShow();

    /// Sets X-Plane window window positioning mode for the specified monitor
    /// \param mode XPLMWindowPositioningMode
    /// \param inMonitorIndex monitor for which mode is set (default to main)
    void SetPositioningMode(XPLMWindowPositioningMode mode,
                            int inMonitorIndex = -1);

    /// Same as previous but could be called from the UI loop
    /// \param mode XPLMWindowPositioningMode
    /// \param inMonitorIndex monitor for which mode is set (default to main)
    void SafePositioningModeSet(XPLMWindowPositioningMode mode,
                                int inMonitorIndex = -1);

    /// Sets X-Plane window gravity
    /// \param inLeftGravity left gravity (0.0 - 1.0)
    /// \param inTopGravity top gravity (0.0 - 1.0)
    /// \param inRightGravity right gravity (0.0 - 1.0)
    /// \param inBottomGravity bottom gravity (0.0 - 1.0)
    void SetGravity(float inLeftGravity, float inTopGravity,
                    float inRightGravity, float inBottomGravity);

    /// Is active when IngWindow is created with shared FontAtlas. User
    /// should load the fonts and build the texture before creating the
    /// window. Otherwise ImgWindow will build default font.
    bool mHasPrebuildFont;

    /// Can be checked during buildInterface() to see if we're being rendered
    /// for the first time or not. This is particularly important for windows
    /// that use columns as SetColumnWidth() should only be called once.
    /// There may be other times where it's advantageous to make specific ImGui
    /// calls once and once only
    bool mFirstRender;

    /// Window current coordinates and size in X-Plane space
    // TODO: maybe move to private
    int mTop, mBottom, mLeft, mRight, mWidth, mHeight;

    /// Window title
    std::string mWindowTitle;

private:
    static void drawWindowCB(XPLMWindowID inWindowID,
                             void *inRefcon);

    static int handleMouseClickCB(
            XPLMWindowID inWindowID,
            int x, int y,
            XPLMMouseStatus inMouse,
            void *inRefcon);

    static void handleKeyFuncCB(
            XPLMWindowID inWindowID,
            char inKey,
            XPLMKeyFlags inFlags,
            char inVirtualKey,
            void *inRefcon,
            int losingFocus);

    static XPLMCursorStatus handleCursorFuncCB(
            XPLMWindowID inWindowID,
            int x, int y,
            void *inRefcon);

    static int handleMouseWheelFuncCB(
            XPLMWindowID inWindowID,
            int x, int y,
            int wheel,
            int clicks,
            void *inRefcon);

    static int handleRightClickFuncCB(
            XPLMWindowID inWindowID,
            int x, int y,
            XPLMMouseStatus inMouse,
            void *inRefcon);

    int handleMouseClickGeneric(
            int x, int y,
            XPLMMouseStatus inMouse,
            int button = 0);

    static float flightLoopHandler(
            float                inElapsedSinceLastCall,
            float                inElapsedTimeSinceLastFlightLoop,
            int                  inCounter,
            void *               inRefcon);

    void renderImGui();

    void updateImGui();

    void boxelsToNative(int x, int y, int &outX, int &outY);

    void translateImGuiToBoxel(float inX, float inY, int &outX, int &outY);

    void translateToImGuiSpace(int inX, int inY, float &outX, float &outY);

    // returns true if self decorated window goes out of screen.
    // If true, the window position is updated to match screen size
    bool checkScreenAndPlace();

    bool mSelfDestruct, mSelfHide, mSelfResize, mSelfPositioning, mSelfPlace;

    ImGuiContext *mImGuiContext;
    XPLMWindowID mWindowID;
    bool mIsInVR;

    /// Variables to hold the size of the window and coordinates which must be set after calling
    /// SafeResize() and SafePlace()
    int mResizeWidth, mResizeHeight, mX, mY;

    /// Variable to hold resize and place anchor
    Anchor mResizeAnchor, mPlaceAnchor;

    /// Variables to provide safe positioning mode set
    XPLMWindowPositioningMode tempPositioningMode;
    int tempMonitorIndex;

    float mLastTimeDrawn = 0;

    XPLMWindowLayer mPreferredLayer;
    XPLMWindowDecoration mDecoration;
    XPLMWindowPositioningMode mPreferredPositioningMode;

    XPLMFlightLoopID flightLoopID;
};

#endif //IMGWINDOW_H
