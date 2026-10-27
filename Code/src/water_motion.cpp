#include "water_motion.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace watermotion {
using namespace forcedisplay;
namespace {
Vec3 interpolate(const Vec3& a, const Vec3& b, double t)
{
    return add(a, scale(sub(b, a), t));
}
}

std::optional<Contact> contactFromContour(const QVector<Vec3>& contour, const Vec3& velocity)
{
    const double speed = std::hypot(velocity[0], velocity[1]);
    if (speed <= 1e-10 || contour.size() < 2) return std::nullopt;
    const Vec3 direction{velocity[0] / speed, velocity[1] / speed, 0.0};
    const Vec3 side{-direction[1], direction[0], 0.0};
    auto leading = contour.front();
    auto trailing = leading;
    double low = dot(leading, side), high = low;
    for (const auto& p : contour) {
        if (dot(p, direction) > dot(leading, direction)) leading = p;
        if (dot(p, direction) < dot(trailing, direction)) trailing = p;
        low = std::min(low, dot(p, side));
        high = std::max(high, dot(p, side));
    }
    if (high - low <= 1e-8 || norm(sub(leading, trailing)) <= 1e-8) return std::nullopt;
    leading[2] = trailing[2] = 0.0;
    return Contact{leading, trailing, high - low};
}

void History::clear()
{
    points_.clear();
    previous_.reset();
    distance_ = nextSample_ = 0.0;
    diagnostic_.clear();
}

void History::append(const Input& input, const graphics::Profile& profile)
{
    if (!std::isfinite(input.time) || !std::isfinite(profile.trackAge)
        || profile.trackAge <= 0 || !std::isfinite(profile.trackSpacing)
        || profile.trackSpacing <= 0 || profile.trackBudget < 2) {
        throw std::invalid_argument("Ungueltige Fahrthistorienparameter.");
    }
    for (const auto& v : {input.center, input.reference, input.velocity}) {
        for (double x : v) if (!std::isfinite(x)) throw std::invalid_argument("Nicht endliche Fahrtdaten.");
    }
    if (previous_ && input.time < previous_->time) clear();
    if (previous_ && input.time == previous_->time) return;
    bool broken = !previous_;
    if (previous_ && norm(sub(input.reference, previous_->reference)) > 1e-9) {
        broken = true;
        diagnostic_ = QStringLiteral("Schwerpunktbezug geaendert: Fahrspursegment unterbrochen.");
    }
    if (broken) {
        distance_ = 0.0;
        nextSample_ = profile.trackSpacing;
        const double speed = std::hypot(input.velocity[0], input.velocity[1]);
        const Vec3 direction = speed > 1e-10
            ? Vec3{input.velocity[0] / speed, input.velocity[1] / speed, 0.0} : Vec3{};
        points_.push_back({input.time, 0.0, {input.center[0], input.center[1], 0.0},
            direction, input.contact, false, speed});
    } else {
        Vec3 delta = sub(input.center, previous_->center);
        delta[2] = 0.0;
        const double length = std::hypot(delta[0], delta[1]);
        if (!std::isfinite(length)) throw std::overflow_error("Fahrweg nicht darstellbar.");
        if (length > 1e-10) {
            const auto direction = scale(delta, 1.0 / length);
            const double end = distance_ + length;
            if (!std::isfinite(end) || !std::isfinite((end - nextSample_) / profile.trackSpacing))
                throw std::overflow_error("Fahrweg oder Abtastung nicht darstellbar.");
            // Skip samples already guaranteed to be evicted by the point budget.
            if (end - nextSample_ > profile.trackSpacing * (profile.trackBudget - 1)) {
                const double skip = std::floor((end - nextSample_) / profile.trackSpacing)
                    - (profile.trackBudget - 1);
                nextSample_ += skip * profile.trackSpacing;
            }
            while (nextSample_ <= end) {
                const double t = std::clamp((nextSample_ - distance_) / length, 0.0, 1.0);
                auto p = interpolate(previous_->center, input.center, t);
                p[2] = 0.0;
                std::optional<Contact> contact;
                if (previous_->contact && input.contact) {
                    contact = Contact{
                        interpolate(previous_->contact->leading, input.contact->leading, t),
                        interpolate(previous_->contact->trailing, input.contact->trailing, t),
                        previous_->contact->width * (1.0 - t) + input.contact->width * t};
                }
                points_.push_back({previous_->time + t * (input.time - previous_->time),
                    nextSample_, p, direction, contact, true,
                    std::hypot(input.velocity[0], input.velocity[1])});
                const double next = nextSample_ + profile.trackSpacing;
                if (!std::isfinite(next) || next <= nextSample_)
                    throw std::overflow_error("Fahrwegabtastung ohne numerischen Fortschritt.");
                nextSample_ = next;
            }
            distance_ = end;
        }
    }
    previous_ = input;
    while (!points_.empty() && (input.time - points_.front().time > profile.trackAge
        || points_.size() > static_cast<std::size_t>(profile.trackBudget))) {
        points_.pop_front();
    }
    if (!points_.empty()) points_.front().connected = false;
}
}
