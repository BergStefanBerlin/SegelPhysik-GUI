#include "navigation.hpp"

#include <QApplication>
#include <QLabel>
#include <QFile>
#include <QDir>
#include <QPainter>
#include <QSvgRenderer>
#include <QMouseEvent>
#include <QSizeGrip>
#include <QSet>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
constexpr double pi = 3.14159265358979323846;
void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}
void near(std::optional<double> actual, double expected)
{
    require(actual && std::abs(*actual - expected) < 1e-8, "navigation angle/sign mismatch");
}
forcedisplay::Pose rotation(int axis, double degrees)
{
    forcedisplay::Pose pose;
    const double half = degrees * pi / 360.0;
    pose.quaternionXyzw[axis] = std::sin(half);
    pose.quaternionXyzw[3] = std::cos(half);
    return pose;
}
QDomElement element(const navigation::Instrument& instrument, const QString& id)
{
    QDomDocument document;
    require(static_cast<bool>(document.setContent(instrument.renderedSvg())), "invalid runtime SVG");
    QList<QDomElement> pending{document.documentElement()};
    while (!pending.isEmpty()) {
        const auto candidate = pending.takeLast();
        if (candidate.attribute(QStringLiteral("id")) == id) return candidate;
        for (auto child = candidate.firstChildElement(); !child.isNull(); child = child.nextSiblingElement())
            pending.append(child);
    }
    throw std::runtime_error(("missing runtime SVG ID: " + id).toStdString());
}
QDomElement element(const navigation::Instrument& instrument, const char* id)
{
    return element(instrument, QString::fromLatin1(id));
}
void testMath()
{
    using navigation::derive;
    const auto identity = derive(forcedisplay::Pose{}, {1, -1, 1});
    near(identity.heading, 0);
    near(identity.roll, 0);
    near(identity.pitch, 0);
    near(identity.cameraAzimuth, 45);
    near(identity.cameraRoll, 45);
    near(identity.cameraPitch, 45);
    near(derive(rotation(2, -90), {1, 0, 0}).heading, 90);
    near(derive(rotation(2, 90), {1, 0, 0}).heading, 270);
    near(derive(rotation(0, 30), {1, 0, 0}).roll, -30);
    near(derive(rotation(1, -30), {1, 0, 0}).pitch, 30);
    near(derive(rotation(0, 150), {1, 0, 0}).roll, -150);
    auto vertical = derive(rotation(1, -90), {0, 0, 1});
    require(!vertical.heading && !vertical.roll, "vertical heading/roll must be undefined");
    near(vertical.pitch, 90);
    auto top = derive(forcedisplay::Pose{}, {0, 0, 1});
    require(!top.cameraAzimuth, "vertical camera has no azimuth");
    near(top.cameraRoll, 90);
    near(top.cameraPitch, 90);
    auto bottom = derive(forcedisplay::Pose{}, {0, 0, -1});
    near(bottom.cameraPitch, -90);
    near(bottom.cameraRoll, -90);
    require(!derive(forcedisplay::Pose{}, {1, 0, 0}).cameraRoll, "edge-on roll projection");
    auto absent = derive(std::nullopt, {1, 0, 0});
    require(!absent.heading && !absent.roll && !absent.pitch && !absent.cameraRoll,
        "absent snapshot must not become zero");
    auto invalidPose = forcedisplay::Pose{};
    invalidPose.quaternionXyzw = {0, 0, 0, 0};
    require(!derive(invalidPose, {1, 0, 0}).pitch, "zero quaternion must be invalid");
    invalidPose.quaternionXyzw[0] = std::numeric_limits<double>::quiet_NaN();
    require(!derive(invalidPose, {1, 0, 0}).pitch, "nonfinite quaternion must be invalid");
    const auto invalidCamera = derive(forcedisplay::Pose{},
        {std::numeric_limits<double>::infinity(), 0, 1});
    near(invalidCamera.heading, 0);
    require(!invalidCamera.cameraAzimuth && !invalidCamera.cameraRoll, "invalid camera marker");
    const auto zeroCamera = derive(forcedisplay::Pose{}, {0, 0, 0});
    require(!zeroCamera.cameraAzimuth && !zeroCamera.cameraPitch, "zero camera projection");
    require(navigation::compassDirection(359) == QStringLiteral("N"), "north wrap");
    require(navigation::compassDirection(-90) == QStringLiteral("W"), "negative wrap");
    require(navigation::compassDirection(11.25) == QStringLiteral("NNO"), "sector boundary");
    require(navigation::compassDirection(90) == QStringLiteral("O"), "east sign");
    require(navigation::compassDirection(180) == QStringLiteral("S"), "south sign");
    QSet<QString> ids;
    for (const auto& preset : navigation::cameraPresets()) {
        ids.insert(QString::fromLatin1(preset.id));
        require(forcedisplay::norm(forcedisplay::cross(preset.direction, preset.up)) > 0,
            "degenerate camera preset");
    }
    require(ids.size() == 14, "14 unique legacy presets required");
}
void testInstruments()
{
    using namespace navigation;
    Instrument compass(Instrument::Kind::Compass, nullptr);
    Instrument roll(Instrument::Kind::Roll, nullptr);
    Instrument pitch(Instrument::Kind::Pitch, nullptr);
    for (auto* instrument : {&compass, &roll, &pitch}) {
        require(element(*instrument, "runtime_error_disk").attribute(QStringLiteral("display"))
            == QStringLiteral("inline"), "missing snapshot error disk");
        require(instrument->renderedSvg().contains("FEHLER"), "missing snapshot error text");
    }
    Values values{90, -30, 20, 45, 50, 60};
    compass.setValues(values);
    roll.setValues(values);
    pitch.setValues(values);
    require(element(compass, "heading_value").text() == QStringLiteral("090°"), "live heading text");
    require(element(compass, "heading_direction").text() == QStringLiteral("O"), "live heading direction");
    require(element(roll, "roll_value").text() == QStringLiteral("-30°"), "live roll text");
    require(element(pitch, "pitch_value").text() == QStringLiteral("+20°"), "live pitch text");
    require(element(roll, "roll_pose").attribute(QStringLiteral("transform"))
        == QStringLiteral("rotate(30 312 405)"), "roll SVG sign");
    require(element(pitch, "pitch_camera").attribute(QStringLiteral("transform"))
        == QStringLiteral("rotate(30 312 405)"), "camera SVG sign");
    values.roll = 90;
    roll.setValues(values);
    require(element(roll, "runtime_danger_ring").attribute(QStringLiteral("display"))
        == QStringLiteral("none"), "90 degrees must not show danger");
    values.roll = -90.01;
    roll.setValues(values);
    require(element(roll, "runtime_danger_ring").attribute(QStringLiteral("display"))
        == QStringLiteral("inline"), "capsize danger ring absent");
    require(element(roll, "runtime_error_disk").attribute(QStringLiteral("display"))
        == QStringLiteral("none"), "capsize is valid, not missing data");
    values.cameraAzimuth = std::numeric_limits<double>::quiet_NaN();
    compass.setValues(values);
    require(element(compass, "camera_symbol").attribute(QStringLiteral("display"))
        == QStringLiteral("none"), "nonfinite camera must hide marker");
    require(element(compass, "heading_value").text() == QStringLiteral("090°"), "camera invalidates heading");
    values.pitch.reset();
    pitch.setValues(values);
    require(element(pitch, "pitch_value").text() == QStringLiteral("FEHLER"), "invalid pitch text");
    require(element(pitch, "pitch_pose").attribute(QStringLiteral("display"))
        == QStringLiteral("none"), "invalid pose must hide");
    values.heading = 359.9;
    compass.setValues(values);
    require(element(compass, "heading_value").text() == QStringLiteral("000°"), "rounded north");
    QSvgRenderer raster(compass.renderedSvg());
    const auto textBounds = raster.boundsOnElement(QStringLiteral("heading_value"));
    require(textBounds.width() >= 70 && textBounds.height() >= 40,
        "primary numeric value must retain readable SVG font size");
    compass.resize(200, 200);
    require(!compass.grab().isNull(), "SVG rasterization");
    const auto evidence = qEnvironmentVariable("SEGELPHYSIK_NAVIGATION_EVIDENCE");
    if (!evidence.isEmpty()) {
        require(QDir().mkpath(evidence), "navigation evidence directory");
        compass.resize(580, 580);
        QFile svg(QDir(evidence).filePath(QStringLiteral("compass_runtime.svg")));
        require(svg.open(QIODevice::WriteOnly), "navigation evidence SVG");
        require(svg.write(compass.renderedSvg()) == compass.renderedSvg().size(), "navigation evidence write");
        require(compass.grab().save(QDir(evidence).filePath(QStringLiteral("compass_runtime.bmp")), "BMP"),
            "navigation evidence raster");
        QSvgRenderer source(QStringLiteral(":/navigation/compass.svg"));
        QSvgRenderer runtime(compass.renderedSvg());
        std::cout << "Source text bounds: " << source.boundsOnElement(QStringLiteral("heading_value")).width()
            << " x " << source.boundsOnElement(QStringLiteral("heading_value")).height()
            << "; runtime: " << runtime.boundsOnElement(QStringLiteral("heading_value")).width()
            << " x " << runtime.boundsOnElement(QStringLiteral("heading_value")).height() << '\n';
        QImage original(580, 580, QImage::Format_ARGB32_Premultiplied);
        original.fill(Qt::transparent);
        { QPainter painter(&original); source.render(&painter); }
        require(original.save(QDir(evidence).filePath(QStringLiteral("compass_source.bmp")), "BMP"),
            "source evidence raster");
    }
}
void testOverlay()
{
    QWidget viewport;
    viewport.resize(1000, 800);
    navigation::Overlay overlay(&viewport);
    viewport.show();
    QApplication::processEvents();
    auto* handle = overlay.findChild<QLabel*>(QStringLiteral("navigationDragHandle"));
    auto* compass = overlay.findChild<QWidget*>(QStringLiteral("navigation_compass"));
    auto* roll = overlay.findChild<QWidget*>(QStringLiteral("navigation_roll"));
    auto* pitch = overlay.findChild<QWidget*>(QStringLiteral("navigation_pitch"));
    require(handle && compass && roll && pitch, "three instruments/header required");
    require(compass->y() < roll->y() && roll->y() < pitch->y(), "vertical instrument order");
    const auto start = overlay.pos();
    const QPointF local(20, 10);
    const auto global = handle->mapToGlobal(local.toPoint());
    QMouseEvent press(QEvent::MouseButtonPress, local, global,
        Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(handle, &press);
    QMouseEvent move(QEvent::MouseMove, local, global + QPoint(300, 100),
        Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(handle, &move);
    QMouseEvent release(QEvent::MouseButtonRelease, local, global + QPoint(300, 100),
        Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(handle, &release);
    require(overlay.pos() == start + QPoint(300, 100), "overlay drag");
    overlay.move(8, 8);
    auto* grip = overlay.findChild<QSizeGrip*>(QStringLiteral("navigationSizeGrip"));
    require(grip && !overlay.isWindow(), "embedded resize grip required");
    const auto previousSize = overlay.size();
    const auto viewportSize = viewport.size();
    const QPointF gripLocal(4, 4);
    const auto gripGlobal = grip->mapToGlobal(gripLocal.toPoint());
    QMouseEvent resizePress(QEvent::MouseButtonPress, gripLocal, gripGlobal,
        Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(grip, &resizePress);
    QMouseEvent resizeMove(QEvent::MouseMove, gripLocal, gripGlobal + QPoint(100, 100),
        Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(grip, &resizeMove);
    QMouseEvent resizeRelease(QEvent::MouseButtonRelease, gripLocal, gripGlobal + QPoint(100, 100),
        Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(grip, &resizeRelease);
    require(overlay.size() == previousSize + QSize(100, 100), "overlay grip must enlarge overlay");
    require(viewport.size() == viewportSize, "overlay grip must not resize main viewport/window");
    require(overlay.pos() == QPoint(8, 8), "resizing must preserve overlay position");
    viewport.resize(1200, 1000);
    QApplication::processEvents();
    require(overlay.size() == previousSize + QSize(100, 100), "viewport growth resets user size");
    viewport.resize(240, 300);
    QApplication::processEvents();
    require(viewport.rect().contains(overlay.geometry()), "overlay must stay within resized viewport");
    viewport.resize(1200, 1000);
    QApplication::processEvents();
    require(overlay.size() == previousSize + QSize(100, 100), "temporary viewport shrink loses preferred size");
    QWidget smallViewport;
    smallViewport.resize(100, 30);
    navigation::Overlay initiallySmall(&smallViewport);
    smallViewport.show();
    smallViewport.resize(1000, 800);
    QApplication::processEvents();
    require(initiallySmall.size() == QSize(180, 564), "startup viewport size must not permanently shrink overlay");
}
}

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    try {
        testMath();
        testInstruments();
        testOverlay();
        std::cout << "Navigation: math, SVG bindings, singularities and draggable/resizable overlay passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
