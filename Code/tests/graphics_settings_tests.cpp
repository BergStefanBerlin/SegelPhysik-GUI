#include "graphics_settings.hpp"

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSettings>
#include <QTemporaryDir>
#include <algorithm>
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
    QCoreApplication app(argc, argv);
    try {
        const auto args = app.arguments();
        if (args.size() == 3) {
            QSettings storage(args[2], QSettings::IniFormat);
            QString error;
            auto settings = graphics::defaults();
            if (args[1] == QStringLiteral("--write")) {
                settings.active = 1;
                settings.profiles[0].enabled.fill(false);
                settings.profiles[1].bodyOpacity = 0.83;
                settings.profiles[0].normalScale = 0.17;
                settings.profiles[0].waterPeriod = 0.72;
                settings.profiles[0].trackAge = 45;
                settings.profiles[0].trackSpacing = 0.5;
                settings.profiles[0].markSpacing = 7;
                settings.profiles[0].trackBudget = 128;
                settings.profiles[1].environmentPath = QStringLiteral("missing-environment.hdr");
                require(graphics::save(storage, settings, &error), "subprocess save");
            } else {
                require(graphics::load(storage, &settings, &error), "subprocess load");
                require(settings.active == 1, "active profile restart");
                for (bool enabled : settings.profiles[0].enabled) require(!enabled, "profile isolation");
                require(settings.profiles[1].bodyOpacity == 0.83, "opacity restart");
                require(settings.profiles[0].normalScale == 0.17, "normal strength restart");
                require(settings.profiles[0].waterPeriod == 0.72, "world-scale period restart");
                require(settings.profiles[0].trackAge == 45 && settings.profiles[0].trackSpacing == 0.5
                    && settings.profiles[0].markSpacing == 7 && settings.profiles[0].trackBudget == 128,
                    "motion parameters restart");
                require(settings.profiles[1].environmentPath == QStringLiteral("missing-environment.hdr"),
                    "environment wish survives restart independently of asset availability");
            }
            return 0;
        }
        QTemporaryDir directory;
        require(directory.isValid(), "temporary directory");
        QString error;
        const auto initial = graphics::defaults();
        require(graphics::featureCount == 31, "feature count");
        const std::array technicalFeatures{
            graphics::Feature::Water, graphics::Feature::FXAA, graphics::Feature::Peeling,
            graphics::Feature::Smooth, graphics::Feature::Waterline, graphics::Feature::Grid,
            graphics::Feature::Axes, graphics::Feature::BodyTransparency,
            graphics::Feature::WaterTrack, graphics::Feature::DistanceMarks};
        for (std::size_t i = 0; i < graphics::featureCount; ++i) {
            const auto f = static_cast<graphics::Feature>(i);
            require(initial.profiles[1].enabled[i] == (f != graphics::Feature::Trail
                    && f != graphics::Feature::Grid && f != graphics::Feature::WaterTrack
                    && f != graphics::Feature::DistanceMarks),
                "optical defaults on except temporarily disabled course trail");
            const bool expected = std::find(technicalFeatures.begin(), technicalFeatures.end(),
                static_cast<graphics::Feature>(i)) != technicalFeatures.end();
            require(initial.profiles[0].enabled[i] == expected, "technical default matrix");
        }
        require(initial.active == 0 && initial.profiles[0].bodyOpacity == 0.35
            && initial.profiles[0].waterOpacity == 0.25 && initial.profiles[0].throughBody
            && initial.profiles[0].quality == 1, "technical parameters");
        require(initial.profiles[1].bodyOpacity == 1.0 && initial.profiles[1].waterOpacity == 0.75
            && !initial.profiles[1].throughBody && initial.profiles[1].quality == 2,
            "optical parameters");
        for (const auto& profile : initial.profiles) {
            require(!profile.liveReduction && profile.environmentPath.isEmpty()
                && profile.normalScale == 0.06 && profile.waterPeriod == 0.35,
                "shared parameter defaults");
        }
        QSettings storage(directory.filePath(QStringLiteral("settings.ini")), QSettings::IniFormat);
        storage.setValue(QStringLiteral("darstellung/sichtmodus"), 0);
        storage.setValue(QStringLiteral("darstellung/kraft_1"), false);
        auto loaded = initial;
        require(graphics::load(storage, &loaded, &error), "migration");
        require(loaded.active == 1, "spatial migrated to optical");
        storage.setValue(QStringLiteral("darstellung/sichtmodus"), 1);
        require(graphics::load(storage, &loaded, &error) && loaded.active == 1, "migration only once");
        require(!storage.value(QStringLiteral("darstellung/kraft_1")).toBool(), "legacy selection untouched");
        for (std::size_t i = 0; i < graphics::featureCount; ++i) {
            auto changed = initial;
            changed.profiles[0].enabled[i] = !changed.profiles[0].enabled[i];
            changed.profiles[1].enabled[i] = false;
            require(graphics::save(storage, changed, &error), "individual toggle save");
            require(graphics::load(storage, &loaded, &error), "individual toggle load");
            require(loaded.profiles[0].enabled == changed.profiles[0].enabled, "technical round trip");
            require(loaded.profiles[1].enabled == changed.profiles[1].enabled, "optical round trip");
        }
        auto optionalTrail = initial;
        for (auto& profile : optionalTrail.profiles) {
            profile.enabled[static_cast<std::size_t>(graphics::Feature::Trail)] = true;
        }
        require(graphics::save(storage, optionalTrail, &error)
            && graphics::load(storage, &loaded, &error), "optional course-trail persistence");
        require(loaded.profiles[0].on(graphics::Feature::Trail)
            && loaded.profiles[1].on(graphics::Feature::Trail), "course trail must remain available");
        require(graphics::save(storage, initial, &error), "save defaults");
        const QString key = QStringLiteral("grafik/profile_v1");
        auto root = QJsonDocument::fromJson(storage.value(key).toByteArray()).object();
        auto profiles = root.value(QStringLiteral("profiles")).toArray();
        auto object = profiles[1].toObject();
        object.remove(QStringLiteral("shadows"));
        object.remove(QStringLiteral("wake"));
        object.remove(QStringLiteral("waterTrack"));
        object.remove(QStringLiteral("distanceMarks"));
        object.remove(QStringLiteral("trackAge"));
        object.remove(QStringLiteral("trackBudget"));
        object.insert(QStringLiteral("grid"), true);
        profiles[1] = object;
        root.insert(QStringLiteral("profiles"), profiles);
        storage.setValue(key, QJsonDocument(root).toJson());
        require(graphics::load(storage, &loaded, &error), "new missing field default");
        require(loaded.profiles[1].on(graphics::Feature::Shadows), "new optical field enabled");
        require(loaded.profiles[1].on(graphics::Feature::Grid)
            && loaded.profiles[1].on(graphics::Feature::Wake)
            && !loaded.profiles[1].on(graphics::Feature::WaterTrack)
            && !loaded.profiles[1].on(graphics::Feature::DistanceMarks)
            && loaded.profiles[1].trackAge == 30 && loaded.profiles[1].trackBudget == 4096,
            "migration must preserve explicit old grid while adding motion defaults");
        const auto lastValid = loaded;
        const auto validRoot = root;
        for (const char* field : {"environmentPath", "throughBody", "quality", "water", "waterPeriod",
                 "trackAge", "trackSpacing", "markSpacing", "trackBudget"}) {
            root = validRoot;
            profiles = root.value(QStringLiteral("profiles")).toArray();
            object = profiles[1].toObject();
            object.insert(QString::fromLatin1(field), QJsonArray{});
            profiles[1] = object;
            root.insert(QStringLiteral("profiles"), profiles);
            storage.setValue(key, QJsonDocument(root).toJson());
            require(!graphics::load(storage, &loaded, &error), "wrong field type rejected");
            require(loaded.profiles[1].enabled == lastValid.profiles[1].enabled,
                "wrong field type preserves state");
        }
        storage.setValue(key, QByteArray("{broken"));
        require(!graphics::load(storage, &loaded, &error), "malformed settings rejected");
        require(loaded.active == lastValid.active, "failure preserves active");
        require(loaded.profiles[1].enabled == lastValid.profiles[1].enabled, "failure preserves values");
        auto invalid = initial;
        invalid.profiles[0].bodyOpacity = 2;
        require(!graphics::save(storage, invalid, &error), "invalid parameter rejected");
        invalid = initial;
        invalid.profiles[0].waterPeriod = 0.01;
        require(!graphics::save(storage, invalid, &error), "invalid world-scale period rejected");
        for (int n = 0; n < 4; ++n) {
            invalid = initial;
            if (n == 0) invalid.profiles[0].trackAge = 0;
            if (n == 1) invalid.profiles[0].trackSpacing = 0;
            if (n == 2) invalid.profiles[0].markSpacing = 0;
            if (n == 3) invalid.profiles[0].trackBudget = 65537;
            require(!graphics::save(storage, invalid, &error), "invalid motion parameter rejected");
        }
        const QString blocked = directory.filePath(QStringLiteral("file"));
        QFile file(blocked);
        require(file.open(QIODevice::WriteOnly), "blocker file");
        file.close();
        QSettings unwritable(blocked + QStringLiteral("/settings.ini"), QSettings::IniFormat);
        require(!graphics::save(unwritable, initial, &error), "storage failure reported");
        for (const QString mode : {QStringLiteral("--write"), QStringLiteral("--read")}) {
            QProcess child;
            child.start(app.applicationFilePath(), {mode, directory.filePath(QStringLiteral("restart.ini"))});
            require(child.waitForFinished(15000), "restart timeout");
            require(child.exitStatus() == QProcess::NormalExit && child.exitCode() == 0, "restart process failed");
        }
        std::cout << "28 defaults/toggles, independent profiles, migration, subprocess restart and error cases passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
