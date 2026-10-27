#include "navigation.hpp"

#include <QFile>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QSizeGrip>
#include <QSvgRenderer>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace navigation {
namespace {
constexpr double pi = 3.14159265358979323846;
constexpr double epsilon = 1e-10;
double degrees(double radians) { return radians * 180.0 / pi; }
double wrap(double angle)
{
    const double result = std::fmod(angle, 360.0);
    return result < 0 ? result + 360.0 : result;
}
void collectElements(const QDomElement& element, QHash<QString, QDomElement>& result)
{
    if (element.hasAttribute(QStringLiteral("id")))
        result.insert(element.attribute(QStringLiteral("id")), element);
    for (auto child = element.firstChildElement(); !child.isNull(); child = child.nextSiblingElement())
        collectElements(child, result);
}
QString angleText(double angle)
{
    const int rounded = static_cast<int>(std::round(angle));
    return QStringLiteral("%1%2°").arg(rounded > 0 ? QStringLiteral("+") : QString()).arg(rounded);
}
}

Values derive(const std::optional<forcedisplay::Pose>& pose,
    const forcedisplay::Vec3& cameraOffset)
{
    using namespace forcedisplay;
    Values result;
    const bool validPose = pose && std::all_of(
        pose->quaternionXyzw.begin(), pose->quaternionXyzw.end(),
        [](double value) { return std::isfinite(value); });
    double normSquared = 0;
    if (validPose)
        for (double value : pose->quaternionXyzw) normSquared += value * value;
    if (validPose && std::abs(normSquared - 1.0) < 1e-6) {
        const auto forward = toWorldVector(*pose, {1, 0, 0});
        const auto port = toWorldVector(*pose, {0, 1, 0});
        const auto up = toWorldVector(*pose, {0, 0, 1});
        const double horizontal = std::hypot(forward[0], forward[1]);
        result.pitch = degrees(std::atan2(forward[2], horizontal));
        if (horizontal > epsilon) {
            result.heading = wrap(degrees(std::atan2(-forward[1], forward[0])));
            result.roll = degrees(std::atan2(-port[2], up[2]));
        }
    }
    if (!std::all_of(cameraOffset.begin(), cameraOffset.end(),
            [](double v) { return std::isfinite(v); }))
        return result;
    const double horizontal = std::hypot(cameraOffset[0], cameraOffset[1]);
    if (horizontal > epsilon)
        result.cameraAzimuth = wrap(degrees(std::atan2(-cameraOffset[1], cameraOffset[0])));
    if (result.heading) {
        const double heading = *result.heading * pi / 180.0;
        const Vec3 forward{std::cos(heading), -std::sin(heading), 0};
        const auto starboard = cross(forward, {0, 0, 1});
        const double pitchX = dot(cameraOffset, forward);
        const double rollX = dot(cameraOffset, starboard);
        if (std::hypot(pitchX, cameraOffset[2]) > epsilon)
            result.cameraPitch = degrees(std::atan2(cameraOffset[2], pitchX));
        if (std::hypot(rollX, cameraOffset[2]) > epsilon)
            result.cameraRoll = degrees(std::atan2(cameraOffset[2], rollX));
    }
    return result;
}

QString compassDirection(double angle)
{
    if (!std::isfinite(angle)) return {};
    static const std::array<const char*, 16> directions{
        "N", "NNO", "NO", "ONO", "O", "OSO", "SO", "SSO",
        "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"};
    return QString::fromLatin1(directions[static_cast<std::size_t>(
        static_cast<int>(std::floor((wrap(angle) + 11.25) / 22.5)) % 16)]);
}

const std::array<CameraPreset, 14>& cameraPresets()
{
    static const std::array<CameraPreset, 14> presets{{
        {"oben_backbord_bug", "Oben Backbord Bug", {1.7, 1.7, 1.5}, {0, 0, 1}},
        {"oben_backbord_heck", "Oben Backbord Heck", {-1.7, 1.7, 1.5}, {0, 0, 1}},
        {"oben_steuerbord_bug", "Oben Steuerbord Bug", {1.7, -1.7, 1.5}, {0, 0, 1}},
        {"oben_steuerbord_heck", "Oben Steuerbord Heck", {-1.7, -1.7, 1.5}, {0, 0, 1}},
        {"unten_backbord_bug", "Unten Backbord Bug", {1.7, 1.7, -1.5}, {0, 0, 1}},
        {"unten_backbord_heck", "Unten Backbord Heck", {-1.7, 1.7, -1.5}, {0, 0, 1}},
        {"unten_steuerbord_bug", "Unten Steuerbord Bug", {1.7, -1.7, -1.5}, {0, 0, 1}},
        {"unten_steuerbord_heck", "Unten Steuerbord Heck", {-1.7, -1.7, -1.5}, {0, 0, 1}},
        {"backbord", "Backbord (Seite)", {0, 1, 0}, {0, 0, 1}},
        {"steuerbord", "Steuerbord (Seite)", {0, -1, 0}, {0, 0, 1}},
        {"bug", "Bug", {1, 0, 0}, {0, 0, 1}},
        {"heck", "Heck", {-1, 0, 0}, {0, 0, 1}},
        {"oben", "Oben", {0, 0, 1}, {0, 1, 0}},
        {"unten", "Unten", {0, 0, -1}, {0, 1, 0}}
    }};
    return presets;
}

Instrument::Instrument(Kind kind, QWidget* parent)
    : QWidget(parent), kind_(kind), renderer_(new QSvgRenderer(this))
{
    const QString fileName = kind == Kind::Compass ? QStringLiteral("compass")
        : kind == Kind::Roll ? QStringLiteral("roll") : QStringLiteral("pitch");
    setObjectName(QStringLiteral("navigation_") + fileName);
    setToolTip(kind == Kind::Compass
        ? QStringLiteral("Bugrichtung: +X = lokales Nord, +Y = West; im Uhrzeigersinn positiv.\n"
            "Gelbe Kamera: tatsächliche Blickposition um das Kameraziel.")
        : kind == Kind::Roll
        ? QStringLiteral("Krängung aus dem gewählten Ergebniszustand; positiv: Steuerbord steigt.\n"
            "Gelbe Kamera: Projektion der tatsächlichen Blickposition.")
        : QStringLiteral("Stampfen aus dem gewählten Ergebniszustand; positiv: Bug steigt.\n"
            "Gelbe Kamera: Projektion der tatsächlichen Blickposition."));
    QFile file(QStringLiteral(":/navigation/") + fileName + QStringLiteral(".svg"));
    if (!file.open(QIODevice::ReadOnly))
        throw std::runtime_error("Navigations-SVG konnte nicht geladen werden.");
    const auto parsed = document_.setContent(file.readAll());
    if (!parsed) throw std::runtime_error(parsed.errorMessage.toStdString());
    collectElements(document_.documentElement(), elements_);
    // QtSvg does not reliably retain these class-based text sizes.
    for (auto it = elements_.begin(); it != elements_.end(); ++it) {
        const auto cssClass = it->attribute(QStringLiteral("class"));
        if (cssClass == QStringLiteral("dial-value"))
            it->setAttribute(QStringLiteral("style"), QStringLiteral("font-size:48px; font-weight:700"));
        else if (cssClass == QStringLiteral("dial-caption"))
            it->setAttribute(QStringLiteral("style"), QStringLiteral("font-size:17px"));
        else if (cssClass == QStringLiteral("dial-code"))
            it->setAttribute(QStringLiteral("style"), QStringLiteral("font-size:18px; font-weight:700"));
    }
    const QString prefix = kind == Kind::Compass ? QStringLiteral("compass") : fileName;
    const auto face = elements_.value(prefix + QStringLiteral("_face"));
    auto mutableFace = face;
    const QString outer = kind == Kind::Compass ? QStringLiteral("dial_outer_ring")
        : prefix + QStringLiteral("_ring_outer");
    if (face.isNull() || !elements_.contains(outer))
        throw std::runtime_error("Navigations-SVG ohne Zifferblatt.");
    dangerRing_ = document_.createElement(QStringLiteral("path"));
    dangerRing_.setAttribute(QStringLiteral("id"), QStringLiteral("runtime_danger_ring"));
    dangerRing_.setAttribute(QStringLiteral("d"), QStringLiteral(
        "M312 195 A210 210 0 1 1 312 615 A210 210 0 1 1 312 195 Z "
        "M312 238.5 A166.5 166.5 0 1 0 312 571.5 A166.5 166.5 0 1 0 312 238.5 Z"));
    dangerRing_.setAttribute(QStringLiteral("fill-rule"), QStringLiteral("evenodd"));
    dangerRing_.setAttribute(QStringLiteral("fill"), QStringLiteral("#ff0000"));
    mutableFace.insertAfter(dangerRing_, elements_[outer]);
    errorDisk_ = document_.createElement(QStringLiteral("circle"));
    errorDisk_.setAttribute(QStringLiteral("id"), QStringLiteral("runtime_error_disk"));
    errorDisk_.setAttribute(QStringLiteral("cx"), QStringLiteral("312"));
    errorDisk_.setAttribute(QStringLiteral("cy"), QStringLiteral("405"));
    errorDisk_.setAttribute(QStringLiteral("r"), QStringLiteral("165.5"));
    errorDisk_.setAttribute(QStringLiteral("fill"), QStringLiteral("#ff0000"));
    mutableFace.insertAfter(errorDisk_, dangerRing_);
    setValues({});
}

void Instrument::text(const QString& id, const QString& value)
{
    auto it = elements_.find(id);
    if (it == elements_.end()) throw std::runtime_error("Navigations-SVG ohne Textbindung.");
    while (!it->firstChild().isNull()) it->removeChild(it->firstChild());
    it->appendChild(document_.createTextNode(value));
}

void Instrument::rotation(const QString& id, std::optional<double> angle)
{
    if (angle && !std::isfinite(*angle)) angle.reset();
    auto it = elements_.find(id);
    if (it == elements_.end()) throw std::runtime_error("Navigations-SVG ohne Rotationsbindung.");
    // The originals contain CSS example rotations, which override SVG transforms.
    it->removeAttribute(QStringLiteral("style"));
    it->setAttribute(QStringLiteral("display"), angle ? QStringLiteral("inline") : QStringLiteral("none"));
    if (angle) it->setAttribute(QStringLiteral("transform"),
        QStringLiteral("rotate(%1 312 405)").arg(*angle, 0, 'g', 12));
}

void Instrument::setValues(const Values& values)
{
    const auto primary = kind_ == Kind::Compass ? values.heading
        : kind_ == Kind::Roll ? values.roll : values.pitch;
    const bool valid = primary && std::isfinite(*primary)
        && (kind_ == Kind::Compass || std::abs(*primary) <= 180.0);
    errorDisk_.setAttribute(QStringLiteral("display"), valid ? QStringLiteral("none") : QStringLiteral("inline"));
    dangerRing_.setAttribute(QStringLiteral("display"),
        valid && kind_ != Kind::Compass && std::abs(*primary) > 90.0
            ? QStringLiteral("inline") : QStringLiteral("none"));
    if (kind_ == Kind::Compass) {
        text(QStringLiteral("heading_value"), valid
            ? QStringLiteral("%1°").arg(static_cast<int>(std::floor(wrap(*primary) + 0.5)) % 360, 3, 10, QChar('0'))
            : QStringLiteral("FEHLER"));
        text(QStringLiteral("heading_direction"), valid ? compassDirection(*primary) : QString());
        rotation(QStringLiteral("heading_pointer"), valid ? primary : std::nullopt);
        rotation(QStringLiteral("camera_symbol"), values.cameraAzimuth);
    } else {
        const QString prefix = kind_ == Kind::Roll ? QStringLiteral("roll") : QStringLiteral("pitch");
        text(prefix + QStringLiteral("_value"), valid ? angleText(*primary) : QStringLiteral("FEHLER"));
        rotation(prefix + QStringLiteral("_pose"), valid ? std::optional<double>(-*primary) : std::nullopt);
        const auto camera = kind_ == Kind::Roll ? values.cameraRoll : values.cameraPitch;
        rotation(prefix + QStringLiteral("_camera"), camera ? std::optional<double>(90.0 - *camera) : std::nullopt);
        if (kind_ == Kind::Roll) {
            // Current snapshots have no sail-angle contract; the source default is port.
            auto sail = elements_.find(QStringLiteral("roll_sail_side"));
            if (sail == elements_.end()) throw std::runtime_error("Navigations-SVG ohne Segelbindung.");
            sail->setAttribute(QStringLiteral("transform"), QString());
        }
    }
    const auto bytes = document_.toByteArray(-1);
    if (bytes == rendered_) return;
    if (!renderer_->load(bytes)) throw std::runtime_error("Navigations-SVG konnte nicht gerendert werden.");
    rendered_ = bytes;
    update();
}

void Instrument::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const double side = std::min(width(), height());
    renderer_->render(&painter, QRectF((width() - side) / 2.0, (height() - side) / 2.0, side, side));
}

Overlay::Overlay(QWidget* viewport)
    : QWidget(viewport), handle_(new QLabel(QStringLiteral("Navigation  ·  ziehen"), this)),
      sizeGrip_(new QSizeGrip(this))
{
    setObjectName(QStringLiteral("navigationOverlay"));
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->setSizeConstraint(QLayout::SetNoConstraint);
    handle_->setObjectName(QStringLiteral("navigationDragHandle"));
    handle_->setFixedHeight(24);
    handle_->setAlignment(Qt::AlignCenter);
    handle_->setCursor(Qt::SizeAllCursor);
    handle_->setStyleSheet(QStringLiteral("background:#11283a; color:#b8cad5; border-radius:4px;"));
    handle_->installEventFilter(this);
    layout->addWidget(handle_);
    const std::array<Instrument::Kind, 3> kinds{
        Instrument::Kind::Compass, Instrument::Kind::Roll, Instrument::Kind::Pitch};
    for (std::size_t i = 0; i < kinds.size(); ++i) {
        instruments_[i] = new Instrument(kinds[i], this);
        layout->addWidget(instruments_[i], 1);
    }
    sizeGrip_->setObjectName(QStringLiteral("navigationSizeGrip"));
    sizeGrip_->installEventFilter(this);
    sizeGrip_->setToolTip(QStringLiteral("Navigationsanzeigen durch Ziehen vergrößern oder verkleinern"));
    viewport->installEventFilter(this);
    move(8, 8);
    resize(preferredSize_);
    fitToViewport();
    show();
    raise();
}

void Overlay::setValues(const Values& values)
{
    for (auto* instrument : instruments_) instrument->setValues(values);
}

void Overlay::clampPosition()
{
    move(std::clamp(x(), 0, std::max(0, parentWidget()->width() - width())),
        std::clamp(y(), 0, std::max(0, parentWidget()->height() - height())));
}

void Overlay::fitToViewport()
{
    const auto available = parentWidget()->size().expandedTo(QSize(1, 1));
    setMaximumSize(available);
    resize(preferredSize_.boundedTo(available));
    clampPosition();
}

void Overlay::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    const auto gripSize = sizeGrip_->sizeHint();
    sizeGrip_->resize(gripSize);
    sizeGrip_->move(width() - gripSize.width(), height() - gripSize.height());
    sizeGrip_->raise();
    clampPosition();
}

bool Overlay::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == parentWidget() && event->type() == QEvent::Resize) fitToViewport();
    if (watched == sizeGrip_) {
        if (event->type() == QEvent::MouseButtonPress) {
            const auto* mouse = static_cast<QMouseEvent*>(event);
            if (mouse->button() == Qt::LeftButton) {
                resizing_ = true;
                resizeStart_ = mouse->globalPosition().toPoint();
                resizeSize_ = size();
                return true;
            }
        } else if (event->type() == QEvent::MouseMove && resizing_) {
            const auto* mouse = static_cast<QMouseEvent*>(event);
            const auto delta = mouse->globalPosition().toPoint() - resizeStart_;
            const QSize available(parentWidget()->width() - x(), parentWidget()->height() - y());
            const auto minimum = QSize(100, 160).boundedTo(available);
            preferredSize_ = (resizeSize_ + QSize(delta.x(), delta.y())).expandedTo(minimum).boundedTo(available);
            resize(preferredSize_);
            return true;
        } else if (event->type() == QEvent::MouseButtonRelease && resizing_) {
            resizing_ = false;
            return true;
        }
    }
    if (watched == handle_) {
        if (event->type() == QEvent::MouseButtonPress) {
            const auto* mouse = static_cast<QMouseEvent*>(event);
            if (mouse->button() == Qt::LeftButton) {
                dragging_ = true;
                dragOffset_ = mouse->globalPosition().toPoint() - mapToGlobal(QPoint());
                return true;
            }
        } else if (event->type() == QEvent::MouseMove && dragging_) {
            const auto* mouse = static_cast<QMouseEvent*>(event);
            move(parentWidget()->mapFromGlobal(mouse->globalPosition().toPoint() - dragOffset_));
            clampPosition();
            return true;
        } else if (event->type() == QEvent::MouseButtonRelease && dragging_) {
            dragging_ = false;
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}
} // namespace navigation
