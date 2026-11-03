#include "core_adapter.hpp"
#include "display_geometry.hpp"
#include "force_display.hpp"
#include "graphics_settings.hpp"
#include "model_catalog.hpp"
#include "vtk_scene.hpp"
#include <QApplication>
#include <QCheckBox>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSettings>
#include <QSurfaceFormat>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <QTabBar>
#include <QVTKOpenGLNativeWidget.h>
#include <vtkLogger.h>
#include "segelphysik/modell.hpp"
#include "segelphysik/simulation.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <filesystem>
#include <functional>
#include <iostream>
#include <numeric>
#include <stdexcept>
#if AUDIT_CURRENT
#include "audit_main_window.hpp"
#endif

void check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
double elapsed(const std::function<void()>& action) {
    QElapsedTimer timer;
    timer.start();
    action();
    return timer.nsecsElapsed() / 1e6;
}
QJsonObject summary(std::vector<double> values) {
    QJsonArray raw;
    for (double value : values) raw.append(value);
    std::sort(values.begin(), values.end());
    return {{ "median_ms", values[values.size()/2] }, { "min_ms", values.front() },
        { "max_ms", values.back() }, { "raw_ms", raw }};
}
int main(int argc, char** argv) {
    vtkLogger::SetStderrVerbosity(vtkLogger::VERBOSITY_WARNING);
    QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());
    QApplication app(argc, argv);
    try {
        check(argc == 4, "Expected models, output JSON and snapshot directory.");
        QTemporaryDir settings;
        check(settings.isValid(), "Temporary settings failed.");
        QCoreApplication::setOrganizationName("PerformanceAudit");
        QCoreApplication::setApplicationName(AUDIT_REVISION);
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
        const QString models = QString::fromLocal8Bit(argv[1]);
        const QString snapshotDir = QString::fromLocal8Bit(argv[3]);
        QDir().mkpath(snapshotDir);
        const auto catalog = segelphysik::modellkatalog(std::filesystem::path(models.toStdWString()));
        QJsonObject report{{"revision", AUDIT_REVISION}, {"warmups", 1}, {"repeats", 5},
            {"snapshots_per_repeat", 6}, {"viewport", "720x480 plus 3x180x160"},
            {"scale", qEnvironmentVariable("QT_SCALE_FACTOR")}};
        for (const QString id : {"420", "folkeboot"}) {
            std::cout << "Model " << id.toStdString() << std::endl;
            const auto* entry = catalog.eintrag(id.toStdString());
            check(entry != nullptr, "Model missing.");
            const auto path = QDir(models).filePath(id + "/modell.json");
            const auto validation = validateModel(path);
            check(validation.valid, "Model invalid.");
            QVector<DisplayPart> parts;
            const double sourceMs = elapsed([&] {
                parts = toDisplayParts(segelphysik::darstellungsgeometrie(
                    catalog, entry->modell_id, entry->modell_fassung));
            });
            report.insert(id + "_source_preparation_ms", sourceMs);
            QVector<QJsonObject> snapshots;
            const QString snapshotPath = QDir(snapshotDir).filePath(id + ".json");
            QFile input(snapshotPath);
            if (input.exists()) {
                check(input.open(QIODevice::ReadOnly), "Cannot read frozen snapshots.");
                for (const auto& value : QJsonDocument::fromJson(input.readAll()).array())
                    snapshots.push_back(value.toObject());
            } else {
                QString error;
                auto live = LiveSimulation::create(path, validation, .5, .01, false, &error, {},
                    TestDriveOptions{true, 200., 463./90.});
                check(live != nullptr, "Live create failed.");
                std::atomic_bool stop{false};
                live->run(stop);
                check(live->progress().status == "abgeschlossen", "Live failed.");
                QJsonArray output;
                for (int i = 1; i <= 6; ++i) {
                    auto state = live->stateJson(i * 8, &error);
                    check(error.isEmpty() && !state.isEmpty(), "Snapshot failed.");
                    snapshots.push_back(state);
                    output.append(state);
                }
                QFile file(snapshotPath);
                check(file.open(QIODevice::WriteOnly), "Cannot persist snapshots.");
                file.write(QJsonDocument(output).toJson());
            }
            QJsonArray cases;
#if AUDIT_CURRENT
            const QStringList modes{"shared_static_dynamic_scene"};
#else
            const QStringList modes{"legacy"};
#endif
            for (const auto& mode : modes) {
#if AUDIT_CURRENT
                const QStringList layouts{"single", "four", "hidden_aux"};
#else
                const QStringList layouts{"single"};
#endif
                for (const auto& viewCase : layouts)
                for (bool simple : {true, false}) {
                    QWidget panel;
                    panel.resize(930, 500);
                    auto* layout = new QHBoxLayout(&panel);
                    auto* free = new VtkSceneWidget;
                    free->setFixedSize(720, 480);
                    layout->addWidget(free);
                    std::vector<VtkSceneWidget*> scenes{free};
#if AUDIT_CURRENT
                    auto* column = new QWidget;
                    auto* vertical = new QVBoxLayout(column);
                    layout->addWidget(column);
                    if (viewCase != "single") {
                        for (int i = 0; i < 3; ++i) {
                            auto* view = new VtkSceneWidget;
                            view->setFixedSize(180, 160);
                            view->setOrthographicView(
                                static_cast<VtkSceneWidget::OrthographicView>(i+1));
                            vertical->addWidget(view);
                            scenes.push_back(view);
                        }
                    }
#endif
                    VtkSceneWidget::DisplayOptions options;
                    options.forces = false;
                    options.leverArm = false;
                    options.lateralCenter = simple;
                    options.sailCenter = simple;
#if AUDIT_CURRENT
                    options.courseArrow = simple;
                    options.staticForces = true;
                    options.staticStyles = forcedisplay::staticForceDefaults();
                    if (!simple)
                        for (auto& style : options.staticStyles) style.enabled = false;
#else
                    options.forces = simple;
#endif
                    graphics::Profile profile;
                    QString error;
                    QJsonArray modelLoads;
                    for (auto* view : scenes) {
                        view->setNavigationVisible(false);
                        modelLoads.append(elapsed([&] {
                            check(view->setModel(parts, &error), "Set model failed.");
                        }));
                        view->setOptions(options);
                        view->setGraphicsProfile(profile);
                    }
                    panel.show();
#if AUDIT_CURRENT
                    if (viewCase == "hidden_aux") {
                        column->hide();
#if AUDIT_OPTIMIZED
                        for (std::size_t i=1; i<scenes.size(); ++i)
                            scenes[i]->setUpdatesActive(false);
#endif
                    }
#endif
                    QApplication::processEvents();
#if AUDIT_OPTIMIZED
                    std::vector<VtkSceneWidget::WorkCounters> before;
                    for (auto* view : scenes) before.push_back(view->workCounters());
#endif
                    std::vector<double> renderTimes, profileTimes, parseTimes;
                    for (int repeat = -1; repeat < 5; ++repeat) {
                        double renderMs=0., profileMs=0., parseMs=0.;
                        for (qsizetype step=0; step<snapshots.size(); ++step) {
                            const auto& state = snapshots[repeat >= 0 && repeat % 2
                                ? snapshots.size()-1-step : step];
                            forcedisplay::ForceState force;
                            parseMs += elapsed([&] { force = forcedisplay::parseForceState(state); });
                            renderMs += elapsed([&] {
                                for (auto* view : scenes)
                                    check(view->setState(state, force, &error), "Set state failed.");
                            });
                            profileMs += elapsed([&] {
                                for (auto* view : scenes) view->setGraphicsProfile(profile);
                            });
                        }
                        if (repeat >= 0) {
                            renderTimes.push_back(renderMs/6.);
                            profileTimes.push_back(profileMs/6.);
                            parseTimes.push_back(parseMs/6.);
                        }
                    }
                    QJsonObject result{{"mode",mode},{"views",viewCase},{"simple",simple},
                        {"model_load_ms",modelLoads},
                        {"state",summary(renderTimes)},{"same_profile",summary(profileTimes)},
                        {"force_parse",summary(parseTimes)}};
#if AUDIT_OPTIMIZED
                    QJsonArray work;
                    for (std::size_t i=0; i<scenes.size(); ++i) {
                        const auto after = scenes[i]->workCounters();
                        work.append(QJsonObject{{"active",scenes[i]->updatesActive()},
                            {"states",static_cast<double>(after.states-before[i].states)},
                            {"preparations",static_cast<double>(after.meshPreparations-before[i].meshPreparations)},
                            {"profiles",static_cast<double>(after.profiles-before[i].profiles)},
                            {"renders",static_cast<double>(after.renders-before[i].renders)}});
                    }
                    result.insert("scene_work",work);
#endif
                    cases.append(result);
                    std::cout << id.toStdString() << " " << viewCase.toStdString()
                              << " simple=" << simple << " complete" << std::endl;
                    panel.close();
                }
            }
#if AUDIT_CURRENT
            QJsonArray windowCases;
            QString error;
            MainWindow window;
            window.resize(1600, 1000);
            window.setModelFolder(models);
            window.selectInitialModel(id);
            window.show();
            QElapsedTimer load;
            load.start();
            while ((!window.validation_.valid || window.modelId_ != id)
                && load.elapsed() < 30000) QApplication::processEvents();
            check(window.validation_.valid && window.modelId_ == id, "Main window load failed.");
            for (int workspace : {1, 2}) {
                window.findChild<QTabBar*>("workspaceTabs")->setCurrentIndex(workspace);
                window.states_ = snapshots;
                window.live_.reset();
                for (bool auxiliary : {true, false}) {
                window.attitudeViewsCheck_->setChecked(auxiliary);
                for (bool simple : {true, false}) {
                    const auto defaults = forcedisplay::staticForceDefaults();
                    for (std::size_t i=0; i<defaults.size(); ++i)
                        window.staticForceStyles_[i].enabled = simple && defaults[i].enabled;
                    window.applyDisplayOptions();
                    std::vector<double> total, table, allValues, profile, menu, json;
                    QJsonArray rows;
                    for (int repeat=-1; repeat<5; ++repeat) {
                        double t=0., ta=0., va=0., pr=0., me=0., js=0.;
                        for (qsizetype step=0; step<snapshots.size(); ++step) {
                            const qsizetype i = repeat >= 0 && repeat % 2
                                ? snapshots.size()-1-step : step;
                            t += elapsed([&] { window.displayState(i, true); });
                            if (repeat == -1) {
                                QJsonArray cells;
                                for (int row=0; row<window.staticValues_->topLevelItemCount(); ++row) {
                                    QJsonArray columns;
                                    for (int col=0; col<window.staticValues_->columnCount(); ++col)
                                        columns.append(window.staticValues_->topLevelItem(row)->text(col));
                                    cells.append(columns);
                                }
                                rows.append(cells);
                            }
                            ta += elapsed([&] { window.updateStaticValues(); });
                            va += elapsed([&] { window.updateValues(); });
                            pr += elapsed([&] { window.applyGraphicsProfile(); });
                            me += elapsed([&] { window.updateStaticForceMenu(); });
                            js += elapsed([&] { const auto copy=window.stateAt(i, &error); check(!copy.isEmpty(), "stateAt"); });
                        }
                        if (repeat>=0) {
                            total.push_back(t/6.); table.push_back(ta/6.); allValues.push_back(va/6.);
                            profile.push_back(pr/6.); menu.push_back(me/6.); json.push_back(js/6.);
                        }
                    }
                    const QString imagePath = QString::fromLocal8Bit(argv[2]) + "." + id
                        + "." + QString::number(workspace) + "." + QString::number(auxiliary)
                        + "." + QString::number(simple) + ".bmp";
                    check(window.scene_->captureImage().save(imagePath, "BMP"), "Cannot save reference image.");
                    windowCases.append(QJsonObject{{"mode",workspace==1 ? "static":"dynamic"},{"simple",simple},
                        {"auxiliary",auxiliary},{"table_reference",rows},
                        {"display_state",summary(total)},{"visible_table",summary(table)},
                        {"all_values_including_visible_table",summary(allValues)},
                        {"profile",summary(profile)},{"force_menu",summary(menu)},{"cached_state",summary(json)}});
                    std::cout << id.toStdString() << " window " << workspace << " simple="
                              << simple << " complete" << std::endl;
                }
                }
            }
            window.close();
            report.insert(id + "_window", windowCases);
#endif
            report.insert(id,cases);
        }
        QFile output(QString::fromLocal8Bit(argv[2]));
        check(output.open(QIODevice::WriteOnly), "Cannot write report.");
        output.write(QJsonDocument(report).toJson());
        std::cout << "Done " << AUDIT_REVISION << std::endl;
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
