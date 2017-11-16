// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <unordered_map>
#include "common/logging/log.h"
#include "core/frontend/camera/blank_camera.h"
#include "core/frontend/camera/factory.h"
#include "core/settings.h"

namespace Camera {

static std::unordered_map<std::string, std::unique_ptr<CameraFactory>> factories;

CameraFactory::~CameraFactory() = default;

void RegisterFactory(const std::string& name, std::unique_ptr<CameraFactory> factory) {
    factories[name] = std::move(factory);
}

std::unique_ptr<CameraInterface> CreateCamera(int camera_id) {
    const std::string camera_name = Settings::values.camera_name[camera_id];
    auto pair = factories.find(camera_name);
    if (pair != factories.end()) {
        return pair->second->Create(camera_id);
    }

    if (camera_name != "blank") {
        LOG_ERROR(Service_CAM, "Unknown camera \"%s\"", camera_name.c_str());
    }
    return std::make_unique<BlankCamera>();
}

} // namespace Camera
