//
// Created by 18067 on 25-4-11.
//

#ifndef MODELUI_H
#define MODELUI_H

#include "ImGuiFileDialog/ImGuiFileDialog.h"
#include "ui.h"

class ModelUI : public UI {
public:
    explicit ModelUI(std::shared_ptr<Viewer> viewer) {
        mViewer = std::move(viewer);
        mName = "Model";
    }
    void render() override
    {
        if (!mVisible) return;

        ImGui::SetNextWindowSize(ImVec2(mViewer->getWidth() / 5, mViewer->getHeight() / 2.5), ImGuiCond_Once);
        ImGui::SetNextWindowPos(ImVec2(30, 30), ImGuiCond_Once);

        ImGui::Begin(mName.c_str(), &mVisible, ImGuiWindowFlags_MenuBar);
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::MenuItem("Add"))
            {
                ImGuiFileDialog::Instance()->OpenDialog(
                    "ChooseModelDlg", "Choose Model File",
                    "3D Models{.obj,.ply},JSON{.json}");
            }
            ImGui::EndMenuBar();
        }

        if(ImGuiFileDialog::Instance()->Display("ChooseModelDlg"))
        {
            if(ImGuiFileDialog::Instance()->IsOk())
            {
                std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
                std::cout << filePath << "\n";
                mViewer->getRender()->addModel(mViewer->getScene()->addModel(filePath));
            }
            ImGuiFileDialog::Instance()->Close();
        }
        ImGui::TextColored(ImVec4(1,1,1,1), "Model List");
        ImGui::BeginChild("Scrolling");
        auto models = mViewer->getScene()->getModels();
        for (const auto& model : models)
        {
            ImGui::TextWrapped("%s", model->getName().c_str());
        }
        ImGui::EndChild();


        ImGui::End();
    }
};

#endif //MODELUI_H
