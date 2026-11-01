// The window every ROMMUSER plugin opens in: vector zoom at any size, themes, and a theme editor.
//
// A plugin's editor is a plain juce::Component laid out once at its base size (its setSize() in
// the constructor). ScalableEditor shows it through a scale transform, so text, knobs, curves and
// strokes are all re-drawn as vectors at the real window size: sharp at 75 % or 250 %, on a
// laptop or a 4K screen. The size and the theme are remembered per plugin (ThemeManager).
//
//     juce::AudioProcessorEditor* MyProcessor::createEditor()
//     {
//         return rommuser::ui::createScalableEditor<MyEditor> (*this);
//     }
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "RommuserLookAndFeel.h"
#include "Theme.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace rommuser::ui
{

namespace detail
{
// Colours set with setColour() (labels, keyboards...) are stored on each component. When the
// theme changes, every one of them that was a theme colour follows its role, keeping its alpha.
// The role is remembered on the component, so two roles sharing a colour never get mixed up.
inline void retheme (juce::Component& c, const Theme& from, const Theme& to)
{
    static const juce::String colourPrefix ("jcclr_"), rolePrefix ("rmmr_role_");
    auto& props = c.getProperties();

    juce::Array<std::pair<int, juce::Colour>> changes;
    for (int i = 0; i < props.size(); ++i)
    {
        const auto name = props.getName (i).toString();
        if (! name.startsWith (colourPrefix))
            continue;

        const auto id = name.substring (colourPrefix.length());
        const auto old = juce::Colour ((juce::uint32) (int) props.getValueAt (i));
        const juce::Identifier roleKey (rolePrefix + id);

        int role = props.contains (roleKey) ? (int) props[roleKey] : from.roleOf (old);
        if (role < 0)
            continue;
        props.set (roleKey, role);
        changes.add ({ id.getHexValue32(), to.c[(size_t) role].withAlpha (old.getAlpha()) });
    }

    for (const auto& [id, colour] : changes)
        c.setColour (id, colour);

    for (auto* child : c.getChildren())
        retheme (*child, from, to);
}
} // namespace detail

//==============================================================================
// One round button at the right of the header: size, theme and the theme editor.
class HeaderMenuButton : public juce::Button
{
public:
    HeaderMenuButton() : juce::Button ("menu")
    {
        setTooltip (tr ("Tamaño y tema de la ventana", "Window size and theme"));
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        auto r = getLocalBounds().toFloat().reduced (1.5f);
        g.setColour (colours::panel.brighter (over || down ? 0.12f : 0.0f));
        g.fillEllipse (r);
        g.setColour (colours::outline.brighter (0.2f));
        g.drawEllipse (r, 1.0f);

        // Three theme dots: the icon shows the theme it controls.
        const auto c = r.getCentre();
        const float d = r.getWidth() * 0.2f, rr = r.getWidth() * 0.2f;
        const juce::Colour dots[] = { colours::accent, colours::good, colours::warn };
        for (int i = 0; i < 3; ++i)
        {
            const float a = juce::MathConstants<float>::twoPi * (float) i / 3.0f - juce::MathConstants<float>::halfPi;
            g.setColour (dots[i]);
            g.fillEllipse (juce::Rectangle<float> (d, d).withCentre ({ c.x + rr * std::cos (a), c.y + rr * std::sin (a) }));
        }
    }
};

//==============================================================================
// Make or edit a theme: click a colour, pick it, see the plugin change live, save with a name.
class ThemeEditorPanel : public juce::Component, private juce::ChangeListener
{
public:
    ThemeEditorPanel (ThemeManager& m, Theme start, std::function<void()> onClose)
        : manager (m), original (m.getCurrent()), theme (std::move (start)), close (std::move (onClose))
    {
        title.setText (theme.isBuiltIn() ? tr ("Crear tema", "Create theme") : tr ("Editar tema", "Edit theme"), juce::dontSendNotification);
        title.setFont (juce::FontOptions (18.0f, juce::Font::bold));
        addAndMakeVisible (title);

        if (theme.isBuiltIn())
        {
            theme.name = tr ("Mi tema", "My theme");
            theme.onlyThisPlugin = true;
        }

        nameLabel.setText (tr ("Nombre", "Name"), juce::dontSendNotification);
        nameLabel.setColour (juce::Label::textColourId, colours::textDim);
        addAndMakeVisible (nameLabel);
        name.setText (theme.name, false);
        name.setFont (juce::Font (juce::FontOptions (14.0f)));
        name.setIndents (6, 4);
        addAndMakeVisible (name);

        for (int i = 0; i < Theme::numRoles; ++i)
        {
            auto& row = rows[(size_t) i];
            row.label.setText (Theme::roleName (i), juce::dontSendNotification);
            row.label.setFont (juce::FontOptions (13.0f));
            addAndMakeVisible (row.label);
            row.swatch.setTooltip (tr ("Clic para elegir el color", "Click to pick the colour"));
            row.swatch.onClick = [this, i] { pickColour (i); };
            addAndMakeVisible (row.swatch);
        }

        onlyHere.setButtonText (tr ("Solo para este plugin", "Only for this plugin") + " (" + ThemeManager::pluginName() + ")");
        onlyHere.setToggleState (theme.onlyThisPlugin, juce::dontSendNotification);
        onlyHere.setTooltip (tr ("Apagado: el tema aparece en todos los plugins ROMMUSER.",
                                 "Off: the theme shows up in every ROMMUSER plugin."));
        addAndMakeVisible (onlyHere);

        save.setButtonText (tr ("Guardar", "Save"));
        save.onClick = [this] { saveTheme(); };
        cancel.setButtonText (tr ("Cancelar", "Cancel"));
        cancel.onClick = [this] { close(); };
        for (auto* b : { &save, &cancel })
            addAndMakeVisible (*b);
        save.setToggleState (true, juce::dontSendNotification); // drawn in the accent colour

        hint.setText (tr ("Los temas se guardan en Documentos/ROMMUSER/Themes. Puedes compartirlos.",
                          "Themes are saved in Documents/ROMMUSER/Themes. You can share them."), juce::dontSendNotification);
        hint.setFont (juce::FontOptions (11.0f));
        hint.setColour (juce::Label::textColourId, colours::textDim);
        hint.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (hint);

        manager.preview (theme);
        refreshSwatches();
    }

    // Closing without saving puts the saved theme back.
    ~ThemeEditorPanel() override
    {
        if (activeSelector != nullptr)
            activeSelector->removeChangeListener (this);
        if (! saved)
            manager.preview (original);
    }

    void paint (juce::Graphics& g) override
    {
        // Dim the plugin behind, keep it visible so changes can be seen live.
        g.fillAll (juce::Colours::black.withAlpha (0.35f));
        g.setColour (colours::panel);
        g.fillRoundedRectangle (card.toFloat(), 10.0f);
        g.setColour (colours::outline.brighter (0.2f));
        g.drawRoundedRectangle (card.toFloat().reduced (0.5f), 10.0f, 1.0f);
    }

    void resized() override
    {
        // Cards sit on the right so the plugin's own controls stay in view while colours change.
        const int w = juce::jmin (380, getWidth() - 24), h = juce::jmin (440, getHeight() - 24);
        card = juce::Rectangle<int> (getWidth() - w - 12, (getHeight() - h) / 2, w, h);
        auto r = card.reduced (16, 12);

        title.setBounds (r.removeFromTop (28));
        auto nameRow = r.removeFromTop (30);
        nameLabel.setBounds (nameRow.removeFromLeft (70));
        name.setBounds (nameRow.reduced (0, 2));
        r.removeFromTop (8);

        hint.setBounds (r.removeFromBottom (30));
        auto buttons = r.removeFromBottom (32);
        cancel.setBounds (buttons.removeFromRight (100));
        buttons.removeFromRight (8);
        save.setBounds (buttons.removeFromRight (100));
        r.removeFromBottom (4);
        onlyHere.setBounds (r.removeFromBottom (28));

        const int rowH = juce::jmin (32, r.getHeight() / Theme::numRoles);
        for (auto& row : rows)
        {
            auto line = r.removeFromTop (rowH);
            row.swatch.setBounds (line.removeFromRight (64).reduced (0, 3));
            row.label.setBounds (line);
        }
    }

private:
    struct Swatch : public juce::Button
    {
        Swatch() : juce::Button ("colour") { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
        juce::Colour colour;
        void paintButton (juce::Graphics& g, bool over, bool) override
        {
            auto r = getLocalBounds().toFloat().reduced (1.0f);
            g.setColour (colour);
            g.fillRoundedRectangle (r, 5.0f);
            g.setColour (over ? colours::text : colours::outline.brighter (0.3f));
            g.drawRoundedRectangle (r, 5.0f, over ? 2.0f : 1.0f);
        }
    };

    struct Row
    {
        juce::Label label;
        Swatch swatch;
    };

    void refreshSwatches()
    {
        for (int i = 0; i < Theme::numRoles; ++i)
        {
            rows[(size_t) i].swatch.colour = theme.c[(size_t) i];
            rows[(size_t) i].swatch.repaint();
        }
    }

    void pickColour (int role)
    {
        editingRole = role;
        auto selector = std::make_unique<juce::ColourSelector> (juce::ColourSelector::showColourspace
                                                                 | juce::ColourSelector::showSliders
                                                                 | juce::ColourSelector::editableColour);
        selector->setCurrentColour (theme.c[(size_t) role], juce::dontSendNotification);
        selector->setSize (300, 300);
        selector->addChangeListener (this);
        activeSelector = selector.get();

        auto* parent = getParentComponent() != nullptr ? getParentComponent() : this;
        auto& swatch = rows[(size_t) role].swatch;
        juce::CallOutBox::launchAsynchronously (std::move (selector), parent->getLocalArea (&swatch, swatch.getLocalBounds()), parent);
    }

    void changeListenerCallback (juce::ChangeBroadcaster* source) override
    {
        if (auto* selector = dynamic_cast<juce::ColourSelector*> (source); selector != nullptr && editingRole >= 0)
        {
            theme.c[(size_t) editingRole] = selector->getCurrentColour();
            manager.preview (theme);
            refreshSwatches();
        }
    }

    void saveTheme()
    {
        theme.name = name.getText().trim();
        theme.onlyThisPlugin = onlyHere.getToggleState();
        if (manager.save (theme))
        {
            saved = true;
            close();
            return;
        }
        hint.setText (tr ("No se pudo guardar el tema en ", "Could not save the theme in ") + ThemeManager::themesFolder().getFullPathName(),
                      juce::dontSendNotification);
        hint.setColour (juce::Label::textColourId, colours::warn);
    }

    ThemeManager& manager;
    const Theme original;
    Theme theme;
    std::function<void()> close;
    int editingRole = -1;
    bool saved = false;
    juce::Component::SafePointer<juce::ColourSelector> activeSelector;

    juce::Rectangle<int> card;
    juce::Label title, nameLabel, hint;
    juce::TextEditor name;
    std::array<Row, Theme::numRoles> rows;
    juce::ToggleButton onlyHere;
    juce::TextButton save, cancel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ThemeEditorPanel)
};

//==============================================================================
class ScalableEditor : public juce::AudioProcessorEditor, private juce::ChangeListener
{
public:
    ScalableEditor (juce::AudioProcessor& p, std::unique_ptr<juce::Component> editor)
        : AudioProcessorEditor (p), content (std::move (editor)), applied (themes->getCurrent())
    {
        baseWidth = juce::jmax (1, content->getWidth());
        baseHeight = juce::jmax (1, content->getHeight());
        addAndMakeVisible (*content);

        // Parts built before the editor set its look-and-feel (slider value boxes) are rebuilt
        // with it, so they use the theme too.
        content->sendLookAndFeelChange();

        // The button lives inside the editor so it scales with the header it sits in.
        menuButton.onClick = [this] { showMenu(); };
        content->addAndMakeVisible (menuButton);
        const int b = 26;
        menuButton.setBounds (baseWidth - 20 - b, 11, b, b);

        themes->addChangeListener (this);

        setResizable (true, true);
        setResizeLimits (baseWidth * 3 / 4, baseHeight * 3 / 4, baseWidth * 5 / 2, baseHeight * 5 / 2);
        if (auto* c = getConstrainer())
            c->setFixedAspectRatio ((double) baseWidth / baseHeight);
        const float zoom = juce::jlimit (0.75f, 2.5f, themes->getZoom());
        setSize (juce::roundToInt (baseWidth * zoom), juce::roundToInt (baseHeight * zoom));
        ready = true;
    }

    ~ScalableEditor() override
    {
        themes->removeChangeListener (this);
        themePanel.reset();
    }

    void paint (juce::Graphics& g) override { g.fillAll (colours::background); }

    void resized() override
    {
        const float s = (float) getWidth() / (float) baseWidth;
        content->setTransform (juce::AffineTransform::scale (s));
        // Only remember sizes the user chose, not the ones passed through while opening.
        if (ready && std::abs (s - themes->getZoom()) > 0.001f)
            themes->setZoom (s);
    }

    juce::Component& getContent() { return *content; }

    // Opens the theme editor on a copy of the current theme.
    void showThemeEditor() { openThemeEditor (themes->getCurrent(), true); }

private:
    void setZoom (float z)
    {
        setSize (juce::roundToInt ((float) baseWidth * z), juce::roundToInt ((float) baseHeight * z));
    }

    void showMenu()
    {
        juce::PopupMenu size;
        const float current = (float) getWidth() / (float) baseWidth;
        for (int pct : { 75, 100, 125, 150, 175, 200, 250 })
            size.addItem (juce::String (pct) + " %", true, std::abs (current * 100.0f - (float) pct) < 1.0f,
                          [this, pct] { setZoom ((float) pct / 100.0f); });

        const auto all = themes->allThemes();
        const auto& now = themes->getCurrent();
        juce::PopupMenu themeMenu;
        bool userSection = false;
        for (const auto& t : all)
        {
            if (! t.isBuiltIn() && ! userSection)
            {
                themeMenu.addSeparator();
                themeMenu.addSectionHeader (tr ("Mis temas", "My themes"));
                userSection = true;
            }
            const bool ticked = t.name == now.name && t.file == now.file;
            themeMenu.addItem (t.displayName() + (t.onlyThisPlugin ? juce::String() : juce::String::fromUTF8 ("  ·  ") + tr ("todos", "all")),
                               true, ticked, [this, t] { themes->setTheme (t); });
        }

        juce::PopupMenu menu;
        menu.addSectionHeader (ThemeManager::pluginName());
        menu.addSubMenu (tr ("Tamaño", "Size"), size);
        menu.addSubMenu (tr ("Tema", "Theme"), themeMenu);
        menu.addSeparator();
        menu.addItem (tr ("Crear tema nuevo...", "Create a new theme..."), [this] { openThemeEditor (themes->getCurrent(), true); });
        menu.addItem (tr ("Editar este tema...", "Edit this theme..."), ! now.isBuiltIn(), false,
                      [this] { openThemeEditor (themes->getCurrent(), false); });
        menu.addItem (tr ("Abrir carpeta de temas", "Open themes folder"), []
        {
            auto folder = ThemeManager::themesFolder();
            folder.createDirectory();
            folder.startAsProcess();
        });

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&menuButton));
    }

    void openThemeEditor (Theme start, bool asNew)
    {
        if (asNew)
        {
            // A new theme starts from the colours on screen and never overwrites a file.
            start.file = juce::File();
            start.name = {};
        }
        themePanel = std::make_unique<ThemeEditorPanel> (*themes, start, [this]
        {
            // Called from a button inside the panel: delete it after the click has finished.
            juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<ScalableEditor> (this)]
            {
                if (safe != nullptr)
                    safe->themePanel.reset();
            });
        });
        content->addAndMakeVisible (*themePanel);
        themePanel->setBounds (content->getLocalBounds());
    }

    void changeListenerCallback (juce::ChangeBroadcaster*) override
    {
        const auto& next = themes->getCurrent();
        if (auto* lnf = dynamic_cast<LookAndFeel*> (&content->getLookAndFeel()))
            lnf->applyTheme();
        content->sendLookAndFeelChange();
        detail::retheme (*content, applied, next);
        applied = next;
        content->repaint();
        repaint();
    }

    juce::SharedResourcePointer<ThemeManager> themes;
    std::unique_ptr<juce::Component> content;
    Theme applied;
    HeaderMenuButton menuButton;
    std::unique_ptr<ThemeEditorPanel> themePanel;
    int baseWidth = 1, baseHeight = 1;
    bool ready = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScalableEditor)
};

// The saved theme is applied before the editor is built, so its colours are right from the start.
template <typename Editor, typename Processor>
juce::AudioProcessorEditor* createScalableEditor (Processor& processor)
{
    juce::SharedResourcePointer<ThemeManager> themes;
    return new ScalableEditor (processor, std::make_unique<Editor> (processor));
}

} // namespace rommuser::ui
