/*
 *   Another ImGui port for X-Plane
 *   Created by Roman Liubich
 *   Inspired by Christopher Collins
 */

#ifndef IMGX_H
#define IMGX_H

#include "XPLMDisplay.h"
#include "XPLMProcessing.h"
#include "imgui.h"

#include <string>

class ImgX {
private:

    static float flightCB(
            float inElapsedSinceLastCall,
            float inElapsedTimeSinceLastFlightLoop,
            int inCounter,
            void *inRefcon);

    static void drawWindowCB(
            XPLMWindowID inWindowID,
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

    static void initPlatformInterface();

    static void shutdownPlatformInterface();

    static void createWindow(ImGuiViewport* viewport);

    static void destroyWindow(ImGuiViewport* viewport);

    static void setWindowSize(ImGuiViewport* viewport, ImVec2 size);

//
//    void renderImGui(ImDrawData *draw_data);
//
//    void updateImgui();
//
//    void updateMatrices();
//
//    void boxelsToNative(int x, int y, int &outX, int &outY);
//
//    void translateImguiToBoxel(float inX, float inY, int &outX, int &outY);
//
//    void translateToImguiSpace(int inX, int inY, float &outX, float &outY);
//
    float mModelView[16], mProjection[16];
    int mViewport[4];
//    bool mSelfDestruct;
//
//    std::string mWindowTitle;
//
//    XPLMWindowID mWindowID;
//    ImGuiContext *mImGuiContext;
//    //GLuint mFontTexture;
    XPLMFlightLoopID mFlightLoop;
    bool mIsInVR = false;
//
    int mTop;
    int mBottom;
    int mLeft;
    int mRight;
    int mWidth;
    int mHeight;
//
    XPLMWindowLayer mPreferredLayer;

protected:
//    /** mFirstRender can be checked during buildInterface() to see if we're
//     * being rendered for the first time or not.  This is particularly
//     * important for windows that use Columns as SetColumnWidth() should only
//     * be called once.
//     *
//     * There may be other times where it's advantageous to make specific ImGui
//     * calls once and once only.
//     */
//    bool mFirstRender;
//
//
//    /** Construct a window with the specified bounds
//     *
//     * @param left Left edge of the window's contents in global boxels.
//     * @param top Top edge of the window's contents in global boxels.
//     * @param right Right edge of the window's contents in global boxels.
//     * @param bottom Bottom edge of the window's contents in global boxels.
//     * @param decoration The decoration style to use (see notes)
//     * @param layer the preferred layer to present this window in (see notes)
//     *
//     * @note The decoration should generally be one presented/rendered by XP -
//     * the ImGui window decorations are very intentionally supressed by
//     * ImgWindow to allow them to fit in with the rest of the simulator.
//     *
//     * @note The only layers that really make sense are Floating and Modal.  Do
//     * not set VR layer here however unless the window is ONLY to be rendered
//     * in VR.
//     */
    explicit ImgX(XPLMWindowLayer layer = xplm_WindowLayerFloatingWindows);


/// init - dont call overridden methods from constructor, use this function instead
    void init();

/// configureImguiContext - can be used to customise ImGui before the font texture is bound
    virtual void configureImguiContext();

//    /** SetWindowTitle sets the title of the window both in the ImGui layer and
//     * at the XPLM layer.
//     *
//     * @param title the title to set.
//     */
//    void SetWindowTitle(const std::string &title);
//
//    /** moveForVR() is an internal helper for moving the window to either it's
//     * preferred layer or the VR layer depending on if the headset is in use.
//     */
//    void moveForVR();

/// buildInterface() is the method where you can define your ImGui interface
/// and handle events.  It is called every frame the window is drawn.
    virtual void buildInterface() = 0;
//
//    /** onShow() is called before making the Window visible.  It provides an
//     * oportunity to prevent the window being shown.
//     *
//     * @note the implementation in the base-class is a null handler.  You can
//     * safely override this without chaining.
//     *
//     * @return true if the Window should be shown, false if the attempt to show
//     * should be suppressed.
//     */
//    virtual bool onShow();
//
//    /** SafeDelete() can be used within buildInterface() to get the object to
//     * self-delete once it's finished rendering this frame.
//     */
//    void SafeDelete();

public:
    virtual ~ImgX();

    /// Draw - start drawing loop
    void Draw();

//    /** SetVisible() makes the window visible after making the onShow() call.
//     * It is also at this time that the window will be relocated onto the VR
//     * display if the VR headset is in use.
//     *
//     * @param inIsVisible true to be displayed, false if the window is to be
//     * hidden.
//     */
//    virtual void SetVisible(bool inIsVisible);
//
//    /** GetVisible() returns the current window visibility.
//     * @return true if the window is visible, false otherwise.
//    */
//    bool GetVisible() const;

};

#endif //IMGX_H
