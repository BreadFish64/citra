// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "citra_qt/camera/still_image_camera.h"
#include "common/math_util.h"
#include "core/settings.h"

namespace Camera {

StillImageCamera::StillImageCamera(int camera_id_) : camera_id(camera_id_) {}

void StillImageCamera::StartCapture(){};

void StillImageCamera::StopCapture(){};

void StillImageCamera::SetFormat(Service::CAM::OutputFormat output_format) {
    output_rgb = output_format == Service::CAM::OutputFormat::RGB565;
}

void StillImageCamera::SetResolution(const Service::CAM::Resolution& resolution) {
    width = resolution.width;
    height = resolution.height;
};

void StillImageCamera::SetFlip(Service::CAM::Flip flip) {
    using namespace Service::CAM;
    flip_horizontal = (flip == Flip::Horizontal) || (flip == Flip::Reverse);
    flip_vertical = (flip == Flip::Vertical) || (flip == Flip::Reverse);
}

void StillImageCamera::SetEffect(Service::CAM::Effect effect) {
    if (effect != Service::CAM::Effect::None) {
        LOG_ERROR(Service_CAM, "Unimplemented effect %d", static_cast<int>(effect));
    }
}

std::vector<u16> StillImageCamera::ReceiveFrame() const {
    const std::string camera_config = Settings::values.camera_config[camera_id];
    QImage image(QString::fromStdString(camera_config));
    if (!image.isNull()) {
        std::vector<u16> buffer(width * height);
        QImage scaled =
            image.scaled(width, height, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        QImage transformed =
            scaled.copy((scaled.width() - width) / 2, (scaled.height() - height) / 2, width, height)
                .mirrored(flip_horizontal, flip_vertical);
        if (output_rgb) {
            QImage converted = transformed.convertToFormat(QImage::Format_RGB16);
            std::memcpy(buffer.data(), converted.bits(), width * height * sizeof(u16));
        } else {
            auto dest = buffer.begin();
            bool write = false;
            int py, pu, pv;
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    QRgb rgb = transformed.pixel(x, y);
                    int r = qRed(rgb), g = qGreen(rgb), b = qBlue(rgb);

                    // The following transformation is a reverse of the one in Y2R using ITU_Rec601
                    r = (r << 5) - 0x18 + 0x166F;
                    g = (g << 5) - 0x18 - 0x10EE;
                    b = (b << 5) - 0x18 + 0x1C5B;
                    int y, u, v;
                    y = 0.00933073 * r + 0.0183538 * g + 0.00356543 * b;
                    u = -0.00527299 * r - 0.0103722 * g + 0.0156451 * b;
                    v = 0.0156741 * r - 0.0131245 * g - 0.00254958 * b;

                    if (write) {
                        pu = (pu + u) / 2;
                        pv = (pv + v) / 2;
                        using MathUtil::Clamp;
                        *(dest++) = (u16)Clamp(py, 0, 0xFF) | ((u16)Clamp(pu, 0, 0xFF) << 8);
                        *(dest++) = (u16)Clamp(y, 0, 0xFF) | ((u16)Clamp(pv, 0, 0xFF) << 8);
                    } else {
                        py = y;
                        pu = u;
                        pv = v;
                    }
                   write = !write;
                }
            }
        }
        return buffer;
    } else {
        LOG_ERROR(Service_CAM, "Couldn't load image \"%s\"", camera_config.c_str());
        return std::vector<u16>(width * height, output_rgb ? 0 : 0x8000);
}

std::unique_ptr<CameraInterface> StillImageCameraFactory::Create(int _camera_id) const {
    return std::make_unique<StillImageCamera>(_camera_id);
}

} // namespace Camera
