//
// Created by z002fctv on 14.11.2018.
//

#include "testwindow.h"

TestWindow::TestWindow(ImFontAtlas *fontAtlas) : ImgWindow(fontAtlas) {
    init(100, 800, 800, 100, xplm_WindowDecorationRoundRectangle);
}

void TestWindow::buildInterface() {
    auto &io = ImGui::GetIO();
    ImGui::Text("ImGui screen size: width = %f  height = %f", io.DisplaySize.x, io.DisplaySize.y);
    ImGui::Text("Want capture mouse: %i", io.WantCaptureMouse);
    ImGui::Text("Want capture keyboard: %i", io.WantCaptureKeyboard);
    int mouse_x, mouse_y;
    XPLMGetMouseLocationGlobal(&mouse_x, &mouse_y);
    ImGui::Text("Mouse position X-Plane: x = %i  y = %i", mouse_x, mouse_y);
    ImGui::Text("Mouse position ImGui: x = %f  y = %f", io.MousePos.x, io.MousePos.y);
    ImGui::Text("Is any items active or hovered: %i", ImGui::IsAnyItemHovered());
}
