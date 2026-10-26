#pragma once

#include "obj_mesh.hpp"

#include <QJsonObject>
#include <QVector>
#include <QWidget>

#include <vtkSmartPointer.h>

#include <vector>

class QVTKOpenGLNativeWidget;
class vtkActor;
class vtkCutter;
class vtkGenericOpenGLRenderWindow;
class vtkPlane;
class vtkPlaneSource;
class vtkPolyData;
class vtkRenderer;
class vtkTransform;

class VtkSceneWidget final : public QWidget {
public:
    explicit VtkSceneWidget(QWidget* parent = nullptr);

    bool setModel(const QString& modelPath, QString* error);
    bool setState(const QJsonObject& state, bool showForces, QString* error);
    void setForceVisibility(bool visible);
    void frameStates(const QVector<QJsonObject>& states);

private:
    struct BodyActor {
        vtkSmartPointer<vtkPolyData> mesh;
        vtkSmartPointer<vtkActor> body;
        vtkSmartPointer<vtkActor> waterline;
        vtkSmartPointer<vtkPlane> localWaterPlane;
        vtkSmartPointer<vtkCutter> cutter;
    };

    void clearForceActors();
    void updateWaterPlane();
    bool bodyBounds(double bounds[6]) const;
    void frameBodies();
    void keepBodiesInView();
    void frameBounds(const double bounds[6], double zoom);

    QVTKOpenGLNativeWidget* vtkWidget_ = nullptr;
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindow_;
    vtkSmartPointer<vtkRenderer> renderer_;
    vtkSmartPointer<vtkActor> waterActor_;
    vtkSmartPointer<vtkPlaneSource> waterPlane_;
    std::vector<BodyActor> bodies_;
    std::vector<vtkSmartPointer<vtkActor>> forceActors_;
    QJsonObject lastState_;
    bool showForces_ = true;
    bool framePending_ = false;
    double viewBounds_[6] = {};
    bool viewBoundsValid_ = false;
};
