#include "helpers/test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <cmath>

// Renders the editor offscreen and writes a PNG next to the build dir.
// Handy for visually checking LookAndFeel work without launching a DAW:
//   ./Builds/Tests "[snapshot]"
TEST_CASE ("Editor snapshot", "[snapshot]")
{
    runWithinPluginEditor ([] (PluginProcessor& plugin)
    {
        auto* editor = plugin.getActiveEditor();
        REQUIRE (editor != nullptr);

        auto snapshot = editor->createComponentSnapshot (editor->getLocalBounds(), true, 2.0f);

        auto file = juce::File::getCurrentWorkingDirectory().getChildFile ("editor_snapshot.png");
        file.deleteFile();

        juce::FileOutputStream stream (file);
        juce::PNGImageFormat png;
        REQUIRE (png.writeImageToStream (snapshot, stream));

        REQUIRE (file.existsAsFile());
    });
}

// Mission 5: with Link on, Pitch and Formant must track each other in BOTH
// directions. The sync lives in the editor's parameter listener, so this needs
// a live editor and a pumped message loop.
TEST_CASE ("Link is bidirectional", "[link]")
{
    runWithinPluginEditor ([] (PluginProcessor& plugin)
    {
        auto& apvts = plugin.apvts;

        auto setParam = [&] (const char* id, float v)
        {
            auto* p = apvts.getParameter (id);
            p->setValueNotifyingHost (p->convertTo0to1 (v));
        };
        auto getParam = [&] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
        auto pump = [] { juce::MessageManager::getInstance()->runDispatchLoopUntil (60); };

        setParam (ParamIDs::link, 1.0f);
        pump();

        // Move Formant → Pitch follows
        setParam (ParamIDs::formant, 4.0f);
        pump();
        CHECK (std::abs (getParam (ParamIDs::pitch) - 4.0f) < 0.05f);

        // Move Pitch → Formant follows
        setParam (ParamIDs::pitch, -3.0f);
        pump();
        CHECK (std::abs (getParam (ParamIDs::formant) - (-3.0f)) < 0.05f);

        // With Link off, they move independently again
        setParam (ParamIDs::link, 0.0f);
        pump();
        setParam (ParamIDs::pitch, 2.0f);
        pump();
        CHECK (std::abs (getParam (ParamIDs::formant) - (-3.0f)) < 0.05f); // formant unchanged
    });
}

TEST_CASE ("Editor snapshot with Link engaged", "[snapshot]")
{
    runWithinPluginEditor ([] (PluginProcessor& plugin)
    {
        plugin.setCurrentProgram (3); // Trap Robot: pitch -5, link on

        // The Link-follows-Pitch sync is driven by an APVTS::Listener callback,
        // which JUCE marshals to the message thread asynchronously (it may
        // originate from the audio thread during host automation). In a real,
        // running plugin that flushes within a frame; here we pump the loop
        // once so the snapshot reflects the settled state instead of a
        // mid-flight one.
        juce::MessageManager::getInstance()->runDispatchLoopUntil (50);

        auto* editor = plugin.getActiveEditor();
        REQUIRE (editor != nullptr);

        auto snapshot = editor->createComponentSnapshot (editor->getLocalBounds(), true, 2.0f);

        auto file = juce::File::getCurrentWorkingDirectory().getChildFile ("editor_snapshot_link.png");
        file.deleteFile();

        juce::FileOutputStream stream (file);
        juce::PNGImageFormat png;
        REQUIRE (png.writeImageToStream (snapshot, stream));

        REQUIRE (file.existsAsFile());
    });
}
