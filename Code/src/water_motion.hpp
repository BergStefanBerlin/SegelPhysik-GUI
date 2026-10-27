#pragma once

#include "force_display.hpp"
#include "graphics_settings.hpp"

#include <deque>
#include <optional>

namespace watermotion {
using forcedisplay::Vec3;
struct Contact {
    Vec3 leading{};
    Vec3 trailing{};
    double width = 0.0;
};
struct Input {
    double time = 0.0;
    Vec3 center{};
    Vec3 reference{};
    Vec3 velocity{};
    std::optional<Contact> contact;
};
struct Point {
    double time = 0.0;
    double distance = 0.0;
    Vec3 center{};
    Vec3 direction{};
    std::optional<Contact> contact;
    bool connected = false;
    double speed = 0.0;
};
std::optional<Contact> contactFromContour(const QVector<Vec3>& contour, const Vec3& velocity);
class History {
public:
    void clear();
    void append(const Input& input, const graphics::Profile& profile);
    const std::deque<Point>& points() const { return points_; }
    const QString& diagnostic() const { return diagnostic_; }
private:
    std::deque<Point> points_;
    std::optional<Input> previous_;
    double distance_ = 0.0;
    double nextSample_ = 0.0;
    QString diagnostic_;
};
}
