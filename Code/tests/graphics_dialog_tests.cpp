#include "graphics_dialog.hpp"

#include <QApplication>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QTableWidget>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QVBoxLayout>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}
}

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    try {
        QTemporaryDir directory;
        require(directory.isValid(), "temporary directory");
        QSettings storage(directory.filePath(QStringLiteral("existing.ini")), QSettings::IniFormat);
        auto committed = graphics::defaults();
        committed.active = 1;
        QString error;
        require(graphics::save(storage, committed, &error), "initial existing settings");
        const auto dynamicData = storage.value(QStringLiteral("grafik/profile_v1"));
        storage.beginGroup(QStringLiteral("statisch"));
        int commits = 0, closed = 0;
        bool saveAllowed = true;
        QWidget host;
        host.resize(800, 600);
        auto* layout = new QVBoxLayout(&host);
        auto create = [&] {
            return createGraphicsSettingsPage(&host, committed, QStringLiteral("Test"),
                [&](const graphics::Settings& proposed) {
                    if (!saveAllowed || !graphics::save(storage, proposed, &error)) return false;
                    committed = proposed;
                    ++commits;
                    return true;
                }, [&] { ++closed; },
                [&](graphics::Feature feature) {
                    if (feature == graphics::Feature::BodyTransparency)
                        return QStringLiteral("Körperdeckkraft %1").arg(committed.profiles[0].bodyOpacity);
                    return committed.profiles[0].on(feature)
                        ? QStringLiteral("Aktiviert.") : QStringLiteral("Ausgeschaltet.");
                });
        };
        std::unique_ptr<QWidget> page(create());
        layout->addWidget(page.get());
        host.show();
        app.processEvents();
        require(page->window() == &host && !page->findChild<QTabWidget*>(),
            "Static technique must be embedded with no optical/profile tabs.");
        require(!page->findChild<QPushButton*>(QStringLiteral("applyGraphics"))
            && !page->findChild<QPushButton*>(QStringLiteral("cancelGraphics")),
            "Immediate settings must not have Apply/Cancel.");
        require(!page->findChild<QCheckBox*>(QStringLiteral("profile0_throughBody")),
            "Static technique must not offer force/vector settings.");
        auto* table = page->findChild<QTableWidget*>(QStringLiteral("staticGraphicsTable"));
        require(table && table->columnCount() == 4 && table->rowCount() > 31
            && table->horizontalHeaderItem(0)->text().isEmpty()
            && table->horizontalHeaderItem(1)->text() == QStringLiteral("Beschreibung")
            && table->horizontalHeaderItem(2)->text() == QStringLiteral("CPU Last")
            && table->horizontalHeaderItem(3)->text() == QStringLiteral("GPU Last"),
            "Required four-column table and unnamed checkbox column missing.");
        for (std::size_t i = 0; i < graphics::featureCount; ++i) {
            const auto key = QString::fromLatin1(graphics::features()[i].key);
            auto* check = page->findChild<QCheckBox*>(QStringLiteral("profile0_%1").arg(key));
            require(check && check->text().isEmpty() && !check->accessibleName().isEmpty(),
                "Feature needs an unnamed accessible checkbox.");
            require(!table->item(static_cast<int>(i), 2)->text().isEmpty()
                && !table->item(static_cast<int>(i), 3)->text().isEmpty(),
                "Every feature needs CPU and GPU load.");
        }
        auto* water = page->findChild<QCheckBox*>(QStringLiteral("profile0_water"));
        const bool original = water->isChecked();
        water->toggle();
        require(commits == 1 && closed == 0
            && committed.profiles[0].on(graphics::Feature::Water) != original,
            "Checkbox must persist immediately without closing.");
        require(page->findChild<QLabel*>(QStringLiteral("profile0_status_water"))->text()
            .contains(QStringLiteral("Ausgeschaltet.")),
            "Feature status must reflect the applied setting immediately.");
        auto* pbr = page->findChild<QCheckBox*>(QStringLiteral("profile0_pbr"));
        auto* texture = page->findChild<QCheckBox*>(QStringLiteral("profile0_colorTexture"));
        pbr->setChecked(true);
        require(texture->isEnabled(), "PBR dependencies must stay explicit.");
        pbr->setChecked(false);
        require(!texture->isEnabled(), "PBR dependency state must update immediately.");
        const int beforeFailure = commits;
        saveAllowed = false;
        water->toggle();
        require(commits == beforeFailure && water->isChecked() != original
            && page->findChild<QLabel*>(QStringLiteral("graphicsDiagnostic"))->text()
                .contains(QStringLiteral("fehlgeschlagen")),
            "Save failure must report and restore last active checkbox.");
        auto* opacity = page->findChild<QDoubleSpinBox*>(QStringLiteral("profile0_bodyOpacity"));
        const double beforeOpacity = opacity->value();
        opacity->setValue(.65);
        require(opacity->value() == beforeOpacity && commits == beforeFailure,
            "Failed numeric setting must restore saved value.");
        saveAllowed = true;
        opacity->setValue(.65);
        require(committed.profiles[0].bodyOpacity == .65 && closed == 0,
            "Numeric changes must save immediately.");
        require(page->findChild<QLabel*>(QStringLiteral("profile0_status_bodyTransparency"))->text()
            .contains(QStringLiteral("0.65")),
            "Opacity explanation must refresh after applying numeric settings.");
        auto* spacing = page->findChild<QLineEdit*>(QStringLiteral("profile0_trackSpacing"));
        spacing->setText(QStringLiteral("0,00001"));
        QMetaObject::invokeMethod(spacing, "editingFinished", Qt::DirectConnection);
        require(committed.profiles[0].trackSpacing == .00001,
            "Valid small track spacing must not be silently clamped.");
        spacing->setText(QStringLiteral("-1"));
        QMetaObject::invokeMethod(spacing, "editingFinished", Qt::DirectConnection);
        require(committed.profiles[0].trackSpacing == .00001
            && spacing->text().toDouble() == .00001
            && page->findChild<QLabel*>(QStringLiteral("graphicsDiagnostic"))->text()
                .contains(QStringLiteral("ausserhalb")),
            "Invalid text must report the error and restore the active setting.");
        page->findChild<QPushButton*>(QStringLiteral("closeGraphics"))->click();
        require(closed == 1, "Back must close without discarding settings.");
        auto reloaded = graphics::defaults();
        require(graphics::load(storage, &reloaded, &error)
            && reloaded.profiles[0].bodyOpacity == .65
            && reloaded.profiles[0].on(graphics::Feature::Water) != original,
            "Immediate static settings must survive storage reload.");
        require(reloaded.profiles[1].enabled == graphics::defaults(1).enabled
            && reloaded.active == 1, "Optical profile and previous active profile must be retained.");
        require(table->isRowHidden(static_cast<int>(graphics::Feature::Wake))
                && table->isRowHidden(static_cast<int>(graphics::Feature::WaterTrack))
                && table->isRowHidden(static_cast<int>(graphics::Feature::DistanceMarks))
                && table->isRowHidden(static_cast<int>(graphics::Feature::Trail)),
            "Static settings must omit motion and speed visualization choices.");
        auto* reset = page->findChild<QPushButton*>(QStringLiteral("resetGraphicsDefaults"));
        require(reset, "Static settings need a default reset button.");
        const auto beforeReset = committed;
        saveAllowed = false;
        reset->click();
        require(committed.profiles[0].enabled == beforeReset.profiles[0].enabled
                && opacity->value() == .65,
            "Failed reset must preserve both applied settings and controls.");
        saveAllowed = true;
        reset->click();
        const auto technical = graphics::staticDefaults().profiles[0];
        require(graphics::load(storage, &reloaded, &error)
                && reloaded.profiles[0].enabled == technical.enabled
                && reloaded.profiles[0].waterOpacity == technical.waterOpacity
                && reloaded.profiles[0].bodyOpacity == 1.
                && opacity->value() == 1.
                && pbr->isChecked() && texture->isChecked()
                && !reloaded.profiles[0].on(graphics::Feature::Shadows)
                && !reloaded.profiles[0].on(graphics::Feature::Reflection)
                && !reloaded.profiles[0].on(graphics::Feature::Lights)
                && reloaded.profiles[0].on(graphics::Feature::Water),
            "Reset must persist the calm material-visible technical default and synchronize controls.");
        opacity->setValue(.8);
        require(graphics::load(storage, &reloaded, &error)
                && reloaded.profiles[0].bodyOpacity == .8,
            "User changes after reset must still persist.");
        storage.endGroup();
        require(storage.value(QStringLiteral("grafik/profile_v1")) == dynamicData,
            "Static settings must not modify existing dynamic settings.");
        auto dynamic = graphics::defaults();
        std::unique_ptr<QWidget> dynamicPage(createGraphicsSettingsPage(&host, dynamic, {},
            [&](const graphics::Settings& proposed) { dynamic = proposed; return true; },
            [] {}, {}, false));
        auto* dynamicTable = dynamicPage->findChild<QTableWidget*>(QStringLiteral("staticGraphicsTable"));
        require(dynamicTable->isRowHidden(static_cast<int>(graphics::Feature::Wake)),
            "Dynamic motion switches must be outside the graphics settings page.");
        dynamicPage->findChild<QPushButton*>(QStringLiteral("resetGraphicsDefaults"))->click();
        require(dynamic.profiles[0].quality == 2
                && dynamic.profiles[0].on(graphics::Feature::Wake)
                && dynamic.profiles[0].on(graphics::Feature::Shadows)
                && dynamic.profiles[0].on(graphics::Feature::Reflection)
                && !dynamic.profiles[0].on(graphics::Feature::WaterTrack)
                && !dynamic.profiles[0].on(graphics::Feature::DistanceMarks)
                && !dynamic.profiles[0].on(graphics::Feature::Trail),
            "Dynamic default must use maximum optical quality and bow/stern waves only.");
        std::cout << "Static table, immediate persistence, dependencies and rollback passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
