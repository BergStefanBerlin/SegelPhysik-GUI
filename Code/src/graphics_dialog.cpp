#include "graphics_dialog.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>
#include <algorithm>
#include <memory>

namespace {
class SettingsTable : public QTableWidget {
public:
    SettingsTable() : QTableWidget(0, 4)
    {
        connect(horizontalHeader(), &QHeaderView::sectionResized, this, [this] { adjustRows(); });
    }

    void adjustRows()
    {
        for (int row = 0; row < rowCount(); ++row) {
            int height = 32;
            for (int column = 0; column < columnCount(); ++column) {
                if (auto* widget = cellWidget(row, column)) {
                    const int desired = widget->hasHeightForWidth()
                        ? widget->heightForWidth(columnWidth(column)) : widget->sizeHint().height();
                    height = std::max(height, desired + 8);
                }
            }
            setRowHeight(row, height);
        }
    }

protected:
    void resizeEvent(QResizeEvent* event) override
    {
        QTableWidget::resizeEvent(event);
        adjustRows();
    }
};
}

QWidget* createGraphicsSettingsPage(QWidget* parent, const graphics::Settings& initial,
    const QString& status, const std::function<bool(const graphics::Settings&)>& commit,
    const std::function<void()>& close,
    const std::function<QString(graphics::Feature)>& featureStatus)
{
    auto* page = new QWidget(parent);
    page->setObjectName(QStringLiteral("graphicsSettingsPage"));
    auto* layout = new QVBoxLayout(page);
    auto* heading = new QLabel(QStringLiteral(
        "<b>Darstellung – Frei im Statischen Modell (Technik)</b><br>"
        "Jede Änderung wird sofort auf Frei angewendet und dauerhaft gespeichert. "
        "Seite, Heck und Draufsicht bleiben fest schematisch und sind nicht konfigurierbar. "
        "Dynamisches Modell und bisherige Optikwerte bleiben unverändert. "
        "CPU-/GPU-Last sind qualitative Einschätzungen, keine Messwerte; "
        "sie betreffen die Darstellung, nicht die Solvergenauigkeit.<br>"
        "<b>Haken = gespeicherte und angewendete Einstellung.</b> "
        "Körperdurchsicht macht das Boot transparent. Materialeffekte gelten für passende Teile; "
        "andere Teile lassen sie ohne Fehler aus."));
    heading->setWordWrap(true);
    layout->addWidget(heading);
    auto* table = new SettingsTable;
    table->setObjectName(QStringLiteral("staticGraphicsTable"));
    table->setHorizontalHeaderLabels({QString(), QStringLiteral("Beschreibung"),
        QStringLiteral("CPU Last"), QStringLiteral("GPU Last")});
    table->verticalHeader()->hide();
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setWordWrap(true);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    table->setColumnWidth(0, 125);
    table->setColumnWidth(2, 95);
    table->setColumnWidth(3, 95);
    layout->addWidget(table, 1);
    auto* diagnostic = new QLabel(status);
    diagnostic->setObjectName(QStringLiteral("graphicsDiagnostic"));
    diagnostic->setWordWrap(true);
    diagnostic->setMaximumHeight(70);
    layout->addWidget(diagnostic);
    auto saved = std::make_shared<graphics::Settings>(initial);
    auto descriptions = std::make_shared<std::array<QLabel*, graphics::featureCount>>();
    auto baseDescriptions = std::make_shared<std::array<QString, graphics::featureCount>>();
    auto refresh = [saved, descriptions, baseDescriptions, featureStatus, table] {
        for (std::size_t i = 0; i < graphics::featureCount; ++i) {
            if (!(*descriptions)[i]) continue;
            const auto state = featureStatus ? featureStatus(static_cast<graphics::Feature>(i))
                : saved->profiles[0].enabled[i] ? QStringLiteral("Eingeschaltet.")
                                               : QStringLiteral("Ausgeschaltet.");
            (*descriptions)[i]->setText((*baseDescriptions)[i] + QStringLiteral("\nStatus: ") + state);
        }
        table->adjustRows();
    };
    const auto change = [saved, commit, diagnostic, refresh](const std::function<void(graphics::Profile&)>& edit) {
        auto next = *saved;
        edit(next.profiles[0]);
        QString error;
        if (!graphics::validate(next, &error)) {
            diagnostic->setText(error);
            return false;
        }
        if (!commit(next)) {
            diagnostic->setText(QStringLiteral(
                "Speichern fehlgeschlagen. Die Änderung wurde nicht angewendet; "
                "der letzte gespeicherte Wert bleibt aktiv."));
            return false;
        }
        *saved = next;
        refresh();
        diagnostic->setText(QStringLiteral("Gespeichert und im statischen Modell angewendet."));
        return true;
    };
    auto addRow = [table](QWidget* control, const QString& description,
                      const QString& cpu, const QString& gpu) {
        const int row = table->rowCount();
        table->insertRow(row);
        if (control) {
            auto* cell = new QWidget;
            auto* box = new QHBoxLayout(cell);
            box->setContentsMargins(5, 5, 5, 5);
            box->setAlignment(Qt::AlignCenter);
            box->addWidget(control);
            table->setCellWidget(row, 0, cell);
        }
        auto* text = new QLabel(description);
        text->setWordWrap(true);
        text->setTextFormat(Qt::PlainText);
        text->setMargin(5);
        table->setCellWidget(row, 1, text);
        table->setItem(row, 2, new QTableWidgetItem(cpu));
        table->setItem(row, 3, new QTableWidgetItem(gpu));
        for (int column : {2, 3}) {
            auto* cost = new QLabel(table->item(row, column)->text());
            cost->setWordWrap(true);
            cost->setMargin(5);
            table->setCellWidget(row, column, cost);
        }
    };
    auto checks = std::make_shared<std::array<QCheckBox*, graphics::featureCount>>();
    auto dependencies = [checks] {
        using F = graphics::Feature;
        const auto on = [checks](F f) { return (*checks)[static_cast<std::size_t>(f)]->isChecked(); };
        for (std::size_t i = 0; i < graphics::featureCount; ++i) {
            const auto f = static_cast<F>(i);
            QStringList reasons;
            if (f >= F::Fresnel && f <= F::Reflection && !on(F::Water))
                reasons << QStringLiteral("Wasser einschalten");
            if ((f == F::ModelIBL || f == F::Coat || f == F::ColorTexture || f == F::ORM
                    || f == F::NormalTexture || f == F::Anisotropy) && !on(F::PBR))
                reasons << QStringLiteral("PBR einschalten");
            if (f == F::Animation && !on(F::WaterNormals))
                reasons << QStringLiteral("Wasser-Normalstruktur einschalten");
            if (f == F::DistanceMarks && !on(F::WaterTrack))
                reasons << QStringLiteral("Technische Fahrspur einschalten");
            (*checks)[i]->setEnabled(reasons.isEmpty());
            (*checks)[i]->setToolTip(reasons.join(QStringLiteral("; ")));
        }
    };
    for (std::size_t i = 0; i < graphics::featureCount; ++i) {
        const auto& info = graphics::features()[i];
        auto* check = new QCheckBox;
        check->setObjectName(QStringLiteral("profile0_%1").arg(QString::fromLatin1(info.key)));
        check->setAccessibleName(QString::fromUtf8(info.label));
        check->setChecked(initial.profiles[0].enabled[i]);
        (*checks)[i] = check;
        const QString explanation = QString::fromUtf8(info.explanation);
        const auto cost = [&explanation](const QString& processor) {
            const QRegularExpression expression(processor + QStringLiteral("(?:/Speicher)? ([^,;.]+)"));
            return expression.match(explanation).captured(1);
        };
        addRow(check, QString::fromUtf8(info.group) + QStringLiteral(" – ")
            + QString::fromUtf8(info.label) + QStringLiteral("\n")
            + explanation.left(explanation.indexOf(QStringLiteral("CPU"))).trimmed(),
            cost(QStringLiteral("CPU")), cost(QStringLiteral("GPU")));
        auto* description = qobject_cast<QLabel*>(table->cellWidget(static_cast<int>(i), 1));
        description->setObjectName(QStringLiteral("profile0_status_%1").arg(QString::fromLatin1(info.key)));
        (*descriptions)[i] = description;
        (*baseDescriptions)[i] = description->text();
        QObject::connect(check, &QCheckBox::toggled, page, [change, dependencies, check, i](bool value) {
            if (!change([i, value](graphics::Profile& p) { p.enabled[i] = value; })) {
                const QSignalBlocker blocker(check);
                check->setChecked(!value);
            }
            dependencies();
        });
    }
    dependencies();
    refresh();
    const auto numeric = [&](const char* key, const QString& description, double graphics::Profile::* member,
                             double minimum, double maximum, double step, const QString& cpu, const QString& gpu) {
        if (member == &graphics::Profile::trackAge || member == &graphics::Profile::trackSpacing
            || member == &graphics::Profile::markSpacing) {
            auto* field = new QLineEdit(QString::number(initial.profiles[0].*member, 'g', 17));
            field->setObjectName(QStringLiteral("profile0_%1").arg(QString::fromLatin1(key)));
            addRow(nullptr, description, cpu, gpu);
            table->setCellWidget(table->rowCount() - 1, 0, field);
            QObject::connect(field, &QLineEdit::editingFinished, page,
                [saved, change, diagnostic, member, field] {
                    bool ok = false;
                    const double value = field->text().trimmed()
                        .replace(QLatin1Char(','), QLatin1Char('.')).toDouble(&ok);
                    if (!ok) diagnostic->setText(QStringLiteral("Bitte einen gültigen Zahlenwert eingeben."));
                    if (!ok || !change([member, value](graphics::Profile& p) { p.*member = value; }))
                        field->setText(QString::number(saved->profiles[0].*member, 'g', 17));
                });
            return;
        }
        auto* field = new QDoubleSpinBox;
        field->setObjectName(QStringLiteral("profile0_%1").arg(QString::fromLatin1(key)));
        field->setRange(minimum, maximum);
        field->setDecimals(3);
        field->setSingleStep(step);
        field->setKeyboardTracking(false);
        field->setValue(initial.profiles[0].*member);
        addRow(nullptr, description, cpu, gpu);
        table->setCellWidget(table->rowCount() - 1, 0, field);
        QObject::connect(field, qOverload<double>(&QDoubleSpinBox::valueChanged), page,
            [saved, change, member, field](double value) {
                const double previous = saved->profiles[0].*member;
                if (!change([member, value](graphics::Profile& p) { p.*member = value; })) {
                    const QSignalBlocker blocker(field);
                    field->setValue(previous);
                }
            });
    };
    numeric("bodyOpacity", QStringLiteral("Körperdeckkraft: 0 unsichtbar, 1 deckend. Wirkt bei analytischer Körperdurchsicht."),
        &graphics::Profile::bodyOpacity, 0, 1, .05, QStringLiteral("niedrig"), QStringLiteral("mittel bis hoch"));
    numeric("waterOpacity", QStringLiteral("Wasserdeckkraft: 0 unsichtbar, 1 deckend. Transparenz benötigt zusätzliche GPU-Durchläufe."),
        &graphics::Profile::waterOpacity, 0, 1, .05, QStringLiteral("niedrig"), QStringLiteral("mittel bis hoch"));
    numeric("normalScale", QStringLiteral("Normalstärke: visueller Welleneindruck, keine Wellenkräfte."),
        &graphics::Profile::normalScale, 0, 1, .01, QStringLiteral("niedrig"), QStringLiteral("niedrig"));
    numeric("waterPeriod", QStringLiteral("Wasserstruktur-Periode [m]: grafische Strukturgröße, keine physikalische Welle."),
        &graphics::Profile::waterPeriod, .02, 20, .05, QStringLiteral("niedrig"), QStringLiteral("niedrig"));
    numeric("trackAge", QStringLiteral("Spuralter [s]: längere Spuren benötigen mehr Geometrie und Speicher."),
        &graphics::Profile::trackAge, .001, 86400, 1, QStringLiteral("mit Länge steigend"), QStringLiteral("mittel"));
    numeric("trackSpacing", QStringLiteral("Räumliche Spurabtastung [m]: größere Abstände sparen Geometrie und Speicher."),
        &graphics::Profile::trackSpacing, .001, 10000, .05, QStringLiteral("bei Dichte steigend"), QStringLiteral("mittel"));
    numeric("markSpacing", QStringLiteral("Abstand der Distanzmarken [m]: kleinere Abstände benötigen mehr Geometrie."),
        &graphics::Profile::markSpacing, .001, 10000, .5, QStringLiteral("bei Dichte steigend"), QStringLiteral("mittel"));
    auto* budget = new QSpinBox;
    budget->setObjectName(QStringLiteral("profile0_trackBudget"));
    budget->setRange(2, 65536);
    budget->setKeyboardTracking(false);
    budget->setValue(initial.profiles[0].trackBudget);
    addRow(nullptr, QStringLiteral("Maximale Stützpunkte je Spur: begrenzt Geometrie und Speicher."),
        QStringLiteral("mit Budget steigend"), QStringLiteral("mit Budget steigend"));
    table->setCellWidget(table->rowCount() - 1, 0, budget);
    QObject::connect(budget, qOverload<int>(&QSpinBox::valueChanged), page, [saved, change, budget](int value) {
        const int previous = saved->profiles[0].trackBudget;
        if (!change([value](graphics::Profile& p) { p.trackBudget = value; })) {
            const QSignalBlocker blocker(budget);
            budget->setValue(previous);
        }
    });
    auto* quality = new QComboBox;
    quality->setObjectName(QStringLiteral("profile0_quality"));
    quality->addItems({QStringLiteral("Niedrig"), QStringLiteral("Ausgewogen"), QStringLiteral("Hoch")});
    quality->setCurrentIndex(initial.profiles[0].quality);
    addRow(nullptr, QStringLiteral("Effektqualität: Schattenkarten 512/1024/2048 Pixel, "
        "4/8/16 Transparenzschichten, 24/48/64 SSAO-Stichproben. Höhere Stufen brauchen mehr Speicher."),
        QStringLiteral("niedrig"), QStringLiteral("mit Qualität steigend"));
    table->setCellWidget(table->rowCount() - 1, 0, quality);
    QObject::connect(quality, qOverload<int>(&QComboBox::currentIndexChanged), page,
        [saved, change, quality](int value) {
            const int previous = saved->profiles[0].quality;
            if (!change([value](graphics::Profile& p) { p.quality = value; })) {
                const QSignalBlocker blocker(quality);
                quality->setCurrentIndex(previous);
            }
        });
    auto* environment = new QLineEdit(initial.profiles[0].environmentPath);
    environment->setObjectName(QStringLiteral("profile0_environment"));
    environment->setPlaceholderText(QStringLiteral("Leer = prozeduraler Himmel"));
    addRow(nullptr, QStringLiteral("HDR-Umgebung: Pfad zu eigener Radiance-HDR-Datei; leer nutzt den prozeduralen Himmel. "
        "Laden und Speicherbedarf hängen von der Dateigröße ab."),
        QStringLiteral("mittel beim Laden"), QStringLiteral("mittel"));
    table->setCellWidget(table->rowCount() - 1, 0, environment);
    QObject::connect(environment, &QLineEdit::editingFinished, page, [saved, change, environment] {
        const QString previous = saved->profiles[0].environmentPath;
        const QString value = environment->text().trimmed();
        if (value == previous) return;
        if (!change([value](graphics::Profile& p) { p.environmentPath = value; }))
            environment->setText(previous);
    });
    table->horizontalHeader()->setMinimumSectionSize(40);
    table->adjustRows();
    auto* back = new QPushButton(QStringLiteral("Zurück zum Boot"));
    back->setObjectName(QStringLiteral("closeGraphics"));
    layout->addWidget(back, 0, Qt::AlignRight);
    QObject::connect(back, &QPushButton::clicked, page, close);
    return page;
}
