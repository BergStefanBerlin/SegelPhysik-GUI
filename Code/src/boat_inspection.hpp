#pragma once

#include "force_display.hpp"
#include "vtk_scene.hpp"

#include <segelphysik/inspektion.hpp>
#include <segelphysik/segelkonfigurator.hpp>

#include <QWidget>
#include <QSet>
#include <QPointF>

#include <memory>
#include <array>
#include <exception>
#include <limits>
#include <optional>
#include <vector>

class QLabel;
class QLineEdit;
class QMenu;
class QTreeWidget;
class QGridLayout;
class QAction;
class QComboBox;

class BoatInspection final : public QWidget {
public:
    explicit BoatInspection(QWidget* parent = nullptr);
    QWidget* valuesWidget() const;
    VtkSceneWidget* scene() const { return scene_; }
    void setSnapshot(std::shared_ptr<const segelphysik::ModelInspection> snapshot,
        std::shared_ptr<const segelphysik::Modellkatalog> catalog = {});
    void clear(const QString& message);
    void setCatalogWidget(QWidget* widget);
    bool setCameraPreset(const QString& id, QString* error);

private:
    void updateTree();
    void updateMesh();
    void updateSelection();
    void bindCurrentGeometry();
    void updateMovementPreview();
    void updateSailHandles();
    void filterValues();
    void reportError(const std::exception& error);
    void showSailContextMenu(const QString& elementId, const QPoint& globalPosition);
    void setManualMode(const QString& elementId, bool enabled);
    void promptFockOffset();
    void setOriginalBuglage(const QString& elementId, segelphysik::SegelBuglage buglage);
    void runAutomaticPlacement(const QString& elementId);
    void onSailDrag(const QString& elementId, const QString& handle,
        const QPointF& point, VtkSceneWidget::InspectionDragPhase phase);
    void previewSailAction(const segelphysik::SegelAktionAnforderung& request);
    void confirmSailAction(const segelphysik::SegelAktionAnforderung& request);
    void reloadSailConfiguration();
    void updateConfiguredMeshes(const std::vector<segelphysik::InspectionMesh>& meshes);
    void showConfigurationDiagnostics(const std::vector<segelphysik::SegelDiagnose>& diagnostics);
    std::optional<segelphysik::SegelReferenz> sailReference(const QString& elementId) const;
    std::optional<segelphysik::SegelLage> sailPose(const QString& elementId) const;
    forcedisplay::Vec3 sailPoint(const QString& elementId,
        const forcedisplay::Vec3& local) const;
    std::optional<segelphysik::SegelEbene> activePlane() const;
    void cancelSailDrag();

    std::shared_ptr<const segelphysik::ModelInspection> snapshot_;
    std::shared_ptr<const segelphysik::Modellkatalog> catalog_;
    segelphysik::SegelKonfigurationSnapshot sailConfiguration_;
    std::vector<segelphysik::InspectionMesh> meshes_;
    QHash<QString, std::vector<segelphysik::InspectionMesh>> configuredMeshes_;
    QHash<QString, std::vector<segelphysik::InspectionMesh>> previewMeshes_;
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
    QString manualElement_;
    QString dragElement_;
    QString dragHandle_;
    QPointF dragStartScreen_;
    std::optional<segelphysik::SegelAktionAnforderung> dragRequest_;
    bool dragChanged_ = false;
    quint64 meshContentGeneration_ = 0;
    quint64 renderedMeshContentGeneration_ = std::numeric_limits<quint64>::max();
};
