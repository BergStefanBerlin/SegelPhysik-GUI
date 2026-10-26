#pragma once

#include "display_geometry.hpp"
#include "force_display.hpp"

#include <QHash>
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
class vtkCutter;
class vtkGenericOpenGLRenderWindow;
class vtkObject;
class vtkPlane;
class vtkPlaneSource;
class vtkPolyData;
class vtkRenderer;

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
        Visibility visibility = Visibility::ThroughBody;
        forcedisplay::ScaleMode forceMode = forcedisplay::ScaleMode::Logarithmic;
        forcedisplay::ScaleMode momentMode = forcedisplay::ScaleMode::Logarithmic;
    };

    explicit VtkSceneWidget(QWidget* parent = nullptr);
    ~VtkSceneWidget() override;

    bool setModel(const QVector<DisplayPart>& parts, QString* error);
    void clearModel();
    bool setState(const QJsonObject& state, const forcedisplay::ForceState& forces,
        QString* error);
    void clearState();
    void frameStates(const QVector<QJsonObject>& states);

    void setOptions(const DisplayOptions& options);
    const DisplayOptions& options() const { return options_; }

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
        vtkSmartPointer<vtkActor> body;
        vtkSmartPointer<vtkActor> waterline;
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
    void updateWaterPlane();
    bool bodyBounds(double bounds[6]) const;
    double bodyHalfDiagonal() const;
    void frameBodies();
    void keepBodiesInView();
    void frameBounds(const double bounds[6], double zoom);

    QVTKOpenGLNativeWidget* vtkWidget_ = nullptr;
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindow_;
    vtkSmartPointer<vtkRenderer> renderer_;
    vtkSmartPointer<vtkRenderer> overlay_;
    vtkSmartPointer<vtkActor> waterActor_;
    vtkSmartPointer<vtkPlaneSource> waterPlane_;
    vtkSmartPointer<vtkPolyData> leverLines_;
    vtkSmartPointer<vtkActor> leverActor_;
    std::vector<BodyActor> bodies_;
    QHash<QString, Visual> visuals_;
    QHash<QString, forcedisplay::Vec3> arcBasis_;
    QHash<QString, forcedisplay::ScaledMagnitude> scaled_;
    QVector<forcedisplay::ScreenShape> screenShapes_;
    forcedisplay::ForceState forces_;
    forcedisplay::Pose pose_;
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
    bool framePending_ = false;
    double viewBounds_[6] = {};
    bool viewBoundsValid_ = false;
};
