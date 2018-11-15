//
// Created by z002fctv on 14.11.2018.
//

#include "gui.h"

Gui::Gui() : ImgX() {
    init();
}

void Gui::buildInterface() {
    auto& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2((float)100.0, (float) 100.0), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(500), static_cast<float>(500)), ImGuiCond_Once);
    ImGui::Begin("Hallo world", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse);
    ImGui::Text("ImGui screen size: width = %f  height = %f", io.DisplaySize.x, io.DisplaySize.y);
    ImGui::Text("Want capture mouse: %i", io.WantCaptureMouse);
    ImGui::Text("Want capture keyboard: %i", io.WantCaptureKeyboard);
    int mouse_x, mouse_y;
    XPLMGetMouseLocationGlobal(&mouse_x, &mouse_y);
    ImGui::Text("Mouse position X-Plane: x = %i  y = %i", mouse_x, mouse_y);
    ImGui::Text("Mouse position ImGui: x = %f  y = %f", io.MousePos.x, io.MousePos.y);
    ImGui::Text("Is any items active or hovered: %i", ImGui::IsAnyItemHovered());
    ImGui::End();
}
