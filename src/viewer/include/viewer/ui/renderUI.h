//
// Created by ftc on 25-5-13.
//

#ifndef TOY_RENDERER_UPDATE_RENDERUI_H
#define TOY_RENDERER_UPDATE_RENDERUI_H

#include "ImGuiFileDialog/ImGuiFileDialog.h"
#include "render/render.h"
#include "ui.h"

class RenderUI : public UI{
public:
    explicit RenderUI(std::shared_ptr<Viewer> viewer) {
        mViewer = std::move(viewer);
        mName = "Render";
    }
    void render() override
    {
        if(!mVisible)return;

        ImGui::SetNextWindowSize(ImVec2(mViewer->getWidth() * 0.1f, 60), ImGuiCond_Once);
        ImGui::SetNextWindowPos(ImVec2(mViewer->getWidth() * 0.9f, 90), ImGuiCond_Once);

        ImGui::Begin(mName.c_str(), &mVisible, ImGuiWindowFlags_MenuBar);
        if (ImGui::BeginMenuBar())
        {
            ImGui::SetWindowFontScale(1.8f);

            SHADER_BACKEND_TYPE currentShaderBackendType = mViewer->getBackendType();

            // Material shader option
            if (currentShaderBackendType == SHADER_BACKEND_TYPE::VULKAN)
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));

            if (ImGui::MenuItem("Vulkan"))
            {
                if(currentShaderBackendType == SHADER_BACKEND_TYPE::OPENGL)
                    mViewer->setSwitchType();
            }

            if (currentShaderBackendType == SHADER_BACKEND_TYPE::VULKAN)
                ImGui::PopStyleColor();
            if (currentShaderBackendType == SHADER_BACKEND_TYPE::OPENGL)
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));

            if (ImGui::MenuItem("OpenGL"))
            {
                if(currentShaderBackendType == SHADER_BACKEND_TYPE::VULKAN)
                    mViewer->setSwitchType();
            }

            if (currentShaderBackendType == SHADER_BACKEND_TYPE::OPENGL)
                ImGui::PopStyleColor();


            ImGui::EndMenuBar();
        }
        ImGui::End();
    }

};

#endif //TOY_RENDERER_UPDATE_RENDERUI_H
