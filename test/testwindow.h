/*
 *   Imgx test plugin for X-Plane
 *   Created by Roman Liubich
 *   Inspired by  William Good and Christopher Collins
 *
 *   This is also an example how to use imgx in the final project
 */

#ifndef IMGX_GUI_H
#define IMGX_GUI_H

#include "imgwindow.h"

class TestWindow : public ImgWindow {

public:
    explicit TestWindow(ImFontAtlas *fontAtlas = nullptr);
    ~TestWindow() final = default;

protected:
    void BuildInterface() override;
};


#endif //IMGX_GUI_H