#pragma once

#include "graphics_settings.hpp"
#include <functional>

class QWidget;

void editGraphicsSettings(QWidget* parent, const graphics::Settings& initial,
    const QString& status, const std::function<bool(const graphics::Settings&)>& commit,
    const std::function<QString()>& currentStatus = {});
