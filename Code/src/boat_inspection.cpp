#include "boat_inspection.hpp"

#include "display_geometry.hpp"
#include "vtk_scene.hpp"
#include "filter_header.hpp"
#include <vtkPolyData.h>

#include <QHeaderView>
#include <QApplication>
#include <QAction>
#include <QActionGroup>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QColor>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QStyle>
#include <QSet>
#include <QSignalBlocker>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>

#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <limits>
#include <type_traits>
#include <utility>

namespace {
QString text(const std::string& value) { return QString::fromStdString(value); }
forcedisplay::Vec3 vector3(const segelphysik::Vektor3& value)
{
    return {value[0], value[1], value[2]};
}
segelphysik::Vektor3 coreVector(const forcedisplay::Vec3& value)
{
    return {value[0], value[1], value[2]};
}
constexpr int rawValueRole = Qt::UserRole + 10;
constexpr int colorRole = Qt::UserRole + 11;
constexpr int movementIdRole = Qt::UserRole + 12;
constexpr int movementActionRole = Qt::UserRole + 13;

class ValueItem final : public QTreeWidgetItem {
public:
    using QTreeWidgetItem::QTreeWidgetItem;
    bool operator<(const QTreeWidgetItem& other) const override
    {
        const int column = treeWidget()->sortColumn();
        const auto a = data(column, rawValueRole), b = other.data(column, rawValueRole);
        if (column == 3 && (a.isValid() || b.isValid())) {
            if (a.isValid() != b.isValid()) return a.isValid();
            return a.toDouble() < b.toDouble();
        }
        return QString::localeAwareCompare(text(column), other.text(column)) < 0;
    }
};

class ColorDelegate final : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override
    {
        const auto color = index.data(colorRole).value<QColor>();
        if (!color.isValid()) {
            QStyledItemDelegate::paint(painter, option, index);
            return;
        }
        auto background = option;
        initStyleOption(&background, index);
        background.text.clear();
        const auto* widget = option.widget;
        auto* style = widget ? widget->style() : QApplication::style();
        style->drawControl(QStyle::CE_ItemViewItem, &background, painter, widget);
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        const int side = std::min(22, option.rect.height()-4);
        const QRectF box(option.rect.left()+5, option.rect.center().y()-side/2., side, side);
        painter->setBrush(color);
        painter->setPen(QColor("#bbaab3"));
        painter->drawRoundedRect(box, 4, 4);
        painter->restore();
    }
};

QString number(double value)
{
    const auto locale = QLocale::c();
    if (value != 0. && (std::abs(value) < .01 || std::abs(value) >= 1e6))
        return locale.toString(value, 'g', 2);
    auto result = locale.toString(value, 'f', 2);
    while (result.contains(QLatin1Char('.')) && result.endsWith(QLatin1Char('0'))) result.chop(1);
    if (result.endsWith(QLatin1Char('.'))) result.chop(1);
    return result == QStringLiteral("-0") ? QStringLiteral("0") : result;
}

QString compactJson(const nlohmann::json& value, double divisor = 1.)
{
    if (value.is_number()) return number(value.get<double>() / divisor);
    if (value.is_array()) {
        QStringList items;
        for (const auto& item : value) items << compactJson(item, divisor);
        return QLatin1Char('[') + items.join(QStringLiteral(", ")) + QLatin1Char(']');
    }
    if (value.is_object()) {
        QStringList items;
        for (const auto& item : value.items())
            items << text(item.key()) + QStringLiteral(": ") + compactJson(item.value(), divisor);
        return QLatin1Char('{') + items.join(QStringLiteral(", ")) + QLatin1Char('}');
    }
    return text(value.dump());
}

std::pair<QString, double> displayUnit(const QString& unit, const nlohmann::json& value)
{
    double magnitude = 0.;
    const auto measure = [&](const auto& self, const nlohmann::json& entry) -> void {
        if (entry.is_number()) magnitude = std::max(magnitude, std::abs(entry.get<double>()));
        else if (entry.is_array()) for (const auto& child : entry) self(self, child);
    };
    measure(measure, value);
    if (unit == QStringLiteral("m")) {
        if (magnitude >= 1000.) return {QStringLiteral("km"), 1000.};
        if (magnitude > 0. && magnitude < .001) return {QStringLiteral("µm"), 1e-6};
        if (magnitude > 0. && magnitude < 1.) return {QStringLiteral("mm"), .001};
    }
    if (unit == QStringLiteral("kg")) {
        if (magnitude >= 1000.) return {QStringLiteral("t"), 1000.};
        if (magnitude > 0. && magnitude < .001) return {QStringLiteral("mg"), 1e-6};
        if (magnitude > 0. && magnitude < 1.) return {QStringLiteral("g"), .001};
    }
    return {unit, 1.};
}

QStringList meshUses(const std::vector<segelphysik::InspectionMesh>& meshes,
                     const segelphysik::InspectionMesh& mesh)
{
    QStringList uses;
    for (const auto& use : meshes)
        if (use.element_id == mesh.element_id && use.source_id == mesh.source_id
            && use.type != "quelle")
            uses << (text(use.type) + (use.provenance == "abgeleitet" ?
                QStringLiteral(" (Ableitung)") : QString()));
    uses.removeDuplicates();
    return uses;
}

forcedisplay::Vec3 rotateByQuaternion(const std::array<double, 4>& q,
    const forcedisplay::Vec3& value)
{
    const forcedisplay::Vec3 vector{q[0], q[1], q[2]};
    const auto first = forcedisplay::cross(vector, value);
    const auto second = forcedisplay::cross(vector, first);
    return forcedisplay::add(value,
        forcedisplay::add(forcedisplay::scale(first, 2.0 * q[3]),
            forcedisplay::scale(second, 2.0)));
}

QString formatValue(const segelphysik::InspectionValue& value)
{
    return std::visit([](const auto& item) -> QString {
        using T = std::decay_t<decltype(item)>;
        if constexpr (std::is_same_v<T, std::monostate>) return QStringLiteral("null");
        else if constexpr (std::is_same_v<T, std::string>) return text(item);
        else if constexpr (std::is_same_v<T, segelphysik::InspectionMatrix>)
            return text(nlohmann::json{{"rows", item.rows}, {"columns", item.columns},
                {"values", item.values}}.dump());
        else return text(nlohmann::json(item).dump());
    }, value);
}
}

BoatInspection::BoatInspection(QWidget* parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("boatInspection"));
    layout_ = new QGridLayout(this);
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(14);
    scene_ = new VtkSceneWidget;
    scene_->setObjectName(QStringLiteral("inspectionScene"));
    scene_->setInspectionMode();
    layout_->addWidget(scene_, 0, 1);
    selectionStatus_ = new QLabel;
    selectionStatus_->setObjectName(QStringLiteral("inspectionSelectionStatus"));
    selectionStatus_->setWordWrap(true);
    layout_->addWidget(selectionStatus_, 1, 1);

    auto* panel = new QWidget;
    panel->setObjectName(QStringLiteral("panel"));
    panel->setMinimumWidth(300);
    panel->setMaximumWidth(420);
    auto* panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(14, 18, 14, 14);
    panelLayout->setSpacing(10);
    auto* caption = new QLabel(QStringLiteral("Objekt / Teil"));
    caption->setObjectName(QStringLiteral("caption"));
    panelLayout->addWidget(caption);
    diagnostic_ = new QLabel(QStringLiteral("Modell auswählen"));
    diagnostic_->setObjectName(QStringLiteral("inspectionDiagnostics"));
    diagnostic_->setWordWrap(true);
    diagnostic_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    panelLayout->addWidget(diagnostic_);
    tree_ = new QTreeWidget;
    tree_->setObjectName(QStringLiteral("inspectionObjectTree"));
    tree_->setHeaderLabel(QStringLiteral("Objekt / Teil"));
    tree_->setHeaderHidden(true);
    tree_->setStyleSheet(QStringLiteral(
        "QTreeWidget#inspectionObjectTree { border-radius:12px; padding:3px; }"));
    tree_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tree_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tree_->setContextMenuPolicy(Qt::CustomContextMenu);
    panelLayout->addWidget(tree_, 1);
    layout_->addWidget(panel, 0, 2);
    layout_->setColumnStretch(1, 1);
    layout_->setRowStretch(0, 1);

    valuesPanel_ = new QWidget(this);
    auto* valuesLayout = new QVBoxLayout(valuesPanel_);
    valuesStatus_ = new QLabel(QStringLiteral("Kein Objekt ausgewählt"));
    valuesStatus_->setObjectName(QStringLiteral("inspectionValuesStatus"));
    valuesLayout->addWidget(valuesStatus_);
    values_ = new QTreeWidget;
    values_->setObjectName(QStringLiteral("inspectionValues"));
    auto* header = new FilterHeader(values_);
    header->setObjectName(QStringLiteral("inspectionValueHeader"));
    header->setStyleSheet(QStringLiteral(
        "QHeaderView::section { border-right:1px solid #795565; }"));
    values_->setHeader(header);
    values_->setHeaderLabels({QStringLiteral("Objekt/Pfad"), QStringLiteral("Bereich"), QStringLiteral("Eigenschaft"),
        QStringLiteral("Wert"), QStringLiteral("Einheit"), QStringLiteral("Status")});
    values_->setRootIsDecorated(false);
    values_->setAlternatingRowColors(true);
    values_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    values_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    values_->setItemDelegateForColumn(3, new ColorDelegate(values_));
    values_->header()->setSectionResizeMode(QHeaderView::Interactive);
    values_->header()->setStretchLastSection(false);
    values_->header()->setMinimumSectionSize(45);
    values_->setSortingEnabled(true);
    values_->sortItems(0, Qt::AscendingOrder);
    values_->setColumnWidth(0, 240);
    values_->setColumnWidth(1, 130);
    values_->setColumnWidth(2, 200);
    values_->setColumnWidth(3, 350);
    for (int i = 0; i < 6; ++i) {
        auto* filter = new QLineEdit(header->viewport());
        filters_[i] = filter;
        filter->setObjectName(QStringLiteral("inspectionFilter%1").arg(i));
        filter->setPlaceholderText(QStringLiteral("*"));
        filter->setToolTip(values_->headerItem()->text(i) + QStringLiteral(" filtern"));
        filter->setClearButtonEnabled(true);
        connect(filter, &QLineEdit::textChanged, this, [this] { filterValues(); });
    }
    header->setFilters(filters_);
    valuesLayout->addWidget(values_, 1);
    connect(values_, &QTreeWidget::itemSelectionChanged, this, [this] { updateMovementPreview(); });
    valuesPanel_->hide();
    connect(tree_, &QTreeWidget::itemSelectionChanged, this, [this] { updateSelection(); });
    connect(tree_, &QTreeWidget::itemChanged, this, [this](QTreeWidgetItem*, int) {
        updateSelection();
    });
    connect(tree_, &QWidget::customContextMenuRequested, this, [this](const QPoint& position) {
        auto* item = tree_->itemAt(position);
        if (!item || item->data(0, Qt::UserRole + 4).toString() != QStringLiteral("element"))
            return;
        const auto ids = item->data(0, Qt::UserRole + 1).toStringList();
        if (ids.size() != 1
            || (ids.front() != QStringLiteral("Großsegel") && ids.front() != QStringLiteral("Fock")))
            return;
        showSailContextMenu(ids.front(), tree_->viewport()->mapToGlobal(position));
    });
    scene_->onInspectionContextMenu = [this](const QString& id, const QPoint& position) {
        showSailContextMenu(id, position);
    };
    scene_->onInspectionPicked = [this](const QString& elementId) {
        for (QTreeWidgetItemIterator it(tree_); *it; ++it) {
            if ((*it)->data(0, Qt::UserRole + 4).toString() != QStringLiteral("element"))
                continue;
            const auto ids = (*it)->data(0, Qt::UserRole + 1).toStringList();
            if (ids.size() == 1 && ids.front() == elementId) {
                tree_->setCurrentItem(*it);
                (*it)->setSelected(true);
                return;
            }
        }
    };
    scene_->onInspectionInteractionHint = [this](const QString& hint) {
        diagnostic_->setText(hint);
        diagnostic_->setStyleSheet(QStringLiteral("color:#ffbf69;"));
    };
    scene_->onInspectionDrag = [this](const QString& elementId, const QString& handle,
        const QPointF& point, VtkSceneWidget::InspectionDragPhase phase) {
        onSailDrag(elementId, handle, point, phase);
    };
    scene_->onInspectionInteractionCancelled = [this] { cancelSailDrag(); };
}

QWidget* BoatInspection::valuesWidget() const { return valuesPanel_; }

void BoatInspection::setCatalogWidget(QWidget* widget)
{
    layout_->addWidget(widget, 0, 0);
}

bool BoatInspection::setCameraPreset(const QString& id, QString* error)
{
    if (!scene_->setInspectionCameraPreset(id, error)) return false;
    if (!scene_->inspectionEditView() && !manualElement_.isEmpty())
        setManualMode(manualElement_, false);
    if (scene_->inspectionEditView() && !manualElement_.isEmpty())
        scene_->setInspectionEditTarget(manualElement_);
    updateSailHandles();
    return true;
}

void BoatInspection::showConfigurationDiagnostics(
    const std::vector<segelphysik::SegelDiagnose>& diagnostics)
{
    QStringList lines;
    for (const auto& diagnostic : diagnostics) {
        const auto textLine = diagnostic.element_id.empty()
            ? QStringLiteral("%1: %2").arg(text(diagnostic.code), text(diagnostic.nachricht))
            : QStringLiteral("%1: %2 (%3)").arg(text(diagnostic.code),
                text(diagnostic.nachricht), text(diagnostic.element_id));
        lines << textLine;
    }
    if (!lines.isEmpty()) {
        diagnostic_->setText(lines.join(QLatin1Char('\n')));
        diagnostic_->setStyleSheet(QStringLiteral("color:#ffbf69;"));
    }
}

std::optional<segelphysik::SegelReferenz> BoatInspection::sailReference(
    const QString& elementId) const
{
    std::optional<segelphysik::SegelReferenz> result;
    for (const auto& reference : sailConfiguration_.referenzen) {
        if (text(reference.element_id) != elementId) continue;
        if (result) return std::nullopt;
        result = reference;
    }
    return result;
}

std::optional<segelphysik::SegelLage> BoatInspection::sailPose(
    const QString& elementId) const
{
    for (std::size_t i = 0; i < sailConfiguration_.referenzen.size()
        && i < sailConfiguration_.lagen.size(); ++i)
        if (text(sailConfiguration_.referenzen[i].element_id) == elementId)
            return sailConfiguration_.lagen[i];
    return std::nullopt;
}

forcedisplay::Vec3 BoatInspection::sailPoint(const QString& elementId,
    const forcedisplay::Vec3& local) const
{
    const auto pose = sailPose(elementId);
    if (!pose) return local;
    return forcedisplay::add(
        rotateByQuaternion(pose->rotation_xyzw, local), vector3(pose->translation_m));
}

std::optional<segelphysik::SegelEbene> BoatInspection::activePlane() const
{
    const auto preset = scene_->inspectionCameraPresetId();
    if (!scene_->inspectionEditView()) return std::nullopt;
    if (preset == QStringLiteral("backbord") || preset == QStringLiteral("steuerbord"))
        return segelphysik::SegelEbene::XZ;
    if (preset == QStringLiteral("bug") || preset == QStringLiteral("heck"))
        return segelphysik::SegelEbene::YZ;
    if (preset == QStringLiteral("oben") || preset == QStringLiteral("unten"))
        return segelphysik::SegelEbene::XY;
    return std::nullopt;
}

void BoatInspection::reportError(const std::exception& error)
{
    diagnostic_->setText(QString::fromUtf8(error.what()));
    diagnostic_->setStyleSheet(QStringLiteral("color:#ffbf69;"));
}

void BoatInspection::clear(const QString& message)
{
    cancelSailDrag();
    snapshot_.reset();
    catalog_.reset();
    sailConfiguration_ = {};
    configuredMeshes_.clear();
    previewMeshes_.clear();
    manualElement_.clear();
    dragElement_.clear();
    const QSignalBlocker treeBlock(tree_);
    tree_->clear();
    meshes_.clear();
    activeMeshes_.clear();
    ++meshContentGeneration_;
    renderedMeshContentGeneration_ = std::numeric_limits<quint64>::max();
    movements_.clear();
    values_->clear();
    valuesStatus_->setText(QStringLiteral("Kein Objekt ausgewählt"));
    diagnostic_->setText(message);
    selectionStatus_->clear();
    scene_->clearModel();
    scene_->setInspectionEditTarget({});
    scene_->setInspectionHandles({});
    scene_->setProperty("hasInspectionMesh", false);
}

void BoatInspection::setSnapshot(std::shared_ptr<const segelphysik::ModelInspection> snapshot,
    std::shared_ptr<const segelphysik::Modellkatalog> catalog)
{
    clear(QString());
    catalog_ = std::move(catalog);
    snapshot_ = std::move(snapshot);
    if (!snapshot_) return;
    try {
        for (const auto& type : segelphysik::getAvailableMeshTypes(*snapshot_)) {
            if (type.id == "quelle") continue;
            const auto meshes = segelphysik::getMesh(*snapshot_, type.id);
            meshes_.insert(meshes_.end(), meshes.begin(), meshes.end());
        }
        for (auto mesh : segelphysik::getMesh(*snapshot_, "quelle")) {
            if (mesh.element_ids.empty()) meshes_.push_back(std::move(mesh));
            else for (const auto& element : mesh.element_ids) {
                mesh.element_id = element;
                meshes_.push_back(mesh);
            }
        }
        QStringList diagnostics;
        if (snapshot_->info().status == "nur_ansicht")
            diagnostics << QStringLiteral("Nur Ansicht – keine Simulationsfreigabe");
        for (const auto& d : snapshot_->info().diagnostics)
            diagnostics << QStringLiteral("%1: %2 (%3)")
                .arg(text(d.fehler_id), text(d.text), text(d.fundstelle));
        diagnostic_->setText(diagnostics.join(QLatin1Char('\n')));
        diagnostic_->setStyleSheet(QString());
        updateTree();
        updateMesh();
        reloadSailConfiguration();
    } catch (const std::exception& error) {
        reportError(error);
    }
}

void BoatInspection::showSailContextMenu(const QString& elementId,
    const QPoint& globalPosition)
{
    if (elementId != QStringLiteral("Großsegel") && elementId != QStringLiteral("Fock"))
        return;
    if (!snapshot_ || !catalog_ || !sailReference(elementId)) {
        diagnostic_->setText(QStringLiteral(
            "Core-Konfiguration fehlt oder ordnet dem Segel keine eindeutige Referenz zu."));
        diagnostic_->setStyleSheet(QStringLiteral("color:#ffbf69;"));
        return;
    }

    QMenu menu(this);
    auto* automatic = menu.addAction(elementId == QStringLiteral("Fock")
        ? QStringLiteral("Fock automatisch setzen")
        : QStringLiteral("Großsegel automatisch setzen"));
    connect(automatic, &QAction::triggered, this,
        [this, elementId] { runAutomaticPlacement(elementId); });
    auto* manual = menu.addAction(QStringLiteral("Manuelles Verschieben"));
    manual->setCheckable(true);
    manual->setChecked(manualElement_ == elementId);
    connect(manual, &QAction::toggled, this,
        [this, elementId](bool checked) { setManualMode(elementId, checked); });
    if (elementId == QStringLiteral("Fock") && manualElement_ == elementId) {
        auto* offset = menu.addAction(QStringLiteral("Höhenversatz entlang Vorliek (cm)…"));
        connect(offset, &QAction::triggered, this, [this] { promptFockOffset(); });
    }

    menu.addSeparator();
    auto* original = menu.addMenu(QStringLiteral("Original gezeichnet für"));
    QActionGroup* originalGroup = new QActionGroup(original);
    originalGroup->setExclusive(true);
    auto currentBuglage = segelphysik::SegelBuglage::Unbekannt;
    if (const auto pose = sailPose(elementId)) currentBuglage = pose->original_buglage;
    else if (const auto reference = sailReference(elementId))
        currentBuglage = reference->original_buglage;
    const std::array<std::pair<const char*, segelphysik::SegelBuglage>, 3> buglagen{{
        {"Nicht festgelegt", segelphysik::SegelBuglage::Unbekannt},
        {"Backbordbug", segelphysik::SegelBuglage::Backbord},
        {"Steuerbordbug", segelphysik::SegelBuglage::Steuerbord}}};
    for (const auto& entry : buglagen) {
        auto* action = original->addAction(QString::fromUtf8(entry.first));
        action->setCheckable(true);
        action->setChecked(currentBuglage == entry.second);
        originalGroup->addAction(action);
        connect(action, &QAction::triggered, this,
            [this, elementId, buglage = entry.second] { setOriginalBuglage(elementId, buglage); });
    }
    menu.exec(globalPosition);
}

void BoatInspection::setManualMode(const QString& elementId, bool enabled)
{
    if (!enabled) {
        if (elementId.isEmpty() || manualElement_ == elementId) {
            cancelSailDrag();
            manualElement_.clear();
            scene_->setInspectionEditTarget({});
            scene_->setInspectionHandles({});
        }
        return;
    }
    if (!scene_->inspectionEditView()) {
        diagnostic_->setText(QStringLiteral(
            "Für manuelles Verschieben bitte eine achsenfeste Kameraansicht wählen."));
        diagnostic_->setStyleSheet(QStringLiteral("color:#ffbf69;"));
        return;
    }
    if (!sailReference(elementId)) {
        diagnostic_->setText(QStringLiteral("Für dieses Element liegt keine eindeutige Core-Referenz vor."));
        diagnostic_->setStyleSheet(QStringLiteral("color:#ffbf69;"));
        return;
    }
    cancelSailDrag();
    manualElement_ = elementId;
    scene_->setInspectionEditTarget(elementId);
    updateSailHandles();
    diagnostic_->setText(QStringLiteral(
        "Manuelles Verschieben: Fläche zieht parallel zur Ansichtsebene; semantische Eckgriffe drehen starr."));
    diagnostic_->setStyleSheet(QString());
}

void BoatInspection::runAutomaticPlacement(const QString& elementId)
{
    segelphysik::SegelAktionAnforderung request{};
    request.aktion = segelphysik::SegelAktion::Automatisch;
    request.element_id = elementId.toStdString();
    request.ebene = segelphysik::SegelEbene::XY;
    confirmSailAction(request);
}

void BoatInspection::promptFockOffset()
{
    if (manualElement_ != QStringLiteral("Fock") || !sailReference(QStringLiteral("Fock"))) {
        diagnostic_->setText(QStringLiteral("Für die Fock liegt keine eindeutige Core-Referenz vor."));
        return;
    }
    const double currentOffset = sailPose(QStringLiteral("Fock"))
        ? sailPose(QStringLiteral("Fock"))->fock_versatz_cm : 0.0;
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("Fock-Versatz"));
    auto* layout = new QVBoxLayout(&dialog);
    auto* form = new QFormLayout;
    auto* value = new QLineEdit(&dialog);
    value->setObjectName(QStringLiteral("fockOffsetCm"));
    value->setText(QLocale::c().toString(currentOffset, 'g', 15));
    value->setPlaceholderText(QStringLiteral("z. B. 10 oder -10"));
    form->addRow(QStringLiteral("Absoluter Versatz (cm)"), value);
    layout->addLayout(form);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("Übernehmen"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("Abbrechen"));
    layout->addWidget(buttons);
    connect(buttons->button(QDialogButtonBox::Cancel), &QPushButton::clicked,
        &dialog, &QDialog::reject);
    connect(buttons->button(QDialogButtonBox::Ok), &QPushButton::clicked, &dialog, [&] {
        bool valid = false;
        const double offset = QLocale::c().toDouble(value->text().trimmed(), &valid);
        if (!valid || !std::isfinite(offset)) {
            QMessageBox::warning(&dialog, QStringLiteral("Ungültiger Versatz"),
                QStringLiteral("Bitte einen endlichen Zahlenwert in Zentimetern eingeben."));
            return;
        }
        dialog.accept();
    });
    if (dialog.exec() != QDialog::Accepted) return;
    bool valid = false;
    const double offset = QLocale::c().toDouble(value->text().trimmed(), &valid);
    if (!valid || !std::isfinite(offset)) return;
    segelphysik::SegelAktionAnforderung request{};
    request.aktion = segelphysik::SegelAktion::FockVersatzCm;
    request.element_id = QStringLiteral("Fock").toStdString();
    request.ebene = segelphysik::SegelEbene::XY;
    request.fock_versatz_cm = offset;
    confirmSailAction(request);
}

void BoatInspection::setOriginalBuglage(const QString& elementId,
    segelphysik::SegelBuglage buglage)
{
    segelphysik::SegelAktionAnforderung request{};
    request.aktion = segelphysik::SegelAktion::OriginalBuglage;
    request.element_id = elementId.toStdString();
    request.ebene = segelphysik::SegelEbene::XY;
    request.original_buglage = buglage;
    previewSailAction(request);
    confirmSailAction(request);
}

void BoatInspection::reloadSailConfiguration()
{
    if (!catalog_ || !snapshot_) return;
    try {
        auto configuration = segelphysik::leseSegelKonfiguration(*catalog_, *snapshot_);
        if (configuration.status == segelphysik::SegelKonfigurationsStatus::Fehler) {
            showConfigurationDiagnostics(configuration.diagnosen);
            if (configuration.diagnosen.empty()) {
                diagnostic_->setText(QStringLiteral(
                    "Core-Konfiguration fehlerhaft; der letzte bestätigte Segelstand bleibt aktiv."));
                diagnostic_->setStyleSheet(QStringLiteral("color:#ff8a7a;"));
            }
            return;
        }
        const auto geometry = segelphysik::leseKonfigurierteSegelgeometrie(configuration);
        if (geometry.status == segelphysik::SegelErgebnisStatus::Fehler) {
            showConfigurationDiagnostics(geometry.diagnosen);
            if (geometry.diagnosen.empty()) {
                diagnostic_->setText(QStringLiteral(
                    "Core-Segelgeometrie fehlerhaft; der letzte bestätigte Segelstand bleibt aktiv."));
                diagnostic_->setStyleSheet(QStringLiteral("color:#ff8a7a;"));
            }
            return;
        }
        sailConfiguration_ = std::move(configuration);
        updateConfiguredMeshes(geometry.meshes);
        showConfigurationDiagnostics(sailConfiguration_.diagnosen);
        updateMesh();
        updateSailHandles();
    } catch (const std::exception& error) {
        reportError(error);
    }
}

void BoatInspection::updateConfiguredMeshes(
    const std::vector<segelphysik::InspectionMesh>& meshes)
{
    configuredMeshes_.clear();
    for (const auto& mesh : meshes)
        if (mesh.element_id == "Großsegel" || mesh.element_id == "Fock")
            configuredMeshes_[text(mesh.element_id)].push_back(mesh);
    ++meshContentGeneration_;
    updateSailHandles();
}

void BoatInspection::previewSailAction(
    const segelphysik::SegelAktionAnforderung& request)
{
    try {
        const auto result = segelphysik::pruefeSegelAktion(sailConfiguration_, request);
        const auto elementId = text(request.element_id);
        if (result.status == segelphysik::SegelErgebnisStatus::Fehler) {
            previewMeshes_.remove(elementId);
            showConfigurationDiagnostics(result.diagnosen);
        } else {
            previewMeshes_[elementId] = result.meshes;
            diagnostic_->setText(QStringLiteral("Unbestätigte Vorschau – Änderungen werden erst beim Loslassen gespeichert."));
            diagnostic_->setStyleSheet(QString());
        }
        ++meshContentGeneration_;
        updateMesh();
        updateSailHandles();
    } catch (const std::exception& error) {
        reportError(error);
    }
}

void BoatInspection::confirmSailAction(
    const segelphysik::SegelAktionAnforderung& request)
{
    if (!catalog_ || !snapshot_) {
        diagnostic_->setText(QStringLiteral("Keine Core-Modellreferenz für die Konfiguration verfügbar."));
        diagnostic_->setStyleSheet(QStringLiteral("color:#ff8a7a;"));
        return;
    }
    try {
        const auto result = segelphysik::bestaetigeSegelAktion(*catalog_, *snapshot_,
            sailConfiguration_, request, sailConfiguration_.revision);
        const auto elementId = text(request.element_id);
        if (result.status != segelphysik::SegelErgebnisStatus::Gespeichert) {
            previewMeshes_.remove(elementId);
            ++meshContentGeneration_;
            QStringList failureLines;
            for (const auto& d : result.diagnosen)
                failureLines << QStringLiteral("%1: %2")
                    .arg(text(d.code), text(d.nachricht));
            showConfigurationDiagnostics(result.diagnosen);
            if (failureLines.isEmpty())
                failureLines << QStringLiteral(
                    "Core hat die Segeländerung nicht dauerhaft bestätigt.");
            reloadSailConfiguration();
            const auto reloadMessage = diagnostic_->text();
            if (!reloadMessage.isEmpty() && !failureLines.contains(reloadMessage))
                failureLines << reloadMessage;
            diagnostic_->setText(failureLines.join(QLatin1Char('\n')));
            diagnostic_->setStyleSheet(QStringLiteral("color:#ff8a7a;"));
            updateMesh();
            updateSailHandles();
            return;
        }
        sailConfiguration_.revision = result.revision;
        if (result.lage) {
            for (std::size_t i = 0; i < sailConfiguration_.referenzen.size()
                && i < sailConfiguration_.lagen.size(); ++i) {
                if (text(sailConfiguration_.referenzen[i].element_id) == elementId) {
                    sailConfiguration_.lagen[i] = *result.lage;
                    break;
                }
            }
        }
        if (!result.meshes.empty()) configuredMeshes_[elementId] = result.meshes;
        previewMeshes_.remove(elementId);
        ++meshContentGeneration_;
        diagnostic_->setText(QStringLiteral("Segelkonfiguration dauerhaft gespeichert."));
        diagnostic_->setStyleSheet(QString());
        updateMesh();
        updateSailHandles();
    } catch (const std::exception& error) {
        previewMeshes_.remove(text(request.element_id));
        ++meshContentGeneration_;
        reportError(error);
        updateMesh();
    }
}

void BoatInspection::updateTree()
{
    if (!snapshot_) return;
    const QSignalBlocker blocker(tree_);
    tree_->clear();
    values_->clear();
    values_->setRootIsDecorated(false);
    valuesStatus_->setText(QStringLiteral("Kein Objekt ausgewählt"));
    scene_->setInspectionSelection({});
    selectionStatus_->clear();
    try {
        const auto nodes = segelphysik::getObjectTree(*snapshot_, "objekte");
        QHash<QString, QTreeWidgetItem*> items;
        QSet<QString> objectIds;
        for (const auto& node : nodes) {
            if (items.contains(text(node.id)))
                throw std::runtime_error("Doppelte Objektreferenz im Core-Baum: " + node.id);
            if (node.kind == "element") {
                if (node.element_ids.size() != 1
                    || objectIds.contains(text(node.element_ids.front())))
                    throw std::runtime_error("Mehrdeutige Objektidentitaet im Core-Baum: " + node.id);
                objectIds.insert(text(node.element_ids.front()));
            }
            auto* item = new QTreeWidgetItem;
            item->setText(0, text(node.name));
            item->setData(0, Qt::UserRole, text(node.id));
            QStringList elements;
            for (const auto& id : node.element_ids) elements << text(id);
            item->setData(0, Qt::UserRole + 1, elements);
            item->setToolTip(0, text(node.id));
            item->setData(0, Qt::UserRole + 4, text(node.kind));
            items.insert(text(node.id), item);
        }
        for (const auto& node : nodes) {
            auto* item = items.value(text(node.id));
            auto* parent = items.value(text(node.parent_id));
            if (parent && parent != item) parent->addChild(item);
            else tree_->addTopLevelItem(item);
        }
        bindCurrentGeometry();
        for (const auto& node : nodes) {
            if (node.kind != "element" || node.element_ids.size() != 1) continue;
            const auto movement = segelphysik::getMovementPreview(*snapshot_, node.element_ids.front());
            if (!movement) continue;
            const auto id = text(node.element_ids.front());
            movements_.insert(id, *movement);
        }
        tree_->expandToDepth(1);
        updateMesh();
        updateMovementPreview();
    } catch (const std::exception& error) {
        reportError(error);
    }

}

void BoatInspection::updateMovementPreview()
{
    try {
        QHash<QString, QSet<QString>> requested;
        QHash<QString, QSet<QString>> selectedActions;
        for (auto* row : values_->selectedItems())
            if (!row->isHidden() && !row->data(0, movementIdRole).toString().isEmpty())
                selectedActions[row->data(0, movementIdRole).toString()].insert(
                    row->data(0, movementActionRole).toString());
        for (auto it = selectedActions.cbegin(); it != selectedActions.cend(); ++it)
            requested[it.key()] = it.value();
        QVector<DisplayPart> parts;
        std::vector<std::array<forcedisplay::Vec3, 2>> axes;
        std::vector<forcedisplay::Vec3> pivots;
        int poseCount = 0;
        for (auto request = requested.cbegin(); request != requested.cend(); ++request) {
            const auto movement = movements_.value(request.key());
            const auto& actions = request.value();
            for (const auto& pose : movement.poses) {
                    if (!actions.contains(QStringLiteral("all")) && !actions.contains(text(pose.label))) continue;
                    if (pose.meshes.empty()) continue;
                    ++poseCount;
                    auto converted = toDisplayParts(pose.meshes);
                    const std::array<std::array<double, 3>, 3> colors = {{
                        {0.25, 0.85, 0.90}, {1.0, 0.55, 0.20}, {0.75, 0.45, 1.0}}};
                    const auto& color = colors[pose.label == "minimum" ? 1 : pose.label == "maximum" ? 2 : 0];
                    for (auto& part : converted)
                        for (int c = 0; c < 3; ++c) part.color[c] = color[c];
                    parts += converted;
            }
            if (movement.axis_point_m && (actions.contains(QStringLiteral("all")) ||
                actions.contains(QStringLiteral("pivot")))) {
                    pivots.push_back(*movement.axis_point_m);
                    auto from = *movement.axis_point_m, to = from;
                    for (int c = 0; c < 3; ++c) {
                        from[c] -= movement.axis_direction[c];
                        to[c] += movement.axis_direction[c];
                    }
                    axes.push_back({from, to});
            }
        }
        scene_->setInspectionPreview(parts, axes, pivots);
        scene_->setProperty("inspectionMovementPreviewCount", requested.size());
        scene_->setProperty("inspectionMovementPoseCount", poseCount);
        scene_->setProperty("inspectionPivotCount", static_cast<int>(pivots.size()));
    } catch (const std::exception& error) {
        reportError(error);
    }
}

void BoatInspection::updateSailHandles()
{
    QVector<VtkSceneWidget::InspectionHandle> handles;
    if (!manualElement_.isEmpty() && scene_->inspectionEditView()) {
        const auto reference = sailReference(manualElement_);
        if (reference) {
            const auto& corners = reference->ecken;
            handles.push_back({QStringLiteral("hals"),
                sailPoint(manualElement_, vector3(corners.hals_m))});
            handles.push_back({QStringLiteral("schothorn"),
                sailPoint(manualElement_, vector3(corners.schothorn_m))});
            handles.push_back({QStringLiteral("kopf_vorliek"),
                sailPoint(manualElement_, vector3(corners.kopf_vorliek_m))});
            if (corners.kopf_achterliek_m)
                handles.push_back({QStringLiteral("kopf_achterliek"),
                    sailPoint(manualElement_, vector3(*corners.kopf_achterliek_m))});
        }
    }
    scene_->setInspectionHandles(handles);
}

void BoatInspection::cancelSailDrag()
{
    const bool hadPreview = !dragElement_.isEmpty() || !previewMeshes_.isEmpty();
    dragElement_.clear();
    dragHandle_.clear();
    dragRequest_.reset();
    dragChanged_ = false;
    previewMeshes_.clear();
    if (hadPreview) {
        ++meshContentGeneration_;
        updateMesh();
    }
    updateSailHandles();
}

void BoatInspection::onSailDrag(const QString& elementId, const QString& handle,
    const QPointF& point, VtkSceneWidget::InspectionDragPhase phase)
{
    if (phase == VtkSceneWidget::InspectionDragPhase::Start) {
        if (manualElement_ != elementId || !activePlane()) return;
        dragElement_ = elementId;
        dragHandle_ = handle;
        dragStartScreen_ = point;
        dragChanged_ = false;
        dragRequest_.reset();
        return;
    }
    if (phase == VtkSceneWidget::InspectionDragPhase::Cancel) {
        cancelSailDrag();
        return;
    }
    if (dragElement_ != elementId) return;
    const auto plane = activePlane();
    if (!plane) {
        cancelSailDrag();
        return;
    }

    segelphysik::SegelAktionAnforderung request{};
    request.element_id = elementId.toStdString();
    request.ebene = *plane;
    if (handle.isEmpty()) {
        request.aktion = segelphysik::SegelAktion::Translation;
        request.translation_m = coreVector(
            scene_->inspectionWorldVectorForScreenDelta(point - dragStartScreen_));
        const auto delta = vector3(request.translation_m);
        dragChanged_ = forcedisplay::norm(delta) > 1e-7;
    } else {
        request.aktion = segelphysik::SegelAktion::Rotation;
        if (handle == QStringLiteral("hals"))
            request.bewegte_ecke = segelphysik::SegelEcke::Hals;
        else if (handle == QStringLiteral("schothorn"))
            request.bewegte_ecke = segelphysik::SegelEcke::Schothorn;
        else if (handle == QStringLiteral("kopf_achterliek"))
            request.bewegte_ecke = segelphysik::SegelEcke::KopfAchterliek;
        else
            request.bewegte_ecke = segelphysik::SegelEcke::KopfVorliek;

        const auto reference = sailReference(elementId);
        if (!reference) {
            diagnostic_->setText(QStringLiteral("Semantische Core-Eckreferenz nicht verfügbar."));
            return;
        }
        const auto pivotLocal = handle == QStringLiteral("hals")
            ? vector3(reference->ecken.kopf_vorliek_m)
            : vector3(reference->ecken.hals_m);
        const auto pivot = scene_->inspectionScreenPointForWorld(sailPoint(elementId, pivotLocal));
        const auto startVector = scene_->inspectionWorldVectorForScreenDelta(dragStartScreen_ - pivot);
        const auto currentVector = scene_->inspectionWorldVectorForScreenDelta(point - pivot);
        const double startLength = forcedisplay::norm(startVector);
        const double currentLength = forcedisplay::norm(currentVector);
        if (startLength < 1e-8 || currentLength < 1e-8) {
            previewMeshes_.remove(elementId);
            diagnostic_->setText(QStringLiteral(
                "Der projizierte Drehhebel ist zu kurz. Bitte eine andere Achsansicht wählen."));
            diagnostic_->setStyleSheet(QStringLiteral("color:#ffbf69;"));
            updateMesh();
            return;
        }
        forcedisplay::Vec3 normal{0.0, 0.0, 0.0};
        if (*plane == segelphysik::SegelEbene::XY) normal[2] = 1.0;
        else if (*plane == segelphysik::SegelEbene::XZ) normal[1] = 1.0;
        else normal[0] = 1.0;
        const auto cross = forcedisplay::cross(startVector, currentVector);
        const double sine = cross[0] * normal[0] + cross[1] * normal[1] + cross[2] * normal[2];
        const double cosine = startVector[0] * currentVector[0]
            + startVector[1] * currentVector[1] + startVector[2] * currentVector[2];
        request.winkel_grad = std::atan2(sine, cosine) * 180.0 / std::acos(-1.0);
        dragChanged_ = std::abs(request.winkel_grad) > 1e-4;
    }
    dragRequest_ = request;
    if (dragChanged_) previewSailAction(request);
    if (phase == VtkSceneWidget::InspectionDragPhase::Finish) {
        if (dragChanged_ && dragRequest_) confirmSailAction(*dragRequest_);
        else {
            previewMeshes_.remove(elementId);
            updateMesh();
        }
        dragElement_.clear();
        dragHandle_.clear();
        dragRequest_.reset();
        dragChanged_ = false;
        updateSailHandles();
    }
}

void BoatInspection::bindCurrentGeometry()
{
    for (QTreeWidgetItemIterator it(tree_); *it; ++it) {
        auto* item = *it;
        if (item->data(0, Qt::UserRole + 4).toString() != QStringLiteral("element")) continue;
        const auto ids = item->data(0, Qt::UserRole + 1).toStringList();
        if (ids.size() != 1) continue;
        for (std::size_t i = 0; i < meshes_.size(); ++i) {
            const auto& mesh = meshes_[i];
            if (text(mesh.element_id) == ids.front() && mesh.type == "darstellung") {
                item->setData(0, Qt::UserRole + 2, static_cast<int>(i));
                break;
            }
        }
    }
}

void BoatInspection::updateMesh()
{
    if (!snapshot_) return;
    QSet<int> requested;
    const auto collect = [&requested](const auto& self, QTreeWidgetItem* item, bool selected) -> void {
        const bool synthetic = item->data(0, Qt::UserRole + 3).toBool();
        const bool include = selected || (synthetic && item->isSelected())
            || item->checkState(0) == Qt::Checked;
        const auto index = item->data(0, Qt::UserRole + 2);
        if (include && index.isValid()) requested.insert(index.toInt());
        for (int i = 0; i < item->childCount(); ++i) self(self, item->child(i), include);
    };
    for (int i = 0; i < tree_->topLevelItemCount(); ++i)
        collect(collect, tree_->topLevelItem(i), false);
    if (requested == activeMeshes_ && scene_->property("hasInspectionMesh").toBool()
        && renderedMeshContentGeneration_ == meshContentGeneration_) return;
    activeMeshes_ = requested;
    const auto camera = scene_->cameraView();
    const bool preserveCamera = scene_->property("hasInspectionMesh").toBool();
    scene_->clearModel();
    scene_->setProperty("hasInspectionMesh", false);
    if (meshes_.empty()) {
        selectionStatus_->setText(QStringLiteral("Keine lesbare Geometrie vorhanden"));
        return;
    }
    try {
        std::vector<segelphysik::InspectionMesh> displayed;
        QSet<QString> currentElements;
        QSet<QString> configuredElements;
        for (auto it = configuredMeshes_.cbegin(); it != configuredMeshes_.cend(); ++it)
            configuredElements.insert(it.key());
        for (auto it = previewMeshes_.cbegin(); it != previewMeshes_.cend(); ++it)
            configuredElements.insert(it.key());
        for (const auto& mesh : meshes_) {
            const auto element = text(mesh.element_id);
            if (mesh.type != "darstellung" || configuredElements.contains(element)) continue;
            if (currentElements.contains(element))
                throw std::runtime_error("Mehrdeutige aktuelle Darstellung fuer Objekt: "
                    + mesh.element_id);
            displayed.push_back(mesh);
            currentElements.insert(element);
        }
        for (const auto& element : configuredElements) {
            const auto selectedPreview = previewMeshes_.constFind(element);
            const auto& configured = selectedPreview != previewMeshes_.cend()
                ? selectedPreview.value() : configuredMeshes_.value(element);
            displayed.insert(displayed.end(), configured.begin(), configured.end());
        }
        QSet<QString> displayedIds;
        for (const auto& mesh : displayed) {
            const auto id = text(mesh.element_id);
            if (id.isEmpty() || displayedIds.contains(id))
                throw std::runtime_error("Mehrdeutige aktuelle Objektgeometrie: " + mesh.element_id);
            displayedIds.insert(id);
        }
        const auto parts = toDisplayParts(displayed);
        QString error;
        if (!scene_->setModel(parts, &error)) throw std::runtime_error(error.toStdString());
        scene_->setProperty("hasInspectionMesh", true);
        renderedMeshContentGeneration_ = meshContentGeneration_;
        if (preserveCamera) scene_->setCameraView(camera);
    } catch (const std::exception& error) {
        selectionStatus_->setText(QStringLiteral("Geometrie nicht verfügbar"));
        reportError(error);
    }
}

void BoatInspection::updateSelection()
{
    if (!snapshot_) return;
    const QSignalBlocker valuesBlock(values_);
    values_->setSortingEnabled(false);
    values_->clear();
    const auto selected = tree_->selectedItems();
    valuesStatus_->setText(selected.isEmpty() ? QStringLiteral("Kein Objekt ausgewählt")
        : QStringLiteral("Auswahl einschließlich aller Unterobjekte"));
    QSet<QString> elements;
    std::vector<std::string> ids;
    const auto collect = [&elements](const auto& self, QTreeWidgetItem* item) -> void {
        for (const auto& id : item->data(0, Qt::UserRole + 1).toStringList())
            elements.insert(id);
        for (int i = 0; i < item->childCount(); ++i) self(self, item->child(i));
    };
    for (auto* item : selected) {
        ids.push_back(item->data(0, Qt::UserRole).toString().toStdString());
        collect(collect, item);
    }
    updateMesh();
    updateMovementPreview();
    scene_->setInspectionSelection(elements, !selected.isEmpty());
    selectionStatus_->clear();
    try {
        if (!selected.isEmpty() && elements.isEmpty())
            selectionStatus_->setText(QStringLiteral("Auswahl enthält keine zugeordnete Geometrie"));
        for (const int index : activeMeshes_) {
            const auto& mesh = meshes_.at(index);
            const auto path = text(mesh.element_id) + QLatin1Char('/') + text(mesh.id);
            const auto uses = meshUses(meshes_, mesh);
            auto* row = new ValueItem(values_, {
                path, QStringLiteral("Geometrie"), QStringLiteral("Geometrieherkunft"),
                text(mesh.provenance), QString(), QStringLiteral("vorhanden")});
            row->setToolTip(3, text(mesh.derivation));
            new ValueItem(values_, {path, QStringLiteral("Geometrie"), QStringLiteral("Quellnetz"),
                text(mesh.source_id), QString(), QStringLiteral("vorhanden")});
            new ValueItem(values_, {path, QStringLiteral("Geometrie"), QStringLiteral("Herleitung / Verwendung"),
                text(mesh.derivation.empty() ? mesh.origin : mesh.derivation),
                QString(), QStringLiteral("vorhanden")});
            new ValueItem(values_, {path, QStringLiteral("Geometrie"), QStringLiteral("Verwendungen"),
                uses.join(QStringLiteral(", ")), QString(), QStringLiteral("vorhanden")});
        }
        for (const auto& id : elements) {
            if (!movements_.contains(id)) continue;
            const auto movement = movements_.value(id);
            auto* normal = new ValueItem(values_, {id + QStringLiteral("/vorschau"),
                QStringLiteral("Bewegung"), QStringLiteral("Normalstellung"),
                number(movement.reference_value), text(movement.value_unit),
                QStringLiteral("vorhanden")});
            normal->setData(0, movementIdRole, id);
            normal->setData(0, movementActionRole, QStringLiteral("normal"));
            normal->setData(3, rawValueRole, movement.reference_value);
            normal->setToolTip(3, QString::number(movement.reference_value, 'g', 17)
                + QLatin1Char(' ') + text(movement.value_unit));
            for (const auto& action : {QStringLiteral("minimum"), QStringLiteral("maximum")}) {
                const auto bound = action == QStringLiteral("minimum") ?
                    movement.minimum_value : movement.maximum_value;
                auto* row = new ValueItem(values_, {id + QStringLiteral("/vorschau"),
                    QStringLiteral("Bewegung"), action == QStringLiteral("minimum") ?
                        QStringLiteral("Vorschau Minimum") : QStringLiteral("Vorschau Maximum"),
                    bound ? number(*bound) : QStringLiteral("–"),
                    text(movement.value_unit),
                    bound ? QStringLiteral("vorhanden") : QStringLiteral("nicht_vorhanden")});
                row->setToolTip(3, bound ? QString::number(*bound, 'g', 17) :
                    QStringLiteral("Keine Grenze im Core definiert; keine Vorschau verfügbar."));
                if (bound) {
                    row->setData(0, movementIdRole, id);
                    row->setData(0, movementActionRole, action);
                    row->setData(3, rawValueRole, *bound);
                }
            }
        }
        QHash<QString, QString> elementByNode;
        for (const auto& node : segelphysik::getObjectTree(*snapshot_, "objekte"))
            if (node.kind == "element" && node.element_ids.size() == 1)
                elementByNode.insert(text(node.id), text(node.element_ids.front()));
        const auto properties = segelphysik::getObjectProperties(*snapshot_, "objekte", ids, true);
        for (const auto& property : properties) {
            const auto value = formatValue(property.value);
            const QString status = text(property.status);
            auto* row = new ValueItem(values_, {
                text(property.object_path), text(property.area), text(property.name),
                status == QStringLiteral("vorhanden") ? value : QStringLiteral("–"),
                text(property.unit), status});
            row->setToolTip(3, value + QLatin1Char(' ') + text(property.unit));
            const auto raw = std::visit([](const auto& item) -> nlohmann::json {
                using T = std::decay_t<decltype(item)>;
                if constexpr (std::is_same_v<T, std::monostate>) return nullptr;
                else if constexpr (std::is_same_v<T, segelphysik::InspectionMatrix>)
                    return nlohmann::json{{"rows", item.rows}, {"columns", item.columns},
                        {"values", item.values}};
                else return item;
            }, property.value);
            if (status == QStringLiteral("vorhanden")) {
                const auto unit = displayUnit(text(property.unit), raw);
                if (!raw.is_string()) row->setText(3, compactJson(raw, unit.second));
                row->setText(4, unit.first);
                if (raw.is_number()) row->setData(3, rawValueRole, raw.get<double>());
                if (text(property.name).contains(QStringLiteral("farbe"), Qt::CaseInsensitive)) {
                    QColor color;
                    if (raw.is_string()) color = QColor(text(raw.get<std::string>()));
                    else if (raw.is_array() && raw.size() == 3 &&
                        std::all_of(raw.begin(), raw.end(), [](const auto& v) {
                            return v.is_number() && v.template get<double>() >= 0. &&
                                v.template get<double>() <= 1.;
                        }))
                        color = QColor::fromRgbF(raw[0].get<double>(),raw[1].get<double>(),raw[2].get<double>());
                    if (color.isValid()) row->setData(3, colorRole, color);
                }
                if (text(property.name).contains(QStringLiteral("drehpunkt"))) {
                    const auto id = elementByNode.value(text(property.node_id));
                    if (movements_.contains(id)) {
                        row->setData(0, movementIdRole, id);
                        row->setData(0, movementActionRole, QStringLiteral("pivot"));
                    }
                }
            }
            if (value.size() > 200) {
                auto* expanded = new ValueItem(row, {QString(), QString(), QString(), row->text(3)});
                expanded->setToolTip(3, value);
                values_->setRootIsDecorated(true);
            }
        }
    } catch (const std::exception& error) {
        reportError(error);
    }
    values_->setSortingEnabled(true);
    filterValues();
    updateMovementPreview();
}

void BoatInspection::filterValues()
{
    const auto apply = [this](const auto& self, QTreeWidgetItem* row) -> bool {
        bool matches = true;
        for (int column = 0; column < 6; ++column)
            matches = matches && row->text(column).contains(filters_[column]->text(), Qt::CaseInsensitive);
        bool childMatches = false;
        for (int i = 0; i < row->childCount(); ++i)
            childMatches = self(self, row->child(i)) || childMatches;
        row->setHidden(!matches && !childMatches);
        return matches || childMatches;
    };
    for (int i = 0; i < values_->topLevelItemCount(); ++i)
        apply(apply, values_->topLevelItem(i));
    updateMovementPreview();
}
