#pragma once

#include "graphics_settings.hpp"
#include <functional>

class QWidget;

QWidget* createGraphicsSettingsPage(QWidget* parent, const graphics::Settings& initial,
    const QString& status, const std::function<bool(const graphics::Settings&)>& commit,
    const std::function<void()>& close,
    const std::function<QString(graphics::Feature)>& featureStatus = {},
    bool staticMode = true);
