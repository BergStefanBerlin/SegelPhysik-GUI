#pragma once

#include <segelphysik/inspektion.hpp>

#include <QWidget>
#include <QSet>

#include <memory>
#include <array>
#include <exception>

class QLabel;
class QLineEdit;
class QTreeWidget;
class VtkSceneWidget;
class QGridLayout;

class BoatInspection final : public QWidget {
public:
    explicit BoatInspection(QWidget* parent = nullptr);
    QWidget* valuesWidget() const;
    VtkSceneWidget* scene() const { return scene_; }
    void setSnapshot(std::shared_ptr<const segelphysik::ModelInspection> snapshot);
    void clear(const QString& message);
    void setCatalogWidget(QWidget* widget);

private:
    void updateTree();
    void updateMesh();
    void updateSelection();
    void appendGeometryNodes();
    void updateMovementPreview();
    void filterValues();
    void reportError(const std::exception& error);

    std::shared_ptr<const segelphysik::ModelInspection> snapshot_;
    std::vector<segelphysik::InspectionMesh> meshes_;
    QSet<int> activeMeshes_;
    QHash<QString, segelphysik::InspectionMovementPreview> movements_;
    QTreeWidget* tree_ = nullptr;
    QTreeWidget* values_ = nullptr;
    std::array<QLineEdit*, 6> filters_{};
    QWidget* valuesPanel_ = nullptr;
    QLabel* valuesStatus_ = nullptr;
    QLabel* diagnostic_ = nullptr;
    QLabel* selectionStatus_ = nullptr;
    VtkSceneWidget* scene_ = nullptr;
    QGridLayout* layout_ = nullptr;
};
