#include "graphics_settings.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <cmath>

namespace graphics {
const std::array<FeatureInfo, featureCount>& features()
{
    static const std::array<FeatureInfo, featureCount> info{{
        {"water", "Wasser", "Wasser bis zum Horizont", true,
            "An: Wasserflaeche bis zum Horizont; aus: freie Unterwassersicht. Basis fuer Wassereffekte. CPU niedrig, GPU mittel, Speicher mittel."},
        {"fresnel", "Wasser", "Fresnel-Effekt", false,
            "An: blickwinkelabhaengige Wasserreflexion; aus: gleichmaessigere Wasserfarbe. Benoetigt Wasser. CPU niedrig, GPU niedrig, Speicher niedrig."},
        {"depth", "Wasser", "Tiefenfarbe und Tiefenebene", false,
            "An: Tiefeneindruck durch Farbverlauf und Ebene; aus: kein Tiefenverlauf. Benoetigt Wasser. CPU niedrig, GPU niedrig, Speicher niedrig."},
        {"waterNormals", "Wasser", "Feine Normalstruktur", false,
            "An: feine optische Wasserwellen; aus: glatte Flaeche. Benoetigt Wasser. Keine Wellenkraefte. CPU niedrig, GPU mittel, Speicher mittel."},
        {"animation", "Wasser", "Animation der Normalstruktur", false,
            "An: Wasserstruktur folgt der dargestellten Simulationszeit; aus: ruhende Struktur. Wasser und Normalstruktur erforderlich. CPU niedrig, GPU mittel, Speicher niedrig."},
        {"waterIBL", "Wasser", "Umgebungsspiegelung", false,
            "An: Himmel/Umgebung spiegelt sich im Wasser; aus: keine Umgebungsspiegelung. Benoetigt Wasser. CPU niedrig, GPU mittel, Speicher mittel."},
        {"reflection", "Wasser", "Spiegelung des Boots", false,
            "An: zusaetzliche gespiegelte Bootsgeometrie; aus: kein Bootsreflex. Benoetigt Wasser. CPU mittel, GPU hoch, Speicher mittel; mehr transparente Geometrie erhoeht Kosten."},
        {"sky", "Umgebung", "Himmel", false,
            "An: sichtbarer Umgebungshimmel; aus: Hintergrundfarbe. Beleuchtung bleibt separat schaltbar. CPU niedrig, GPU niedrig, Speicher mittel."},
        {"haze", "Umgebung", "Horizontdunst", false,
            "An: weicher ferner Horizont; aus: klare Entfernung. Nur Darstellung, keine Sichtweitenphysik. CPU niedrig, GPU niedrig, Speicher niedrig."},
        {"modelIBL", "Beleuchtung", "Umgebungslicht der Materialien", false,
            "An: Umgebung beleuchtet PBR-Materialien; aus: nur direkte Beleuchtung. PBR erforderlich. CPU niedrig, GPU mittel, Speicher mittel."},
        {"lights", "Beleuchtung", "Erweitertes Lichtset", false,
            "An: mehrere Szenenlichter; aus: einfachere Grundbeleuchtung. CPU niedrig, GPU mittel je sichtbarer Geometrie, Speicher niedrig."},
        {"shadows", "Beleuchtung", "Sonnenschatten", false,
            "An: zusaetzlicher Schattenpass; aus: keine Sonnenschatten. CPU mittel, GPU hoch, Speicher hoch; zusammen mit Reflexion/SSAO besonders aufwendig."},
        {"pbr", "Materialien", "Physikalisch basierte Materialien", false,
            "An: materialabhaengige Rauheit und Glanz; aus: einfachere Oberflaechen. Basis fuer Materialdetails. CPU niedrig, GPU mittel, Speicher mittel; keine Aenderung der Physik."},
        {"coat", "Materialien", "Klarlackschicht", false,
            "An: zusaetzlicher Glanz auf passenden Lackoberflaechen; aus: Grundmaterial. PBR und geeignete Materialdaten erforderlich. CPU niedrig, GPU mittel, Speicher niedrig."},
        {"colorTexture", "Materialien", "Farbtexturen", false,
            "An: prozedurale Materialfarben; aus: Grundfarbe. PBR und passende Oberflaechen erforderlich; keine fehlenden Modellteile erzeugen. CPU mittel beim Aufbau, GPU mittel, Speicher mittel."},
        {"orm", "Materialien", "Rauheits-/ORM-Texturen", false,
            "An: lokale Rauheit/Metallanteile; aus: gleichmaessige Materialwerte. PBR und passende Oberflaechen erforderlich. CPU mittel beim Aufbau, GPU mittel, Speicher mittel."},
        {"normalTexture", "Materialien", "Normal-Maps", false,
            "An: feine optische Materialstruktur ohne neue Physikgeometrie; aus: glatte Normalen. PBR erforderlich. CPU mittel beim Aufbau, GPU mittel, Speicher mittel."},
        {"anisotropy", "Materialien", "Gebuerstetes Metall", false,
            "An: gerichteter Glanz auf explizit gebuerstetem Metall; aus: ungerichteter Glanz. PBR und passende Materialdaten erforderlich. CPU niedrig, GPU mittel, Speicher niedrig."},
        {"cloth", "Materialien", "Stofftransparenz", false,
            "An: grafische Lichtdurchlaessigkeit expliziter Stoffteile; aus: deckender Stoff. Ohne Stoffmaterial nicht anwendbar. CPU niedrig, GPU mittel bis hoch bei Transparenz, Speicher mittel."},
        {"fxaa", "Bildqualitaet", "Kantenglaettung (FXAA)", true,
            "An: Nachbearbeitung glaettet Bildkanten; aus: schaerfere, eventuell gezackte Kanten. CPU niedrig, GPU mittel je Pixelzahl, Speicher mittel."},
        {"ssao", "Bildqualitaet", "Umgebungsverdeckung (SSAO)", false,
            "An: Kontaktschatten in Vertiefungen; aus: keine Bildschirmraum-Verdeckung. CPU niedrig, GPU hoch, Speicher hoch; abhaengig von Aufloesung und Qualitaet."},
        {"peeling", "Bildqualitaet", "Transparenz-Tiefenschaelung", true,
            "An: mehrere Tiefendurchlaeufe sortieren Transparenz; aus: einfachere Ueberlagerung. CPU mittel, GPU hoch, Speicher hoch bei vielen transparenten Schichten."},
        {"smooth", "Geometrie", "Geglaettete Normalen", true,
            "An: weichere Oberflaechenschattierung; aus: sichtbare Facetten. Aendert keine Volumen- oder Kraftgeometrie. CPU mittel beim Aufbau, GPU niedrig, Speicher mittel."},
        {"waterline", "Hilfen", "Wasserlinie", true,
            "An: Schnittkontur bei Welt-Z=0; aus: Kontur verborgen. CPU mittel fuer Geometrieschnitt, GPU niedrig, Speicher niedrig; keine Aenderung der Wasserhoehe."},
        {"grid", "Hilfen", "Bezugsraster", true,
            "An: weltfestes Raster zur Bewegungsbeurteilung; aus: kein Raster. CPU niedrig, GPU niedrig bis mittel, Speicher niedrig."},
        {"axes", "Hilfen", "Achsenkreuz", true,
            "An: Orientierungshilfe der Koordinaten; aus: Achsen verborgen. CPU niedrig, GPU niedrig, Speicher niedrig."},
        {"trail", "Hilfen", "Kursspur", false,
            "An: geometrische Spur dargestellter Zustaende; aus: Spur verborgen. CPU/Speicher mittel, steigend mit Laenge; GPU mittel. Keine vorausberechnete Bahn."},
        {"bodyTransparency", "Transparenz", "Analytische Koerperdurchsicht", true,
            "An: Koerperdeckkraft fuer Unterwassersicht nutzen; aus: normale Materialdeckkraft. CPU niedrig, GPU mittel bis hoch mit Tiefenschaelung, Speicher mittel."},
        {"wake", "Bewegung durchs Wasser", "Kielwasser / Bugwelle (grafisch)", false,
            "An: geometrischer Fahreffekt; aus: keine Bug-/Heckwelle. Keine Wellenhydrodynamik. CPU mittel, GPU mittel, Speicher mittel; benoetigt passende Fahrtdaten."},
        {"waterTrack", "Bewegung durchs Wasser", "Technische Fahrspur auf Wasserhoehe", true,
            "An: rekonstruierte zeitliche Wasserspur; aus: verborgen. CPU/Speicher mittel bis hoch bei dichtem Punktbudget, GPU mittel; keine Solverwirkung."},
        {"distanceMarks", "Bewegung durchs Wasser", "Distanzmarken ohne Beschriftung", true,
            "An: Abstandsmarken entlang technischer Fahrspur; aus: keine Marken. Fahrspur erforderlich. CPU mittel, GPU niedrig bis mittel, Speicher niedrig; kleinere Abstaende kosten mehr."}
    }};
    return info;
}

Profile defaults(int profile)
{
    Profile result;
    for (std::size_t i = 0; i < featureCount; ++i) {
        result.enabled[i] = profile == 1 || features()[i].technical;
    }
    result.enabled[static_cast<std::size_t>(Feature::Trail)] = false;
    result.enabled[static_cast<std::size_t>(Feature::Grid)] = profile == 0;
    result.enabled[static_cast<std::size_t>(Feature::Wake)] = profile == 1;
    result.enabled[static_cast<std::size_t>(Feature::WaterTrack)] = profile == 0;
    result.enabled[static_cast<std::size_t>(Feature::DistanceMarks)] = profile == 0;
    if (profile == 0) {
        result.bodyOpacity = 0.35;
        result.waterOpacity = 0.25;
        result.throughBody = true;
        result.quality = 1;
    }
    return result;
}

Settings defaults()
{
    return {{defaults(0), defaults(1)}, 0};
}

bool validate(const Settings& settings, QString* error)
{
    if (settings.active < 0 || settings.active > 1) {
        if (error) *error = QStringLiteral("Ungueltiges Grafikprofil.");
        return false;
    }
    for (const auto& p : settings.profiles) {
        if (!std::isfinite(p.bodyOpacity) || p.bodyOpacity < 0 || p.bodyOpacity > 1
            || !std::isfinite(p.waterOpacity) || p.waterOpacity < 0 || p.waterOpacity > 1
            || !std::isfinite(p.normalScale) || p.normalScale < 0 || p.normalScale > 1
            || !std::isfinite(p.waterPeriod) || p.waterPeriod < 0.02 || p.waterPeriod > 20
            || p.quality < 0 || p.quality > 2
            || !std::isfinite(p.trackAge) || p.trackAge <= 0 || p.trackAge > 86400
            || !std::isfinite(p.trackSpacing) || p.trackSpacing <= 0 || p.trackSpacing > 10000
            || !std::isfinite(p.markSpacing) || p.markSpacing <= 0 || p.markSpacing > 10000
            || p.trackBudget < 2 || p.trackBudget > 65536) {
            if (error) *error = QStringLiteral("Grafikparameter liegen ausserhalb des erlaubten Bereichs.");
            return false;
        }
    }
    return true;
}

bool save(QSettings& storage, const Settings& settings, QString* error)
{
    if (!validate(settings, error)) return false;
    QJsonObject root;
    root.insert(QStringLiteral("version"), 1);
    root.insert(QStringLiteral("active"), settings.active);
    QJsonArray profiles;
    for (const auto& p : settings.profiles) {
        QJsonObject object;
        for (std::size_t i = 0; i < featureCount; ++i) {
            object.insert(QString::fromLatin1(features()[i].key), p.enabled[i]);
        }
        object.insert(QStringLiteral("bodyOpacity"), p.bodyOpacity);
        object.insert(QStringLiteral("waterOpacity"), p.waterOpacity);
        object.insert(QStringLiteral("normalScale"), p.normalScale);
        object.insert(QStringLiteral("waterPeriod"), p.waterPeriod);
        object.insert(QStringLiteral("trackAge"), p.trackAge);
        object.insert(QStringLiteral("trackSpacing"), p.trackSpacing);
        object.insert(QStringLiteral("markSpacing"), p.markSpacing);
        object.insert(QStringLiteral("trackBudget"), p.trackBudget);
        object.insert(QStringLiteral("quality"), p.quality);
        object.insert(QStringLiteral("throughBody"), p.throughBody);
        object.insert(QStringLiteral("liveReduction"), p.liveReduction);
        object.insert(QStringLiteral("environmentPath"), p.environmentPath);
        profiles.append(object);
    }
    root.insert(QStringLiteral("profiles"), profiles);
    const QString key = QStringLiteral("grafik/profile_v1");
    const auto previous = storage.value(key);
    storage.setValue(key, QJsonDocument(root).toJson(QJsonDocument::Compact));
    storage.sync();
    if (storage.status() != QSettings::NoError) {
        if (previous.isValid()) storage.setValue(key, previous);
        else storage.remove(key);
        if (error) *error = QStringLiteral("Grafikprofile konnten nicht dauerhaft gespeichert werden.");
        return false;
    }
    return true;
}

bool load(QSettings& storage, Settings* settings, QString* error)
{
    storage.sync();
    if (storage.status() != QSettings::NoError) {
        if (error) *error = QStringLiteral("Grafikeinstellungen konnten nicht gelesen werden.");
        return false;
    }
    Settings result = defaults();
    const QString key = QStringLiteral("grafik/profile_v1");
    if (!storage.contains(key)) {
        if (storage.contains(QStringLiteral("darstellung/sichtmodus"))) {
            bool ok = false;
            const int legacy = storage.value(QStringLiteral("darstellung/sichtmodus")).toInt(&ok);
            if (!ok || (legacy != 0 && legacy != 1)) {
                if (error) *error = QStringLiteral("Ungueltiger bisheriger Sichtmodus.");
                return false;
            }
            result.active = legacy == 0 ? 1 : 0;
        }
        if (!save(storage, result, error)) return false;
        *settings = result;
        return true;
    }
    QJsonParseError parse;
    const auto document = QJsonDocument::fromJson(storage.value(key).toByteArray(), &parse);
    const auto root = document.object();
    auto invalid = [error] {
        if (error) *error = QStringLiteral("Gespeicherte Grafikprofile sind ungueltig oder stammen aus einer nicht unterstuetzten Version.");
        return false;
    };
    if (parse.error != QJsonParseError::NoError || !document.isObject()
        || root.value(QStringLiteral("version")) != QJsonValue(1)
        || !root.value(QStringLiteral("active")).isDouble()
        || !root.value(QStringLiteral("profiles")).isArray()) return invalid();
    const double active = root.value(QStringLiteral("active")).toDouble();
    if (active != 0 && active != 1) return invalid();
    result.active = static_cast<int>(active);
    const auto profiles = root.value(QStringLiteral("profiles")).toArray();
    if (profiles.size() != 2) return invalid();
    for (int n = 0; n < 2; ++n) {
        if (!profiles[n].isObject()) return invalid();
        const auto object = profiles[n].toObject();
        auto& p = result.profiles[static_cast<std::size_t>(n)];
        const auto environment = object.value(QStringLiteral("environmentPath"));
        if (!environment.isUndefined() && !environment.isString()) return invalid();
        p.environmentPath = environment.toString();
        for (std::size_t i = 0; i < featureCount; ++i) {
            const auto v = object.value(QString::fromLatin1(features()[i].key));
            if (v.isUndefined()) continue;
            if (!v.isBool()) return invalid();
            p.enabled[i] = v.toBool();
        }
        auto number = [&object](const char* name, double* target) {
            const auto v = object.value(QString::fromLatin1(name));
            if (v.isUndefined()) return true;
            if (!v.isDouble()) return false;
            *target = v.toDouble();
            return true;
        };
        double quality = p.quality;
        double budget = p.trackBudget;
        if (!number("bodyOpacity", &p.bodyOpacity) || !number("waterOpacity", &p.waterOpacity)
            || !number("normalScale", &p.normalScale) || !number("waterPeriod", &p.waterPeriod)
            || !number("quality", &quality)
            || !number("trackAge", &p.trackAge) || !number("trackSpacing", &p.trackSpacing)
            || !number("markSpacing", &p.markSpacing) || !number("trackBudget", &budget)
            || budget != std::floor(budget) || budget < 2 || budget > 65536
            || quality != std::floor(quality) || quality < 0 || quality > 2) return invalid();
        p.quality = static_cast<int>(quality);
        p.trackBudget = static_cast<int>(budget);
        for (const char* name : {"throughBody", "liveReduction"}) {
            const auto v = object.value(QString::fromLatin1(name));
            if (!v.isUndefined() && !v.isBool()) return invalid();
        }
        if (object.contains(QStringLiteral("throughBody")))
            p.throughBody = object.value(QStringLiteral("throughBody")).toBool();
        if (object.contains(QStringLiteral("liveReduction")))
            p.liveReduction = object.value(QStringLiteral("liveReduction")).toBool();
    }
    if (!validate(result, error)) return false;
    *settings = result;
    return true;
}
}
