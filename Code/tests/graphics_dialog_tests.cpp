#include "graphics_dialog.hpp"

#include <QApplication>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>
#include <iostream>
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
    auto activeDialog = []() -> QDialog* {
        for (auto* widget : QApplication::topLevelWidgets()) {
            if (auto* dialog = qobject_cast<QDialog*>(widget)) {
                if (dialog->objectName() == QStringLiteral("graphicsSetup")) return dialog;
            }
        }
        return nullptr;
    };
    QTimer watchdog;
    watchdog.setInterval(5000);
    QObject::connect(&watchdog, &QTimer::timeout, &app, [&] {
        if (auto* dialog = activeDialog()) dialog->reject();
    });
    watchdog.start();
    try {
        auto committed = graphics::defaults();
        int commits = 0;
        bool controlsFound = false;
        bool dependenciesCorrect = false;
        bool matrixComplete = false;
        QTimer::singleShot(0, [&] {
            auto* dialog = activeDialog();
            if (!dialog) return;
            auto* buttons = dialog->findChild<QDialogButtonBox*>();
            auto* technical = dialog->findChild<QCheckBox*>(QStringLiteral("profile0_ssao"));
            auto* optical = dialog->findChild<QCheckBox*>(QStringLiteral("profile1_ssao"));
            auto* environment = dialog->findChild<QLineEdit*>(QStringLiteral("profile1_environment"));
            auto* age = dialog->findChild<QLineEdit*>(QStringLiteral("profile0_trackAge"));
            auto* budget = dialog->findChild<QSpinBox*>(QStringLiteral("profile0_trackBudget"));
            auto* grid = dialog->findChild<QCheckBox*>(QStringLiteral("profile0_grid"));
            auto* motionGrid = dialog->findChild<QCheckBox*>(QStringLiteral("profile0_motionGrid"));
            auto* track = dialog->findChild<QCheckBox*>(QStringLiteral("profile0_waterTrack"));
            auto* marks = dialog->findChild<QCheckBox*>(QStringLiteral("profile0_distanceMarks"));
            controlsFound = buttons && technical && optical && environment && age && budget
                && grid && motionGrid && track && marks;
            if (!controlsFound) { dialog->reject(); return; }
            matrixComplete = true;
            for (int n = 0; n < 2; ++n) {
                auto* trail = dialog->findChild<QCheckBox*>(QStringLiteral("profile%1_trail").arg(n));
                if (!trail || trail->isChecked() || !trail->isEnabled()) matrixComplete = false;
                for (const auto& feature : graphics::features()) {
                    if (!dialog->findChild<QCheckBox*>(QStringLiteral("profile%1_%2").arg(n)
                            .arg(QString::fromLatin1(feature.key)))) matrixComplete = false;
                }
            }
            auto* pbr = dialog->findChild<QCheckBox*>(QStringLiteral("profile0_pbr"));
            auto* texture = dialog->findChild<QCheckBox*>(QStringLiteral("profile0_colorTexture"));
            auto* normals = dialog->findChild<QCheckBox*>(QStringLiteral("profile0_waterNormals"));
            auto* animation = dialog->findChild<QCheckBox*>(QStringLiteral("profile0_animation"));
            auto* water = dialog->findChild<QCheckBox*>(QStringLiteral("profile0_water"));
            dependenciesCorrect = pbr && texture && normals && animation && water;
            if (dependenciesCorrect) {
                track->setChecked(false);
                dependenciesCorrect = marks->isChecked() && !marks->isEnabled();
                track->setChecked(true);
                dependenciesCorrect = dependenciesCorrect && marks->isEnabled();
                motionGrid->setChecked(false);
                dependenciesCorrect = dependenciesCorrect && !grid->isChecked();
                grid->setChecked(true);
                dependenciesCorrect = dependenciesCorrect && motionGrid->isChecked();
                age->setText(QStringLiteral("45"));
                budget->setValue(128);
                dependenciesCorrect = !texture->isEnabled() && normals->isEnabled() && !animation->isEnabled();
                pbr->setChecked(true);
                normals->setChecked(true);
                dependenciesCorrect = dependenciesCorrect && texture->isEnabled() && animation->isEnabled();
                water->setChecked(false);
                dependenciesCorrect = dependenciesCorrect && !normals->isEnabled()
                    && normals->isChecked() && !animation->isEnabled();
                water->setChecked(true);
            }
            technical->setChecked(true);
            optical->setChecked(false);
            environment->setText(QStringLiteral("test-environment.hdr"));
            age->setText(QStringLiteral("-1"));
            buttons->button(QDialogButtonBox::Apply)->click();
            age->setText(QStringLiteral("45"));
            buttons->button(QDialogButtonBox::Apply)->click();
            age->setText(QStringLiteral("1"));
            budget->setValue(4);
            technical->setChecked(false);
            optical->setChecked(true);
            environment->clear();
            buttons->button(QDialogButtonBox::Cancel)->click();
        });
        editGraphicsSettings(nullptr, committed, QStringLiteral("Test"), [&](const graphics::Settings& proposed) {
            committed = proposed;
            ++commits;
            return true;
        });
        require(controlsFound, "dialog controls absent");
        require(matrixComplete, "feature matrix incomplete in dialog");
        require(dependenciesCorrect, "dependencies must be explicit without losing wishes");
        require(commits == 1, "apply/cancel commit count");
        require(committed.profiles[0].on(graphics::Feature::SSAO), "apply lost technical");
        require(!committed.profiles[1].on(graphics::Feature::SSAO), "cancel changed optical");
        require(committed.profiles[1].environmentPath == QStringLiteral("test-environment.hdr"),
            "cancel changed committed environment");
        require(committed.profiles[0].trackAge == 45 && committed.profiles[0].trackBudget == 128,
            "motion Apply/Cancel transaction");
        bool resetWorked = false;
        QTimer::singleShot(0, [&] {
            auto* dialog = activeDialog();
            if (!dialog) return;
            auto* all = dialog->findChild<QPushButton*>(QStringLiteral("profile0_all"));
            auto* none = dialog->findChild<QPushButton*>(QStringLiteral("profile1_none"));
            auto* reset = dialog->findChild<QPushButton*>(QStringLiteral("profile0_reset"));
            auto* buttons = dialog->findChild<QDialogButtonBox*>();
            if (!all || !none || !reset || !buttons) { dialog->reject(); return; }
            all->click();
            none->click();
            reset->click();
            resetWorked = true;
            buttons->button(QDialogButtonBox::Ok)->click();
        });
        editGraphicsSettings(nullptr, committed, {}, [&](const graphics::Settings& proposed) {
            committed = proposed;
            ++commits;
            return true;
        });
        require(resetWorked, "reset controls");
        require(commits == 2, "OK commit count");
        require(committed.profiles[0].enabled == graphics::defaults(0).enabled, "reset technical");
        for (bool enabled : committed.profiles[1].enabled) require(!enabled, "optical all off independent");
        QTimer::singleShot(0, [&] {
            auto* dialog = activeDialog();
            if (!dialog) return;
            auto* buttons = dialog->findChild<QDialogButtonBox*>();
            if (buttons) buttons->button(QDialogButtonBox::Ok)->click();
            QTimer::singleShot(0, dialog, &QDialog::reject);
        });
        editGraphicsSettings(nullptr, committed, {}, [&](const graphics::Settings&) {
            ++commits;
            return false;
        });
        require(commits == 3, "failed persistence callback");
        std::cout << "Separate profile editing, Apply/Cancel and rejected persistence passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
