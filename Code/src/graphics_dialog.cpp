#include "graphics_dialog.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>
#include <limits>

namespace {
struct Controls {
    std::array<QCheckBox*, graphics::featureCount> features{};
    QCheckBox* motionGrid = nullptr;
    QDoubleSpinBox* body = nullptr;
    QDoubleSpinBox* water = nullptr;
    QDoubleSpinBox* normal = nullptr;
    QDoubleSpinBox* period = nullptr;
    QComboBox* quality = nullptr;
    QCheckBox* through = nullptr;
    QCheckBox* liveReduction = nullptr;
    QLineEdit* environment = nullptr;
    QLineEdit* trackAge = nullptr;
    QLineEdit* trackSpacing = nullptr;
    QLineEdit* markSpacing = nullptr;
    QSpinBox* trackBudget = nullptr;

    void set(const graphics::Profile& p) const
    {
        for (std::size_t i = 0; i < features.size(); ++i) features[i]->setChecked(p.enabled[i]);
        motionGrid->setChecked(p.on(graphics::Feature::Grid));
        body->setValue(p.bodyOpacity);
        water->setValue(p.waterOpacity);
        normal->setValue(p.normalScale);
        period->setValue(p.waterPeriod);
        quality->setCurrentIndex(p.quality);
        through->setChecked(p.throughBody);
        liveReduction->setChecked(p.liveReduction);
        environment->setText(p.environmentPath);
        trackAge->setText(QString::number(p.trackAge, 'g', 17));
        trackSpacing->setText(QString::number(p.trackSpacing, 'g', 17));
        markSpacing->setText(QString::number(p.markSpacing, 'g', 17));
        trackBudget->setValue(p.trackBudget);
    }
    graphics::Profile get() const
    {
        graphics::Profile p;
        for (std::size_t i = 0; i < features.size(); ++i) p.enabled[i] = features[i]->isChecked();
        p.bodyOpacity = body->value();
        p.waterOpacity = water->value();
        p.normalScale = normal->value();
        p.waterPeriod = period->value();
        p.quality = quality->currentIndex();
        p.throughBody = through->isChecked();
        p.liveReduction = liveReduction->isChecked();
        p.environmentPath = environment->text().trimmed();
        auto positiveNumber = [](const QLineEdit* field) {
            bool ok = false;
            const double value = field->text().trimmed().replace(QLatin1Char(','), QLatin1Char('.')).toDouble(&ok);
            return ok ? value : std::numeric_limits<double>::quiet_NaN();
        };
        p.trackAge = positiveNumber(trackAge);
        p.trackSpacing = positiveNumber(trackSpacing);
        p.markSpacing = positiveNumber(markSpacing);
        p.trackBudget = trackBudget->value();
        return p;
    }
};
}

void editGraphicsSettings(QWidget* parent, const graphics::Settings& initial,
    const QString& status, const std::function<bool(const graphics::Settings&)>& commit,
    const std::function<QString()>& currentStatus)
{
    QDialog dialog(parent);
    dialog.setObjectName(QStringLiteral("graphicsSetup"));
    dialog.setWindowTitle(QStringLiteral("Setup – Grafik"));
    dialog.resize(640, 760);
    auto* layout = new QVBoxLayout(&dialog);
    auto* explanation = new QLabel(QStringLiteral(
        "Beide Profile werden getrennt gespeichert. Fahrteffekte sind rein grafisch.\n"
        "Nicht anwendbare Materialeffekte erzeugen keine fehlenden Modellteile.\n"
        "Materialtexturen benoetigen PBR; Wasser und sichtbarer Himmel sind separat schaltbar."));
    explanation->setWordWrap(true);
    layout->addWidget(explanation);
    auto* tabs = new QTabWidget;
    layout->addWidget(tabs, 1);
    std::array<Controls, 2> controls;
    for (int n = 0; n < 2; ++n) {
        auto& c = controls[static_cast<std::size_t>(n)];
        auto* page = new QWidget;
        auto* column = new QVBoxLayout(page);
        QString previous;
        QVBoxLayout* groupLayout = nullptr;
        for (std::size_t i = 0; i < graphics::featureCount; ++i) {
            const auto& info = graphics::features()[i];
            const QString group = QString::fromUtf8(info.group);
            if (group != previous) {
                auto* box = new QGroupBox(group);
                groupLayout = new QVBoxLayout(box);
                column->addWidget(box);
                previous = group;
            }
            auto* check = new QCheckBox(static_cast<graphics::Feature>(i) == graphics::Feature::Grid
                    ? QStringLiteral("Wasserraster (weltfest, Bewegung durchs Wasser)")
                    : QString::fromUtf8(info.label));
            check->setObjectName(QStringLiteral("profile%1_%2").arg(n).arg(QString::fromLatin1(info.key)));
            check->setToolTip(QStringLiteral("Unabhaengiger Profilwunsch. Effektive Verfuegbarkeit wird in der Grafikdiagnose angezeigt."));
            groupLayout->addWidget(check);
            c.features[i] = check;
        }
        c.motionGrid = new QCheckBox(QStringLiteral("Wasserraster (derselbe Wert wie Bezugsraster)"));
        c.motionGrid->setObjectName(QStringLiteral("profile%1_motionGrid").arg(n));
        groupLayout->addWidget(c.motionGrid);
        auto* grid = c.features[static_cast<std::size_t>(graphics::Feature::Grid)];
        QObject::connect(grid, &QCheckBox::toggled, c.motionGrid, &QCheckBox::setChecked);
        QObject::connect(c.motionGrid, &QCheckBox::toggled, grid, &QCheckBox::setChecked);
        auto updateDependencies = [&c] {
            using F = graphics::Feature;
            auto requested = [&c](F feature) {
                return c.features[static_cast<std::size_t>(feature)]->isChecked();
            };
            for (std::size_t i = 0; i < graphics::featureCount; ++i) {
                const auto feature = static_cast<F>(i);
                QStringList reasons;
                if (feature >= F::Fresnel && feature <= F::Reflection && !requested(F::Water)) {
                    reasons << QStringLiteral("Wasser einschalten");
                }
                if ((feature == F::ModelIBL || feature == F::Coat || feature == F::ColorTexture || feature == F::ORM
                        || feature == F::NormalTexture || feature == F::Anisotropy)
                    && !requested(F::PBR)) {
                    reasons << QStringLiteral("PBR einschalten");
                }
                if (feature == F::Animation && !requested(F::WaterNormals)) {
                    reasons << QStringLiteral("Wasser-Normalstruktur einschalten");
                }
                if (feature == F::DistanceMarks && !requested(F::WaterTrack)) {
                    reasons << QStringLiteral("Technische Fahrspur einschalten");
                }
                auto* check = c.features[i];
                check->setEnabled(reasons.isEmpty());
                check->setToolTip(reasons.isEmpty()
                    ? QStringLiteral("Unabhaengiger Profilwunsch. Modell-/Hardwaregrenzen stehen in der Grafikdiagnose.")
                    : QStringLiteral("Zur Wirksamkeit erforderlich: %1. Der gespeicherte Wunsch bleibt erhalten.")
                          .arg(reasons.join(QStringLiteral("; "))));
            }
        };
        for (auto* check : c.features) {
            QObject::connect(check, &QCheckBox::toggled, &dialog, updateDependencies);
        }
        auto* parameters = new QFormLayout;
        auto spin = [parameters](const QString& name) {
            auto* field = new QDoubleSpinBox;
            field->setRange(0, 1);
            field->setSingleStep(0.05);
            parameters->addRow(name, field);
            return field;
        };
        c.body = spin(QStringLiteral("Koerperdeckkraft"));
        c.water = spin(QStringLiteral("Wasserdeckkraft"));
        c.normal = spin(QStringLiteral("Normalstaerke"));
        c.period = spin(QStringLiteral("Wasserstruktur-Periode"));
        c.period->setRange(0.02, 20.0);
        c.period->setSuffix(QStringLiteral(" m"));
        auto trackParameter = [&](const QString& name, const char* key, double maximum, const QString& unit) {
            auto* field = new QLineEdit;
            parameters->addRow(name + QStringLiteral(" [") + unit.trimmed() + QStringLiteral("]"), field);
            field->setObjectName(QStringLiteral("profile%1_%2").arg(n).arg(QString::fromLatin1(key)));
            field->setToolTip(QStringLiteral("Endlicher Wert > 0 bis %1. Punkt oder Komma; keine stille Begrenzung.")
                .arg(maximum));
            return field;
        };
        c.trackAge = trackParameter(QStringLiteral("Spuralter"), "trackAge", 86400, QStringLiteral(" s"));
        c.trackSpacing = trackParameter(QStringLiteral("Raeumliche Spurabtastung"), "trackSpacing", 10000, QStringLiteral(" m"));
        c.markSpacing = trackParameter(QStringLiteral("Abstand Distanzmarken"), "markSpacing", 10000, QStringLiteral(" m"));
        c.trackBudget = new QSpinBox;
        c.trackBudget->setObjectName(QStringLiteral("profile%1_trackBudget").arg(n));
        c.trackBudget->setRange(2, 65536);
        parameters->addRow(QStringLiteral("Maximale Stuetzpunkte je Spur"), c.trackBudget);
        c.quality = new QComboBox;
        c.quality->addItems({QStringLiteral("Niedrig"), QStringLiteral("Ausgewogen"), QStringLiteral("Hoch")});
        parameters->addRow(QStringLiteral("Effektqualitaet"), c.quality);
        c.through = new QCheckBox(QStringLiteral("Vektoren durch Koerper sichtbar"));
        parameters->addRow(c.through);
        c.liveReduction = new QCheckBox(QStringLiteral("Live-Lauf: teure Effekte reduzieren"));
        c.liveReduction->setToolTip(QStringLiteral(
            "Waehrend eines laufenden Core-Laufs werden SSAO, Schatten und Bootsspiegelung "
            "voruebergehend ausgeschaltet. Gespeicherte Einzelwuensche bleiben unveraendert."));
        parameters->addRow(c.liveReduction);
        auto* environmentRow = new QHBoxLayout;
        c.environment = new QLineEdit;
        c.environment->setObjectName(QStringLiteral("profile%1_environment").arg(n));
        c.environment->setPlaceholderText(QStringLiteral("Prozedural (keine HDR-Datei)"));
        c.environment->setToolTip(QStringLiteral(
            "Leer = prozeduraler Himmel. Eine eigene HDR-Datei ersetzt die Umgebung; "
            "fehlende oder unlesbare Dateien werden sichtbar gemeldet."));
        auto* browse = new QPushButton(QStringLiteral("HDR …"));
        environmentRow->addWidget(c.environment, 1);
        environmentRow->addWidget(browse);
        parameters->addRow(QStringLiteral("Umgebung"), environmentRow);
        QObject::connect(browse, &QPushButton::clicked, &dialog, [&dialog, &c] {
            const auto path = QFileDialog::getOpenFileName(&dialog,
                QStringLiteral("HDR-Umgebung auswaehlen"), c.environment->text(),
                QStringLiteral("Radiance HDR (*.hdr);;Alle Dateien (*)"));
            if (!path.isEmpty()) c.environment->setText(path);
        });
        column->addLayout(parameters);
        auto* actions = new QHBoxLayout;
        auto* all = new QPushButton(QStringLiteral("Alle ein"));
        auto* none = new QPushButton(QStringLiteral("Alle aus"));
        auto* reset = new QPushButton(QStringLiteral("Profilstandard"));
        all->setObjectName(QStringLiteral("profile%1_all").arg(n));
        none->setObjectName(QStringLiteral("profile%1_none").arg(n));
        reset->setObjectName(QStringLiteral("profile%1_reset").arg(n));
        actions->addWidget(all);
        actions->addWidget(none);
        actions->addWidget(reset);
        column->addLayout(actions);
        QObject::connect(all, &QPushButton::clicked, &dialog, [&c] {
            for (auto* check : c.features) check->setChecked(true);
        });
        QObject::connect(none, &QPushButton::clicked, &dialog, [&c] {
            for (auto* check : c.features) check->setChecked(false);
        });
        QObject::connect(reset, &QPushButton::clicked, &dialog, [&c, n] { c.set(graphics::defaults(n)); });
        c.set(initial.profiles[static_cast<std::size_t>(n)]);
        updateDependencies();
        auto* scroll = new QScrollArea;
        scroll->setWidgetResizable(true);
        scroll->setWidget(page);
        tabs->addTab(scroll, n == 0 ? QStringLiteral("Technik") : QStringLiteral("Optik"));
    }
    tabs->setCurrentIndex(initial.active);
    auto* diagnostic = new QLabel(QStringLiteral("Aktive Szene (nicht Dialogentwurf):\n") + status);
    diagnostic->setWordWrap(true);
    diagnostic->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(diagnostic);
    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);
    auto apply = [&] {
        graphics::Settings result = initial;
        for (std::size_t i = 0; i < controls.size(); ++i) result.profiles[i] = controls[i].get();
        QString error;
        if (!graphics::validate(result, &error)) {
            diagnostic->setText(error);
            return false;
        }
        const bool saved = commit(result);
        if (saved && currentStatus) {
            diagnostic->setText(QStringLiteral("Aktive Szene (nicht Dialogentwurf):\n") + currentStatus());
        }
        return saved;
    };
    QObject::connect(buttons->button(QDialogButtonBox::Apply), &QPushButton::clicked,
        &dialog, [&] { apply(); });
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        if (apply()) dialog.accept();
    });
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    dialog.exec();
}
