#include "boat_inspection.hpp"

#include "display_geometry.hpp"
#include "vtk_scene.hpp"
#include <vtkPolyData.h>

#include <QHeaderView>
#include <QApplication>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QColor>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPainter>
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
#include <type_traits>
#include <utility>

namespace {
QString text(const std::string& value) { return QString::fromStdString(value); }
constexpr int rawValueRole = Qt::UserRole + 10;
constexpr int colorRole = Qt::UserRole + 11;
constexpr int movementIdRole = Qt::UserRole + 12;
constexpr int movementActionRole = Qt::UserRole + 13;

class FilterHeader final : public QHeaderView {
public:
    explicit FilterHeader(QWidget* parent) : QHeaderView(Qt::Horizontal, parent)
    {
        setObjectName(QStringLiteral("inspectionValueHeader"));
        setStyleSheet(QStringLiteral(
            "QHeaderView::section { border-right:1px solid #795565; }"));
        connect(this, &QHeaderView::sectionResized, this, [this] { positionFilters(); });
        connect(this, &QHeaderView::geometriesChanged, this, [this] { positionFilters(); });
    }

    void setFilters(const std::array<QLineEdit*, 6>& filters)
    {
        filters_ = filters;
        positionFilters();
    }

    QSize sizeHint() const override
    {
        auto size = QHeaderView::sizeHint();
        size.rheight() += filterHeight();
        return size;
    }

protected:
    void paintSection(QPainter* painter, const QRect& rect, int section) const override
    {
        auto title = rect;
        title.setHeight(std::max(0, rect.height() - filterHeight()));
        QHeaderView::paintSection(painter, title, section);
    }

    void resizeEvent(QResizeEvent* event) override
    {
        QHeaderView::resizeEvent(event);
        positionFilters();
    }

    void scrollContentsBy(int dx, int dy) override
    {
        QHeaderView::scrollContentsBy(dx, dy);
        positionFilters();
    }

private:
    int filterHeight() const { return fontMetrics().height() + 14; }
    void positionFilters()
    {
        for (int i = 0; i < static_cast<int>(filters_.size()); ++i)
            if (filters_[i])
                filters_[i]->setGeometry(sectionViewportPosition(i),
                    height() - filterHeight(), sectionSize(i), filterHeight());
    }
    std::array<QLineEdit*, 6> filters_{};
};

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
        "QTreeWidget#inspectionObjectTree { border-radius:6px; padding:3px; }"));
    tree_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tree_->setEditTriggers(QAbstractItemView::NoEditTriggers);
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
}

QWidget* BoatInspection::valuesWidget() const { return valuesPanel_; }

void BoatInspection::setCatalogWidget(QWidget* widget)
{
    layout_->addWidget(widget, 0, 0);
}

void BoatInspection::reportError(const std::exception& error)
{
    diagnostic_->setText(QString::fromUtf8(error.what()));
    diagnostic_->setStyleSheet(QStringLiteral("color:#ffbf69;"));
}

void BoatInspection::clear(const QString& message)
{
    snapshot_.reset();
    const QSignalBlocker treeBlock(tree_);
    tree_->clear();
    meshes_.clear();
    activeMeshes_.clear();
    movements_.clear();
    values_->clear();
    valuesStatus_->setText(QStringLiteral("Kein Objekt ausgewählt"));
    diagnostic_->setText(message);
    selectionStatus_->clear();
    scene_->clearModel();
    scene_->setProperty("hasInspectionMesh", false);
}

void BoatInspection::setSnapshot(std::shared_ptr<const segelphysik::ModelInspection> snapshot)
{
    clear(QString());
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
    } catch (const std::exception& error) {
        reportError(error);
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
        for (const auto& node : nodes) {
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
        appendGeometryNodes();
        for (const auto& node : nodes) {
            if (node.kind != "element" || node.element_ids.size() != 1) continue;
            const auto movement = segelphysik::getMovementPreview(*snapshot_, node.element_ids.front());
            if (!movement) continue;
            const auto id = text(node.element_ids.front());
            movements_.insert(id, *movement);
            auto* item = new QTreeWidgetItem(items.value(text(node.id)),
                {QStringLiteral("Bewegungsvorschau · Achse / Normal / Extreme")});
            item->setData(0, Qt::UserRole, text(node.id));
            item->setData(0, Qt::UserRole + 1, QStringList{id});
            item->setData(0, Qt::UserRole + 3, true);
            item->setData(0, Qt::UserRole + 5, id);
            item->setCheckState(0, Qt::Unchecked);
            QStringList description;
            description << QStringLiteral("Reine Vorschau – verändert weder Quelle noch Simulation.")
                << QStringLiteral("Normal: Cyan · Minimum: Orange · Maximum: Violett · Achse: Gelb");
            if (!movement->minimum_value) description << QStringLiteral("Minimum: nicht vorhanden");
            if (!movement->maximum_value) description << QStringLiteral("Maximum: nicht vorhanden");
            item->setToolTip(0, description.join(QLatin1Char('\n')));
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
        for (QTreeWidgetItemIterator it(tree_); *it; ++it) {
            const auto id = (*it)->data(0, Qt::UserRole + 5).toString();
            if (!id.isEmpty() && ((*it)->isSelected() || (*it)->checkState(0) == Qt::Checked))
                    requested[id].insert(QStringLiteral("all"));
        }
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

void BoatInspection::appendGeometryNodes()
{
    QHash<QString, QTreeWidgetItem*> elements;
    for (QTreeWidgetItemIterator it(tree_); *it; ++it) {
        if ((*it)->data(0, Qt::UserRole + 4).toString() != QStringLiteral("element")) continue;
        const auto ids = (*it)->data(0, Qt::UserRole + 1).toStringList();
        if (ids.size() == 1) elements.insert(ids.front(), *it);
    }
    QHash<QString, QTreeWidgetItem*> derivedRoots;
    QSet<QString> sources;
    for (std::size_t i = 0; i < meshes_.size(); ++i) {
        const auto& mesh = meshes_[i];
        const bool derived = mesh.provenance == "abgeleitet";
        auto* parent = elements.value(text(mesh.element_id));
        if (!parent) continue;
        const auto identity = text(mesh.element_id) + QLatin1Char('|') + text(mesh.source_id);
        if (!derived && !mesh.source_id.empty()) {
            if (sources.contains(identity)) continue;
            sources.insert(identity);
        }
        if (derived) {
            auto*& root = derivedRoots[text(mesh.element_id)];
            if (!root) {
                root = new QTreeWidgetItem(parent, {QStringLiteral("Abgeleitete Daten")});
                root->setData(0, Qt::UserRole + 3, true);
                root->setData(0, Qt::UserRole, parent->data(0, Qt::UserRole));
            }
            parent = root;
        }
        auto* item = new QTreeWidgetItem(parent);
        item->setText(0, derived ? text(mesh.type)
            : QStringLiteral("Quellmesh · %1").arg(text(mesh.source_id)));
        item->setData(0, Qt::UserRole, elements.value(text(mesh.element_id))->data(0, Qt::UserRole));
        item->setData(0, Qt::UserRole + 1, QStringList{text(mesh.element_id)});
        item->setData(0, Qt::UserRole + 2, static_cast<int>(i));
        item->setData(0, Qt::UserRole + 3, true);
        item->setCheckState(0, Qt::Unchecked);
        const auto uses = meshUses(meshes_, mesh);
        item->setToolTip(0, QStringLiteral("%1\nQuelle: %2\n%3\nVerwendungen: %4")
            .arg(text(mesh.origin), text(mesh.source_id), text(mesh.derivation),
                uses.join(QStringLiteral(", "))));
        if (derived) item->setForeground(0, QColor(QStringLiteral("#e9b64d")));
    }
    for (auto it = elements.begin(); it != elements.end(); ++it) {
        QSet<QString> unique;
        for (const auto& mesh : meshes_)
            if (text(mesh.element_id) == it.key() && mesh.provenance == "quelle")
                unique.insert(text(mesh.source_id));
        if (unique.size() > 1) {
            it.value()->setText(0, it.value()->text(0) + QStringLiteral(" [mehrere Bestandsquellen]"));
            it.value()->setToolTip(0, QStringLiteral(
                "%1\n%2 Quellreferenzen im Bestandsformat. Noch kein Ein-Quellmesh-Modell; "
                "die fachliche Zusammenführung erfolgt ausschließlich im Core.")
                .arg(it.value()->data(0, Qt::UserRole).toString()).arg(unique.size()));
            it.value()->setForeground(0, QColor(QStringLiteral("#e9b64d")));
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
    if (requested == activeMeshes_ && scene_->property("hasInspectionMesh").toBool()) return;
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
        QSet<QString> replaced;
        for (const int index : requested) {
            if (meshes_.at(index).provenance != "abgeleitet")
                replaced.insert(text(meshes_.at(index).element_id));
        }
        QSet<QString> baseElements;
        for (const auto& mesh : meshes_) {
            if (mesh.type == "darstellung" && !replaced.contains(text(mesh.element_id))) {
                displayed.push_back(mesh);
                baseElements.insert(text(mesh.element_id));
            }
        }
        for (const auto& mesh : meshes_) {
            const auto element = text(mesh.element_id);
            if (mesh.provenance == "quelle" && !baseElements.contains(element)
                && !replaced.contains(element)) {
                displayed.push_back(mesh);
                baseElements.insert(element);
            }
        }
        for (const int index : requested) {
            auto mesh = meshes_.at(index);
            if (mesh.provenance == "abgeleitet")
                mesh.color_rgb = std::array<double, 3>{0.91, 0.71, 0.30};
            if (mesh.surface && *mesh.surface == "unsichtbar")
                mesh.surface = "drahtgitter";
            displayed.push_back(std::move(mesh));
        }
        const auto parts = toDisplayParts(displayed);
        QString error;
        if (!scene_->setModel(parts, &error)) throw std::runtime_error(error.toStdString());
        scene_->setProperty("hasInspectionMesh", true);
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
