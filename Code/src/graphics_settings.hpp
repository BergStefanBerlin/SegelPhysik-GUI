#pragma once

#include <QString>
#include <array>

class QSettings;

namespace graphics {
enum class Feature {
    Water, Fresnel, Depth, WaterNormals, Animation, WaterIBL, Reflection,
    Sky, Haze, ModelIBL, Lights, Shadows, PBR, Coat, ColorTexture, ORM,
    NormalTexture, Anisotropy, Cloth, FXAA, SSAO, Peeling, Smooth,
    Waterline, Grid, Axes, Trail, BodyTransparency, Wake, WaterTrack, DistanceMarks, Count
};
constexpr std::size_t featureCount = static_cast<std::size_t>(Feature::Count);
struct FeatureInfo {
    const char* key;
    const char* group;
    const char* label;
    bool technical;
};
const std::array<FeatureInfo, featureCount>& features();
struct Profile {
    std::array<bool, featureCount> enabled{};
    double bodyOpacity = 1.0;
    double waterOpacity = 0.75;
    double normalScale = 0.06;
    double waterPeriod = 0.35;
    double trackAge = 30.0;
    double trackSpacing = 0.25;
    double markSpacing = 5.0;
    int trackBudget = 4096;
    int quality = 2;
    bool throughBody = false;
    bool liveReduction = false;
    QString environmentPath;
    bool on(Feature feature) const { return enabled[static_cast<std::size_t>(feature)]; }
};
struct Settings {
    std::array<Profile, 2> profiles;
    int active = 0;
};
Profile defaults(int profile);
Settings defaults();
bool validate(const Settings& settings, QString* error);
bool load(QSettings& storage, Settings* settings, QString* error);
bool save(QSettings& storage, const Settings& settings, QString* error);
}
