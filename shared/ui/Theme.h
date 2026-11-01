// Themes for every ROMMUSER plugin: the colours the UI draws with, the built-in themes, and the
// user's own themes (small JSON files anyone can make, edit and share).
//
// The live colours sit in rommuser::ui::colours and are read at paint time, so switching theme
// only needs a repaint. ThemeManager loads the saved choice for this plugin, lists the themes
// and tells every open editor when the theme changes.
//
// Theme files: Documents/ROMMUSER/Themes/*.json         (shown in every plugin)
//              Documents/ROMMUSER/Themes/<Plugin>/*.json (shown only in that plugin)
//
//     { "name": "My theme", "background": "#15161A", "panel": "#1F2127", "outline": "#2E3139",
//       "text": "#E8E9EE", "textDim": "#8A8F9C", "accent": "#F2A23A", "good": "#58C98B",
//       "warn": "#E5534B" }
//
// Missing colours fall back to the ROMMUSER theme; "#AARRGGBB" also works.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>

namespace rommuser::ui
{

// The colours the UI is drawing with right now (set by ThemeManager).
namespace colours
{
    inline juce::Colour background { 0xff15161a };
    inline juce::Colour panel      { 0xff1f2127 };
    inline juce::Colour outline    { 0xff2e3139 };
    inline juce::Colour text       { 0xffe8e9ee };
    inline juce::Colour textDim    { 0xff8a8f9c };
    inline juce::Colour accent     { 0xfff2a23a }; // warm orange
    inline juce::Colour good       { 0xff58c98b };
    inline juce::Colour warn       { 0xffe5534b };
}

inline bool isSpanish()
{
    static const bool spanish = juce::SystemStats::getUserLanguage().startsWithIgnoreCase ("es")
                             || juce::SystemStats::getDisplayLanguage().startsWithIgnoreCase ("es");
    return spanish;
}

// tr ("Entrada", "Input")
inline juce::String tr (const char* spanish, const char* english)
{
    return juce::String::fromUTF8 (isSpanish() ? spanish : english);
}

struct Theme
{
    static constexpr int numRoles = 8;

    juce::String name;
    std::array<juce::Colour, numRoles> c;
    juce::File file;          // empty for built-in themes
    bool onlyThisPlugin = false;

    // Role order is fixed: it is the order of the JSON keys and of the theme editor rows.
    static const char* key (int role)
    {
        static const char* keys[] = { "background", "panel", "outline", "text", "textDim", "accent", "good", "warn" };
        return keys[role];
    }

    static juce::String roleName (int role)
    {
        switch (role)
        {
            case 0:  return tr ("Fondo", "Background");
            case 1:  return tr ("Paneles", "Panels");
            case 2:  return tr ("Bordes y líneas", "Borders and lines");
            case 3:  return tr ("Texto", "Text");
            case 4:  return tr ("Texto suave", "Soft text");
            case 5:  return tr ("Acento (perillas, curvas)", "Accent (knobs, curves)");
            case 6:  return tr ("Bien (medidores en verde)", "Good (green meters)");
            default: return tr ("Alerta (rojo, clip)", "Warning (red, clip)");
        }
    }

    bool isBuiltIn() const { return file == juce::File(); }

    void apply() const
    {
        colours::background = c[0];
        colours::panel = c[1];
        colours::outline = c[2];
        colours::text = c[3];
        colours::textDim = c[4];
        colours::accent = c[5];
        colours::good = c[6];
        colours::warn = c[7];
    }

    static Theme make (const char* name, std::array<juce::uint32, numRoles> argb)
    {
        Theme t;
        t.name = juce::String::fromUTF8 (name);
        for (int i = 0; i < numRoles; ++i)
            t.c[(size_t) i] = juce::Colour (argb[(size_t) i]);
        return t;
    }

    // Own designs. Keep "ROMMUSER" first: it is the default and the fallback for missing colours.
    static const std::vector<Theme>& builtIns()
    {
        static const std::vector<Theme> themes {
            //                         background  panel       outline     text        textDim     accent      good        warn
            make ("ROMMUSER",        { 0xff15161a, 0xff1f2127, 0xff2e3139, 0xffe8e9ee, 0xff8a8f9c, 0xfff2a23a, 0xff58c98b, 0xffe5534b }),
            make ("Midnight",        { 0xff0e131d, 0xff172031, 0xff263247, 0xffe4ebf7, 0xff7f8ba5, 0xff4fb3ff, 0xff52d6a0, 0xffff5d6c }),
            make ("Neon",            { 0xff110c18, 0xff1b1426, 0xff2e2340, 0xfff2ebff, 0xff9a8db4, 0xffff4fd8, 0xff38f0c4, 0xffff5a5a }),
            make ("Forest",          { 0xff101611, 0xff19211a, 0xff29352a, 0xffe5eee3, 0xff86958a, 0xffa3d65c, 0xff5bd1a0, 0xffef6b4f }),
            make ("Sunset",          { 0xff1a1214, 0xff251a1d, 0xff3a2a2e, 0xfff5e9e6, 0xffa08e8b, 0xffff7a59, 0xfff2c14e, 0xffe5484d }),
            make ("Light",           { 0xffeceff4, 0xffffffff, 0xffc9cfd9, 0xff1c2030, 0xff687086, 0xffdc7400, 0xff1d9a5a, 0xffd5372f }),
            make ("High contrast",   { 0xff000000, 0xff0f0f0f, 0xff6a6a6a, 0xffffffff, 0xffd0d0d0, 0xffffd400, 0xff00e676, 0xffff3b30 }),
        };
        return themes;
    }

    // Built-in names are stored in English and shown in the user's language.
    juce::String displayName() const
    {
        if (! isBuiltIn())         return name;
        if (name == "Midnight")    return tr ("Medianoche", "Midnight");
        if (name == "Neon")        return tr ("Neón", "Neon");
        if (name == "Forest")      return tr ("Bosque", "Forest");
        if (name == "Sunset")      return tr ("Atardecer", "Sunset");
        if (name == "Light")       return tr ("Claro", "Light");
        if (name == "High contrast") return tr ("Alto contraste", "High contrast");
        return name;
    }

    juce::String toJson() const
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("name", name);
        for (int i = 0; i < numRoles; ++i)
            obj->setProperty (key (i), "#" + c[(size_t) i].toDisplayString (c[(size_t) i].getAlpha() != 255));
        return juce::JSON::toString (juce::var (obj));
    }

    // Returns false (and leaves the theme untouched) if the text is not a theme.
    bool fromJson (const juce::String& json)
    {
        const auto v = juce::JSON::parse (json);
        auto* obj = v.getDynamicObject();
        if (obj == nullptr)
            return false;

        Theme t = builtIns().front();
        t.name = obj->getProperty ("name").toString().trim();
        bool any = false;
        for (int i = 0; i < numRoles; ++i)
        {
            const auto s = obj->getProperty (key (i)).toString().trim().trimCharactersAtStart ("#");
            if (s.length() == 6 || s.length() == 8)
            {
                const auto argb = (juce::uint32) s.getHexValue64();
                t.c[(size_t) i] = juce::Colour (s.length() == 6 ? (0xff000000u | argb) : argb);
                any = true;
            }
        }
        if (! any)
            return false;

        t.file = file;
        t.onlyThisPlugin = onlyThisPlugin;
        *this = t;
        return true;
    }

    int roleOf (juce::Colour colour) const
    {
        for (int i = 0; i < numRoles; ++i)
            if ((c[(size_t) i].getARGB() & 0x00ffffffu) == (colour.getARGB() & 0x00ffffffu))
                return i;
        return -1;
    }
};

//==============================================================================
class ThemeManager : public juce::ChangeBroadcaster
{
public:
    ThemeManager()
    {
        juce::PropertiesFile::Options o;
        o.applicationName = pluginName();
        o.filenameSuffix = ".settings";
        o.folderName = "ROMMUSER";
        o.osxLibrarySubFolder = "Application Support";
        o.millisecondsBeforeSaving = 1000;
        settings = std::make_unique<juce::PropertiesFile> (o);

        // ROMMUSER_THEME overrides the saved theme (README screenshots).
        const auto forced = juce::SystemStats::getEnvironmentVariable ("ROMMUSER_THEME", {});
        const auto wanted = forced.isNotEmpty() ? forced : settings->getValue ("theme", "ROMMUSER");
        current = Theme::builtIns().front();
        for (const auto& t : allThemes())
            if (t.name == wanted)
                current = t;
        current.apply();
    }

    ~ThemeManager() override { settings->saveIfNeeded(); }

    static juce::String pluginName()
    {
       #ifdef JucePlugin_Name
        return JucePlugin_Name;
       #else
        return "ROMMUSER";
       #endif
    }

    static juce::File themesFolder()
    {
        return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("ROMMUSER").getChildFile ("Themes");
    }

    static juce::File pluginThemesFolder()
    {
        return themesFolder().getChildFile (juce::File::createLegalFileName (pluginName()));
    }

    // Built-ins first, then this plugin's own themes, then the shared ones. Re-read every call so
    // a theme someone just dropped in the folder shows up the next time the menu opens.
    std::vector<Theme> allThemes() const
    {
        auto themes = Theme::builtIns();
        auto scan = [&themes] (const juce::File& folder, bool onlyThis)
        {
            auto files = folder.findChildFiles (juce::File::findFiles, false, "*.json");
            files.sort();
            for (const auto& f : files)
            {
                Theme t;
                t.file = f;
                t.onlyThisPlugin = onlyThis;
                if (f.getSize() < 64 * 1024 && t.fromJson (f.loadFileAsString()))
                {
                    if (t.name.isEmpty())
                        t.name = f.getFileNameWithoutExtension();
                    themes.push_back (t);
                }
            }
        };
        scan (pluginThemesFolder(), true);
        scan (themesFolder(), false);
        return themes;
    }

    const Theme& getCurrent() const { return current; }

    // Applies and remembers the theme for this plugin.
    void setTheme (const Theme& t)
    {
        preview (t);
        settings->setValue ("theme", t.name);
    }

    // Applies without remembering it (live preview while editing a theme).
    void preview (const Theme& t)
    {
        current = t;
        current.apply();
        sendSynchronousChangeMessage();
    }

    // Writes a user theme and selects it. Returns false if the file could not be written.
    bool save (Theme t)
    {
        const auto folder = t.onlyThisPlugin ? pluginThemesFolder() : themesFolder();
        if (! folder.createDirectory())
            return false;

        if (t.name.trim().isEmpty())
            t.name = tr ("Mi tema", "My theme");

        // Built-in names are taken: a copy of "Neon" saved as is becomes "Neon 2".
        auto taken = [this, &t] (const juce::String& n)
        {
            for (const auto& other : allThemes())
                if (other.name == n && (other.isBuiltIn() || other.file != t.file))
                    return true;
            return false;
        };
        if (taken (t.name))
        {
            int n = 2;
            while (taken (t.name + " " + juce::String (n)))
                ++n;
            t.name << " " << n;
        }

        auto target = t.file;
        if (target == juce::File() || target.getParentDirectory() != folder)
        {
            if (t.file != juce::File())
                t.file.deleteFile(); // moved between "this plugin" and "all plugins"
            target = folder.getNonexistentChildFile (juce::File::createLegalFileName (t.name), ".json", false);
        }

        if (! target.replaceWithText (t.toJson()))
            return false;

        t.file = target;
        setTheme (t);
        return true;
    }

    float getZoom() const { return (float) settings->getDoubleValue ("zoom", 1.0); }
    void setZoom (float z) { settings->setValue ("zoom", (double) z); }

private:
    std::unique_ptr<juce::PropertiesFile> settings;
    Theme current;
};

} // namespace rommuser::ui
