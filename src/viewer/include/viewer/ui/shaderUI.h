//
// Created by ftc on 25-4-13.
//

#ifndef TOY_RENDERER_UPDATE_SHADERUI_H
#define TOY_RENDERER_UPDATE_SHADERUI_H

#include "ImGuiFileDialog/ImGuiFileDialog.h"
#include "shader/shader.h"
#include "ui.h"

class ShaderUI : public UI{
public:
    explicit ShaderUI(std::shared_ptr<Viewer> viewer) {
        mViewer = std::move(viewer);
        mName = "Shader";
    }
    void render() override
    {
        if(!mVisible)return;

        ImGui::SetNextWindowSize(ImVec2(mViewer->getWidth() * 0.1f, 60), ImGuiCond_Once);
        ImGui::SetNextWindowPos(ImVec2(mViewer->getWidth() * 0.9f, 30), ImGuiCond_Once);

        ImGui::Begin(mName.c_str(), &mVisible, ImGuiWindowFlags_MenuBar);
        if (ImGui::BeginMenuBar())
        {
            ImGui::SetWindowFontScale(2.0f);

            SHADER_TYPE currentShaderType = mViewer->getRender()->getShaderType();

            // Material shader option
            if (currentShaderType == SHADER_TYPE::MATERIAL)
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));

            if (ImGui::MenuItem("M"))
            {
                mViewer->getRender()->setShaderType(SHADER_TYPE::MATERIAL);
            }

            if (currentShaderType == SHADER_TYPE::MATERIAL)
                ImGui::PopStyleColor();

            if (currentShaderType == SHADER_TYPE::Blinn_Phong)
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));

            if (ImGui::MenuItem("B"))
            {
                mViewer->getRender()->setShaderType(SHADER_TYPE::Blinn_Phong);
            }

            if (currentShaderType == SHADER_TYPE::Blinn_Phong)
                ImGui::PopStyleColor();

            if (currentShaderType == SHADER_TYPE::WIREFRAME)
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));

            if (ImGui::MenuItem("W"))
            {
                mViewer->getRender()->setShaderType(SHADER_TYPE::WIREFRAME);
            }

            if (currentShaderType == SHADER_TYPE::WIREFRAME)
                ImGui::PopStyleColor();

            ImGui::EndMenuBar();
        }
        ImGui::End();
    }
};

#endif //TOY_RENDERER_UPDATE_SHADERUI_H