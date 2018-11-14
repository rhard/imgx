//
// Created by z002fctv on 14.11.2018.
//

#ifndef IMGX_GUI_H
#define IMGX_GUI_H

#include "imgx.h"

class Gui : public ImgX {

public:
    Gui();
    ~Gui() final = default;

protected:
    void buildInterface() override;
};


#endif //IMGX_GUI_H