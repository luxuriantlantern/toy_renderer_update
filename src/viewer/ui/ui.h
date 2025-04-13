//
// Created by ftc on 25-4-10.
//

#ifndef TOY_RENDERER_UPDATE_UI_H
#define TOY_RENDERER_UPDATE_UI_H

#include "imgui.h"
#include "imgui_internal.h"
#include <string>
#include <memory>

class Viewer;

class ModelUI;
class ShaderUI;

class UI {
public:
    UI() = default;
    ~UI() = default;

    void setVisible(bool visible) { mVisible = visible; }
    virtual void render() = 0;
protected:
    std::string mName;
    bool mVisible = true;
    std::shared_ptr<Viewer> mViewer;
};


#endif //TOY_RENDERER_UPDATE_UI_H
