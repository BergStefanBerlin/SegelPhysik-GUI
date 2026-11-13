#pragma once

#include "force_display.hpp"

#include <QByteArray>
#include <QDomDocument>
#include <QHash>
#include <QPoint>
#include <QWidget>
#include <array>
#include <optional>
#include <vector>

class QSvgRenderer;
class QLabel;
class QSizeGrip;

namespace navigation {

class WindProfileWidget;

struct Values {
    std::optional<double> heading;
    std::optional<double> roll;
    std::optional<double> pitch;
    std::optional<double> cameraAzimuth;
    std::optional<double> cameraRoll;
    std::optional<double> cameraPitch;
    std::optional<forcedisplay::Vec3> velocityWorld;
    std::optional<double> speed;
    std::optional<double> courseRateRadS;
    QString courseRateStatus = QStringLiteral("nicht_verfuegbar");
    bool approximate = false;
    std::optional<double> trueWindDirection;
    std::optional<double> apparentWindDirection;
    std::vector<std::array<double, 2>> windProfile;
    std::optional<std::array<double, 2>> windReference;
    QString windDiagnostic;
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
    void setCardLayout(bool enabled);
    const QByteArray& renderedSvg() const { return rendered_; }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void text(const QString& id, const QString& value);
    void rotation(const QString& id, std::optional<double> angle);
    Kind kind_;
    bool cardLayout_ = false;
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
    void setDocked(bool docked);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void fitToViewport();
    void layoutDocked();
    void clampPosition();
    QLabel* handle_;
    QWidget* speedPanel_ = nullptr;
    QWidget* speedRow_ = nullptr;
    WindProfileWidget* windProfile_ = nullptr;
    QSizeGrip* sizeGrip_;
    std::array<Instrument*, 3> instruments_;
    QLabel* speedMagnitude_;
    QLabel* courseRate_;
    std::array<QLabel*, 3> speedComponents_;
    bool dragging_ = false;
    bool docked_ = false;
    QPoint dragOffset_;
    bool resizing_ = false;
    QPoint resizeStart_;
    QSize resizeSize_;
    QSize preferredSize_{180, 670};
};

} // namespace navigation
