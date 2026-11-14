#pragma once

#include "force_display.hpp"
#include "vtk_scene.hpp"

#include <segelphysik/inspektion.hpp>
#include <segelphysik/segelkonfigurator.hpp>

#include <QWidget>
#include <QSet>
#include <QPointF>
#include <QPointer>
#include <QStringList>

#include <memory>
#include <array>
#include <exception>
#include <functional>
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
class QThread;

class BoatInspection final : public QWidget {
public:
    explicit BoatInspection(QWidget* parent = nullptr);
    ~BoatInspection() override;
    QWidget* valuesWidget() const;
    VtkSceneWidget* scene() const { return scene_; }
    void setStructure(const segelphysik::Modellstruktur& structure);
    void setSnapshot(std::shared_ptr<const segelphysik::ModelInspection> snapshot,
        std::shared_ptr<const segelphysik::Modellkatalog> catalog = {},
        bool loadAerodynamics = true,
        std::optional<segelphysik::SegelKonfigurationSnapshot> sailConfiguration = std::nullopt);
    std::function<void()> onSailConfigurationSaving;
    std::function<void(const QString&, bool, const QString&,
        std::shared_ptr<const segelphysik::Modellkatalog>, const QString&)>
        onSailConfigurationFinished;
    std::function<void(bool)> onManualModeChanged;
    void clear(const QString& message);
    void setCatalogWidget(QWidget* widget);
    bool setCameraPreset(const QString& id, QString* error);
    bool finishManualMode();
    bool manualModeActive() const { return !manualElement_.isEmpty(); }
    bool sailSaveInProgress() const { return !saveThread_.isNull(); }

private:
    void updateTree();
    void appendGeometryNodes();
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
    bool stageSailAction(const segelphysik::SegelAktionAnforderung& request);
    bool showFastSailPose(const segelphysik::SegelLage& pose);
    void discardManualChanges();
    void confirmSailAction(const segelphysik::SegelAktionAnforderung& request);
    void reloadSailConfiguration();
    void applySailConfiguration(segelphysik::SegelKonfigurationSnapshot configuration);
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
    std::vector<segelphysik::InspectionMesh> manualMeshes_;
    std::optional<segelphysik::SegelKonfigurationSnapshot> manualBaseConfiguration_;
    std::optional<segelphysik::SegelLage> renderedManualPose_;
    QSet<int> activeMeshes_;
    QHash<QString, segelphysik::InspectionMovementPreview> movements_;
    QHash<QString, QStringList> movementDiagnostics_;
    QSet<QString> activeMovementPreviews_;
    QHash<QString, QString> referenceBadges_;
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
    QPointer<QThread> saveThread_;
    bool dragChanged_ = false;
    bool manualChanges_ = false;
    quint64 meshContentGeneration_ = 0;
    quint64 renderedMeshContentGeneration_ = std::numeric_limits<quint64>::max();
};
