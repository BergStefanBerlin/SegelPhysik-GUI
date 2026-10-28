#pragma once

#include "force_display.hpp"

#include <QByteArray>
#include <QDomDocument>
#include <QHash>
#include <QPoint>
#include <QWidget>
#include <array>
#include <optional>

class QSvgRenderer;
class QLabel;
class QSizeGrip;

namespace navigation {

struct Values {
    std::optional<double> heading;
    std::optional<double> roll;
    std::optional<double> pitch;
    std::optional<double> cameraAzimuth;
    std::optional<double> cameraRoll;
    std::optional<double> cameraPitch;
    std::optional<forcedisplay::Vec3> velocityWorld;
    std::optional<double> speed;
};

Values derive(const std::optional<forcedisplay::Pose>& pose,
    const forcedisplay::Vec3& cameraOffset,
    const std::optional<forcedisplay::Vec3>& velocityWorld = std::nullopt);
QString compassDirection(double degrees);

struct CameraPreset {
    const char* id;
    const char* label;
    forcedisplay::Vec3 direction;
    forcedisplay::Vec3 up;
};
const std::array<CameraPreset, 14>& cameraPresets();

class Instrument final : public QWidget {
public:
    enum class Kind { Compass, Roll, Pitch };
    Instrument(Kind kind, QWidget* parent);
    void setValues(const Values& values);
    const QByteArray& renderedSvg() const { return rendered_; }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void text(const QString& id, const QString& value);
    void rotation(const QString& id, std::optional<double> angle);
    Kind kind_;
    QDomDocument document_;
    QHash<QString, QDomElement> elements_;
    QDomElement errorDisk_;
    QDomElement dangerRing_;
    QSvgRenderer* renderer_;
    QByteArray rendered_;
};

class Overlay final : public QWidget {
public:
    explicit Overlay(QWidget* viewport);
    void setValues(const Values& values);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void fitToViewport();
    void clampPosition();
    QLabel* handle_;
    QSizeGrip* sizeGrip_;
    std::array<Instrument*, 3> instruments_;
    QLabel* speedMagnitude_;
    std::array<QLabel*, 3> speedComponents_;
    bool dragging_ = false;
    QPoint dragOffset_;
    bool resizing_ = false;
    QPoint resizeStart_;
    QSize resizeSize_;
    QSize preferredSize_{180, 670};
};

} // namespace navigation
