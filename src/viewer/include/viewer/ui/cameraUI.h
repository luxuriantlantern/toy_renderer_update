//
// Created by ftc on 25-4-13.
//

#ifndef TOY_RENDERER_UPDATE_CAMERAUI_H
#define TOY_RENDERER_UPDATE_CAMERAUI_H

#include "ui.h"
#include "ImGuiFileDialog.h"

class CameraUI : public UI {
public:
    explicit CameraUI(std::shared_ptr<Viewer> viewer) {
        mViewer = std::move(viewer);
        mName = "Camera";
    }

    void render() override {
        if (!mVisible) return;

        ImGui::SetNextWindowSize(ImVec2(mViewer->getWidth() / 4, mViewer->getHeight() / 2), ImGuiCond_Once);
        ImGui::SetNextWindowPos(ImVec2(30, 30 + mViewer->getHeight() / 2.5), ImGuiCond_Once);

        ImGui::Begin(mName.c_str(), &mVisible, ImGuiWindowFlags_MenuBar);
        ImGui::SetWindowFontScale(1.6f);
        ImGui::TextWrapped("Type");
        const char* cameraTypes[] = { "Perspective", "Orthographic" };
        static int currentCameraType = mViewer->getCamera()->getType() == CameraType::PERSPECTIVE ? 0 : 1;
        if (ImGui::Combo("##Camera Type", &currentCameraType, cameraTypes, IM_ARRAYSIZE(cameraTypes))) {
            if (currentCameraType == 0) {
                mViewer->setCamera(std::make_shared<PerspectiveCamera>(*(mViewer->getCamera())));
            } else if (currentCameraType == 1) {
                mViewer->setCamera(std::make_shared<OrthographicCamera>(*(mViewer->getCamera())));
            }
        }
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Reset").x);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));
        if (ImGui::Button("Reset")) {
            mViewer->getCamera()->resetControl(mViewer->getWidth(), mViewer->getHeight());
        }
        ImGui::PopStyleColor();
        ImGui::Spacing();

        ImGui::TextWrapped("Move Speed");
        float movementSpeed = mViewer->getMovementSpeed();
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::SliderFloat("##Move Speed", &movementSpeed, 0.5f, 30.0f)) {
            mViewer->setMovementSpeed(movementSpeed);
        }
        ImGui::PopItemWidth();
        ImGui::Spacing();

        ImGui::TextWrapped("Mouse Sensitivity");
        float mouseSensitivity = mViewer->getMouseSensitivity();
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::SliderFloat("##Mouse Sensitivity", &mouseSensitivity, 0.01f, 0.50f)) {
            mViewer->setMouseSensitivity(mouseSensitivity);
        }
        ImGui::PopItemWidth();
        ImGui::Spacing();

        ImGui::TextWrapped("Camera Position");
        glm::vec3 position = mViewer->getCamera()->getPosition();
        float pos[3] = { position.x, position.y, position.z };
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 60);
        ImGui::Text("X:"); ImGui::SameLine();
        if (ImGui::InputFloat("##Camera Position X", &pos[0])) {
            mViewer->getCamera()->setPosition(glm::vec3(pos[0], pos[1], pos[2]));
        }
        ImGui::Text("Y:"); ImGui::SameLine();
        if (ImGui::InputFloat("##Camera Position Y", &pos[1])) {
            mViewer->getCamera()->setPosition(glm::vec3(pos[0], pos[1], pos[2]));
        }
        ImGui::Text("Z:"); ImGui::SameLine();
        if (ImGui::InputFloat("##Camera Position Z", &pos[2])) {
            mViewer->getCamera()->setPosition(glm::vec3(pos[0], pos[1], pos[2]));
        }
        ImGui::PopItemWidth();

        ImGui::TextWrapped("Camera Rotation");
        float rotation[2] = { mViewer->getCamera()->getYaw(), mViewer->getCamera()->getPitch()};
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 60);
        ImGui::Text("Yaw:"); ImGui::SameLine();
        if (ImGui::InputFloat("##Camera Yaw", &rotation[0])) {
            mViewer->getCamera()->setYaw(rotation[0]);
        }
        ImGui::Text("Pitch:"); ImGui::SameLine();
        if (ImGui::InputFloat("##Camera Pitch", &rotation[1])) {
            mViewer->getCamera()->setPitch(rotation[1]);
        }
        ImGui::PopItemWidth();

        if(mViewer->getCamera()->getType() == CameraType::PERSPECTIVE)
        {
            ImGui::TextWrapped("Camera FOV");
            float fov = mViewer->getCamera()->getFov();
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::SliderFloat("##Camera FOV", &fov, 1.0f, 180.0f)) {
                mViewer->getCamera()->setFov(fov);
            }
            ImGui::PopItemWidth();
        }

        ImGui::End();
    };
};
#endif //TOY_RENDERER_UPDATE_CAMERAUI_H
