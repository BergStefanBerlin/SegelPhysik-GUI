#pragma once

#include "display_geometry.hpp"
#include "force_display.hpp"
#include "graphics_settings.hpp"
#include "water_motion.hpp"

#include <QHash>
#include <QImage>
#include <QJsonObject>
#include <QString>
#include <QVector>
#include <QWidget>

#include <vtkSmartPointer.h>

#include <array>
#include <functional>
#include <optional>
#include <vector>

class QVTKOpenGLNativeWidget;
class vtkActor;
class vtkAxesActor;
class vtkBillboardTextActor3D;
class vtkCutter;
class vtkClipPolyData;
class vtkGenericOpenGLRenderWindow;
class vtkLight;
class vtkLightKit;
class vtkObject;
class vtkPlane;
class vtkPolyData;
class vtkPolyDataNormals;
class vtkPolyDataTangents;
class vtkTriangleFilter;
class vtkRenderer;
class vtkSkybox;
class vtkTexture;
class vtkTransform;
namespace navigation { class Overlay; }

class VtkSceneWidget final : public QWidget {
public:
    enum class Visibility { Spatial, ThroughBody };

    // Je Rubrik: Gewicht gesamt/Teile, Auftrieb gesamt/Teile, Summe.
    struct VectorSelection {
        bool weightTotal = false;
        bool weightParts = true;
        bool buoyancyTotal = false;
        bool buoyancyParts = true;
        bool sum = true;
    };

    struct DisplayOptions {
        bool forces = true;
        VectorSelection force;
        VectorSelection moment{false, false, false, false, true};
        bool leverArm = true;
        bool lateralCenter = true;
        Visibility visibility = Visibility::ThroughBody;
        forcedisplay::ScaleMode forceMode = forcedisplay::ScaleMode::Logarithmic;
        forcedisplay::ScaleMode momentMode = forcedisplay::ScaleMode::Logarithmic;
    };

    explicit VtkSceneWidget(QWidget* parent = nullptr);
    ~VtkSceneWidget() override;

    bool setModel(const QVector<DisplayPart>& parts, QString* error);
    // Gruppenzuordnung aller Elemente; ein Teil gilt als geflutet, sobald ein
    // Element seiner Gruppe vollgelaufen ist.
    void setElementGroups(const QHash<QString, QString>& groups) { elementGroups_ = groups; }
    void clearModel();
    bool setState(const QJsonObject& state, const forcedisplay::ForceState& forces,
        QString* error);
    void clearState();
    void frameStates(const QVector<QJsonObject>& states);
    bool appendMotionStates(const QVector<QJsonObject>& states, QString* error);
    void resetMotionHistory();
    void invalidateMotionHistory(qsizetype processed, const QString& diagnostic);
    qsizetype motionStateCount() const { return motionStateCount_; }
    const watermotion::History& motionHistory() const { return motionHistory_; }
    struct MotionGeometry {
        std::size_t supports = 0;
        long long trackLines = 0;
        long long wakeLines = 0;
    };
    MotionGeometry motionGeometry() const;

    void setOptions(const DisplayOptions& options);
    const DisplayOptions& options() const { return options_; }
    void setGraphicsProfile(const graphics::Profile& profile);
    // Explizite GUI-Vorschau: 0 Original, 1 Lack, 2 Stoff, 3 Holz, 4 gebürstetes Metall.
    bool setMaterialPreview(const QString& elementId, int style);
    QString graphicsStatus() const;
    double renderNow();
    QImage captureImage();
    double lastRenderDurationMs() const { return lastRenderDurationMs_; }
    struct CameraView {
        forcedisplay::Vec3 position{};
        forcedisplay::Vec3 focalPoint{};
        forcedisplay::Vec3 viewUp{0.0, 0.0, 1.0};
    };
    CameraView cameraView() const;
    bool setCameraView(const CameraView& view);
    bool setCameraPreset(const QString& id, QString* error);
    void setNavigationVisible(bool visible);

    // Referenzen <= 0 gelten als nicht gesetzt; R_M wird dann aus dem ersten
    // gültigen Zustand bestimmt (A-G-00011).
    void setReferences(double forceReference, double momentReference);
    double forceReference() const { return forceReference_; }
    double momentReference() const { return momentReference_; }
    bool refitReferences();
    std::optional<forcedisplay::ScaledMagnitude> scaleInfo(const QString& id) const;

    void setSelected(const QString& id);
    const QString& selected() const { return selected_; }

    // Auswahl per Klick in 3D (A-G-00014) und geänderte Referenzen.
    std::function<void(const QString&)> onPicked;
    std::function<void()> onReferencesChanged;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    struct BodyActor {
        vtkSmartPointer<vtkPolyData> mesh;
        vtkSmartPointer<vtkPolyData> originalMesh;
        vtkSmartPointer<vtkPolyData> materialMesh;
        vtkSmartPointer<vtkPolyData> originalMaterialMesh;
        bool coreTransformApplied = false;
        vtkSmartPointer<vtkActor> body;
        vtkSmartPointer<vtkTriangleFilter> triangles;
        vtkSmartPointer<vtkPolyDataNormals> normals;
        vtkSmartPointer<vtkPolyDataTangents> tangents;
        double baseColor[3] = {0.0, 0.0, 0.0};
        int materialPreview = 0;
        double opacity = 1.0;
        DisplayPart::Surface surface = DisplayPart::Surface::Default;
        vtkSmartPointer<vtkActor> waterline;
        vtkSmartPointer<vtkActor> reflection;
        vtkSmartPointer<vtkClipPolyData> reflectionClipper;
        vtkSmartPointer<vtkPolyData> waterlineData;
        double waterlineOffset = 0.0;
        QString elementId;
        QString group;
        vtkSmartPointer<vtkPlane> localWaterPlane;
        vtkSmartPointer<vtkCutter> cutter;
    };
    struct Visual {
        vtkSmartPointer<vtkPolyData> lines;
        vtkSmartPointer<vtkPolyData> head;
        vtkSmartPointer<vtkActor> lineActor;
        vtkSmartPointer<vtkActor> headActor;
    };

    void onRenderStart(vtkObject* caller, unsigned long event, void* data);
    void rebuildForceGeometry();
    void hideForceVisuals();
    Visual& visualFor(const QString& id);
    vtkRenderer* forceRenderer() const;
    void moveForceActors();
    void updateScaling();
    void ensureReferences();
    bool contributionVisible(const forcedisplay::Contribution& contribution) const;
    double worldPerLogicalPixel(const forcedisplay::Vec3& point) const;
    QPointF toLogicalScreen(const forcedisplay::Vec3& point) const;
    void pickAt(const QPoint& position);
    void requestRender();
    void applyBodyStyle();
    void applyGraphicsProfile();
    void updateEnvironmentTexture();
    void updateWaterAnimation();
    void updateWaterPlane();
    void rebuildEnvironment();
    void rebuildWaterDisplay(const std::function<void(double, double, unsigned char*)>& waterRgba);
    void updateTrail();
    void updateWaterMotion();
    void addEnvironmentActors();
    bool bodyBounds(double bounds[6]) const;
    double bodyHalfDiagonal() const;
    void frameBodies();
    void frameBounds(const double bounds[6], double zoom);

    QVTKOpenGLNativeWidget* vtkWidget_ = nullptr;
    navigation::Overlay* navigation_ = nullptr;
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindow_;
    vtkSmartPointer<vtkRenderer> renderer_;
    vtkSmartPointer<vtkRenderer> overlay_;
    vtkSmartPointer<vtkRenderer> axesRenderer_;
    vtkSmartPointer<vtkAxesActor> axesActor_;
    vtkSmartPointer<vtkTexture> skyTexture_;
    vtkSmartPointer<vtkTexture> proceduralSkyTexture_;
    QString loadedEnvironmentPath_;
    QString environmentDiagnostic_;
    vtkSmartPointer<vtkSkybox> skybox_;
    vtkSmartPointer<vtkLightKit> baseLights_;
    vtkSmartPointer<vtkLight> sunLight_;
    vtkSmartPointer<vtkLight> fillLight_;
    vtkSmartPointer<vtkTexture> proceduralColorTexture_;
    vtkSmartPointer<vtkTexture> proceduralOrmTexture_;
    vtkSmartPointer<vtkTexture> proceduralNormalTexture_;
    vtkSmartPointer<vtkTexture> woodColorTexture_;
    vtkSmartPointer<vtkTexture> woodOrmTexture_;
    vtkSmartPointer<vtkTexture> woodNormalTexture_;
    vtkSmartPointer<vtkTexture> clothColorTexture_;
    vtkSmartPointer<vtkTexture> clothOrmTexture_;
    vtkSmartPointer<vtkTexture> clothNormalTexture_;
    vtkSmartPointer<vtkTexture> waterNormalTexture_;
    vtkSmartPointer<vtkTransform> waterNormalTransform_;
    vtkSmartPointer<vtkTriangleFilter> waterTriangles_;
    vtkSmartPointer<vtkPolyDataTangents> waterTangents_;
    graphics::Profile graphicsProfile_;
    // Wasser: Polarnetz um den Kamerafußpunkt mit Fresnel-Deckkraft und
    // Auslauf zum Horizont, Tiefenebene und weltfestes Raster.
    vtkSmartPointer<vtkPolyData> waterMesh_;
    vtkSmartPointer<vtkPolyData> waterDisplay_;
    vtkSmartPointer<vtkActor> waterActor_;
    vtkSmartPointer<vtkPolyData> deepMesh_;
    vtkSmartPointer<vtkActor> deepActor_;
    vtkSmartPointer<vtkPolyData> gridMesh_;
    vtkSmartPointer<vtkActor> gridActor_;
    vtkSmartPointer<vtkPolyData> trailMesh_;
    vtkSmartPointer<vtkActor> trailActor_;
    vtkSmartPointer<vtkPolyData> motionMesh_;
    vtkSmartPointer<vtkActor> motionActor_;
    vtkSmartPointer<vtkPolyData> wakeMesh_;
    vtkSmartPointer<vtkActor> wakeActor_;
    watermotion::History motionHistory_;
    qsizetype motionStateCount_ = 0;
    std::optional<watermotion::Input> lastMotion_;
    QString motionDiagnostic_;
    struct TrackPoint {
        double time = 0.0;
        forcedisplay::Vec3 point{};
    };
    QVector<TrackPoint> track_;
    double currentTime_ = 0.0;
    double lastRenderDurationMs_ = 0.0;
    double sceneSpan_ = 1.0;
    // Trockenbereiche in der Wasserebene (Welt-xy) und ihre Hüllbox.
    std::vector<std::vector<std::array<double, 2>>> dryLoops_;
    QHash<QString, QString> elementGroups_;
    std::array<double, 4> dryBox_{};
    double horizonRadius_ = 0.0;
    vtkSmartPointer<vtkPolyData> leverLines_;
    vtkSmartPointer<vtkActor> leverActor_;
    vtkSmartPointer<vtkPolyData> lateralLines_;
    vtkSmartPointer<vtkActor> lateralActor_;
    vtkSmartPointer<vtkBillboardTextActor3D> lateralLabel_;
    std::vector<BodyActor> bodies_;
    QHash<QString, Visual> visuals_;
    QHash<QString, forcedisplay::Vec3> arcBasis_;
    QHash<QString, forcedisplay::ScaledMagnitude> scaled_;
    QVector<forcedisplay::ScreenShape> screenShapes_;
    forcedisplay::ForceState forces_;
    forcedisplay::Pose pose_;
    forcedisplay::Pose cameraPose_;
    std::optional<forcedisplay::Vec3> velocityWorld_;
    bool hasState_ = false;
    DisplayOptions options_;
    double forceReference_ = 0.0;
    double momentReference_ = 0.0;
    QString selected_;
    std::array<double, 15> cameraKey_{};
    bool geometryDirty_ = true;
    unsigned long renderObserver_ = 0;
    QPoint pressPosition_;
    bool pressActive_ = false;
};
