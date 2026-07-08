#include "PluginEditor.h"

namespace
{
    juce::String formatSemitones (double v)
    {
        juce::String s (v, 1);
        return (v >= 0.0 ? "+" : "") + s + " st";
    }

    juce::String formatPercent (double v)
    {
        return juce::String ((int) std::lround (v)) + " %";
    }

    // U+00B7 MIDDLE DOT / U+203A SINGLE RIGHT-POINTING ANGLE QUOTATION MARK,
    // spelled as UTF-8 escapes so the glyphs survive regardless of how this
    // source file gets re-encoded down the line.
    const juce::String middleDot (juce::CharPointer_UTF8 ("\xC2\xB7"));
    const juce::String rightAngleQuote (juce::CharPointer_UTF8 ("\xE2\x80\xBA"));
}

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setLookAndFeel (&peachyLookAndFeel);

    auto& apvts = processorRef.apvts;

    // Add the sliders as children (establishing LookAndFeel inheritance) *before*
    // configuring their text box style below — Slider::setTextBoxStyle() creates
    // the text box Label immediately via getLookAndFeel(), which falls back to
    // the global default LookAndFeel if the slider isn't parented yet, silently
    // ignoring PeachyLookAndFeel's colours.
    for (auto* slider : { &pitchSlider, &formantSlider, &driveSlider, &mixSlider })
        addAndMakeVisible (slider);

    // ---- Pitch & Formant knobs ----
    setupKnob (pitchSlider, 25, false);
    setupKnob (formantSlider, 25, false);

    // ---- Drive & Mix knobs ----
    setupKnob (driveSlider, 21, true);
    setupKnob (mixSlider, 21, false);

    for (auto* label : { &pitchNameLabel, &formantNameLabel, &driveNameLabel, &mixNameLabel })
    {
        label->setJustificationType (juce::Justification::centred);
        label->setColour (juce::Label::textColourId, PeachyColours::knobLabel);
        label->setFont (peachyLookAndFeel.getOswaldFontWithKerning (11.5f, 0.22f)); // discreet, wide-tracked
        addAndMakeVisible (label);
    }

    // ---- editable LCD value chips (below each name label) ----
    for (auto* value : { &pitchValueLabel, &formantValueLabel, &driveValueLabel, &mixValueLabel })
    {
        value->setFont (peachyLookAndFeel.getLcdFont (12.0f));
        addAndMakeVisible (value);
    }

    // ---- Header LCD: preset + mode display, < > preset navigation, dice = random ----
    headerLcd.onClick = [this] { showPresetMenu(); };
    headerLcd.onPrevious = [this] { stepPreset (-1); };
    headerLcd.onNext = [this] { stepPreset (1); };
    headerLcd.onDice = [this]
    {
        processorRef.randomiseParameters();
        refreshLcd();
    };
    addAndMakeVisible (headerLcd);

    // ---- Link toggle (its LED is drawn separately, floating above the button) ----
    linkButton.setButtonText ("LINK");
    linkButton.setClickingTogglesState (true);
    linkButton.getProperties().set (PeachyKnobProps::noInternalLed, true);
    addAndMakeVisible (linkButton);

    // ---- Mode buttons (radio-style, single source of truth = the parameter) ----
    for (auto* button : { &modeTransposeButton, &modeQuantizeButton, &modeRobotButton })
    {
        button->setClickingTogglesState (false);
        addAndMakeVisible (button);
    }

    modeTransposeButton.onClick = [this] { modeAttachment->setValueAsCompleteGesture (0.0f); };
    modeQuantizeButton.onClick = [this] { modeAttachment->setValueAsCompleteGesture (1.0f); };
    modeRobotButton.onClick = [this] { modeAttachment->setValueAsCompleteGesture (2.0f); };

    // ---- APVTS attachments ----
    pitchAttachment = std::make_unique<SliderAttachment> (apvts, ParamIDs::pitch, pitchSlider);
    formantAttachment = std::make_unique<SliderAttachment> (apvts, ParamIDs::formant, formantSlider);
    driveAttachment = std::make_unique<SliderAttachment> (apvts, ParamIDs::drive, driveSlider);
    mixAttachment = std::make_unique<SliderAttachment> (apvts, ParamIDs::mix, mixSlider);
    linkAttachment = std::make_unique<ButtonAttachment> (apvts, ParamIDs::link, linkButton);

    // SliderAttachment installs the parameter's own text conversion over
    // textFromValueFunction/valueFromTextFunction, so ours must be (re)applied
    // after the attachments exist, not in setupKnob().
    pitchSlider.textFromValueFunction = formatSemitones;
    pitchSlider.valueFromTextFunction = [] (const juce::String& t) { return t.getDoubleValue(); };
    formantSlider.textFromValueFunction = formatSemitones;
    formantSlider.valueFromTextFunction = [] (const juce::String& t) { return t.getDoubleValue(); };
    driveSlider.textFromValueFunction = formatPercent;
    driveSlider.valueFromTextFunction = [] (const juce::String& t) { return t.getDoubleValue(); };
    mixSlider.textFromValueFunction = formatPercent;
    mixSlider.valueFromTextFunction = [] (const juce::String& t) { return t.getDoubleValue(); };

    // Keep each value chip in sync with its knob, both ways: knob move → chip text,
    // and typing in the chip → knob value (dontSendNotification on the write-back
    // avoids a feedback loop).
    auto wireValue = [] (juce::Slider& slider, LcdValueLabel& label)
    {
        label.setText (slider.getTextFromValue (slider.getValue()), juce::dontSendNotification);
        slider.onValueChange = [&slider, &label]
        {
            label.setText (slider.getTextFromValue (slider.getValue()), juce::dontSendNotification);
        };
        label.onTextChange = [&slider, &label]
        {
            slider.setValue (slider.getValueFromText (label.getText()), juce::sendNotificationSync);
        };
    };

    wireValue (pitchSlider, pitchValueLabel);
    wireValue (formantSlider, formantValueLabel);
    wireValue (driveSlider, driveValueLabel);
    wireValue (mixSlider, mixValueLabel);

    modeAttachment = std::make_unique<juce::ParameterAttachment> (
        *apvts.getParameter (ParamIDs::mode),
        [this] (float newValue) { updateModeButtons ((int) std::lround (newValue)); },
        nullptr);
    modeAttachment->sendInitialUpdate();

    // Bidirectional Link: while Link is on, moving Pitch moves Formant AND moving
    // Formant moves Pitch. Both knobs stay fully interactive. Driven off the
    // parameters (not just slider drags), so host automation and preset loads
    // (e.g. "Trap Robot", which ships with Link on) stay in sync too.
    apvts.addParameterListener (ParamIDs::pitch, this);
    apvts.addParameterListener (ParamIDs::formant, this);
    apvts.addParameterListener (ParamIDs::link, this);

    refreshLcd();
    startTimerHz (4);

    // ~2.2:1 like the mockup, with room for the bigger header + stacked labels
    setSize (960, 410);
}

PluginEditor::~PluginEditor()
{
    processorRef.apvts.removeParameterListener (ParamIDs::pitch, this);
    processorRef.apvts.removeParameterListener (ParamIDs::formant, this);
    processorRef.apvts.removeParameterListener (ParamIDs::link, this);
    setLookAndFeel (nullptr);
}

void PluginEditor::setupKnob (juce::Slider& slider, int numTicks, bool vuStyle)
{
    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    // JUCE requires both rotary angles >= 0; 225°..495° is the positive-form
    // equivalent of a ±135° sweep centred at 12 o'clock (same visual result).
    slider.setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);

    // Knob only — the value is shown in a separate LcdValueLabel below the name
    // label (see the constructor's wireValue), matching the mockup's order.
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);

    slider.getProperties().set (PeachyKnobProps::numTicks, numTicks);
    slider.getProperties().set (PeachyKnobProps::vuStyle, vuStyle);
}

void PluginEditor::updateModeButtons (int modeIndex)
{
    modeTransposeButton.setToggleState (modeIndex == 0, juce::dontSendNotification);
    modeQuantizeButton.setToggleState (modeIndex == 1, juce::dontSendNotification);
    modeRobotButton.setToggleState (modeIndex == 2, juce::dontSendNotification);
    repaint (panelMode);
    refreshLcd(); // reflect mode (and preset) on the LCD immediately, not on the next timer tick
}

void PluginEditor::parameterChanged (const juce::String& parameterID, float newValue)
{
    juce::ignoreUnused (newValue);

    // Audio-thread safe: just record which parameter moved and ping the
    // message thread. No allocation, no locks.
    if (parameterID == ParamIDs::pitch)
        pendingLinkSource.store (0, std::memory_order_relaxed);
    else if (parameterID == ParamIDs::formant)
        pendingLinkSource.store (1, std::memory_order_relaxed);
    else
        pendingLinkSource.store (2, std::memory_order_relaxed);

    triggerAsyncUpdate();
}

void PluginEditor::handleAsyncUpdate()
{
    const int source = pendingLinkSource.exchange (-1, std::memory_order_relaxed);

    if (source == 0)
        handleLinkedParameterChange (ParamIDs::pitch);
    else if (source == 1)
        handleLinkedParameterChange (ParamIDs::formant);
    else if (source == 2)
        handleLinkedParameterChange (ParamIDs::link);
}

void PluginEditor::handleLinkedParameterChange (const juce::String& changedID)
{
    // Repaint the Link LED whenever Link toggles
    if (changedID == ParamIDs::link)
        repaint (linkLedBounds.expanded (12));

    const bool linked = processorRef.apvts.getRawParameterValue (ParamIDs::link)->load() > 0.5f;
    if (! linked)
        return;

    auto* pitchParam = processorRef.apvts.getParameter (ParamIDs::pitch);
    auto* formantParam = processorRef.apvts.getParameter (ParamIDs::formant);
    const float pitchValue = processorRef.apvts.getRawParameterValue (ParamIDs::pitch)->load();
    const float formantValue = processorRef.apvts.getRawParameterValue (ParamIDs::formant)->load();

    // Already equal → nothing to do. This value-equality check is what breaks the
    // pitch→formant→pitch feedback loop: setting the follower to match re-enters
    // here, finds them equal, and stops. No re-entrancy flag needed.
    if (std::abs (pitchValue - formantValue) < 0.001f)
        return;

    // Push the change from whichever knob moved onto the other one. On a plain
    // link-toggle (neither pitch nor formant is the trigger) snap Formant to Pitch.
    auto setNormalised = [] (juce::RangedAudioParameter* param, float plainValue)
    {
        param->setValueNotifyingHost (param->convertTo0to1 (plainValue));
    };

    if (changedID == ParamIDs::formant)
        setNormalised (pitchParam, formantValue);
    else
        setNormalised (formantParam, pitchValue);
}

void PluginEditor::refreshLcd()
{
    const int programIndex = processorRef.getCurrentProgram();
    const auto presetName = processorRef.getProgramName (programIndex).toUpperCase();
    const auto line1 = juce::String::formatted ("%02d ", programIndex + 1) + middleDot + " " + presetName;

    juce::String modeName = "TRANSPOSE";
    if (auto* modeChoice = dynamic_cast<juce::AudioParameterChoice*> (processorRef.apvts.getParameter (ParamIDs::mode)))
        modeName = modeChoice->getCurrentChoiceName().toUpperCase();

    headerLcd.setLines (line1, "MODE " + rightAngleQuote + " " + modeName);
}

void PluginEditor::stepPreset (int delta)
{
    const int count = processorRef.getNumPrograms();
    if (count <= 0)
        return;

    const int next = ((processorRef.getCurrentProgram() + delta) % count + count) % count;
    processorRef.setCurrentProgram (next);
    refreshLcd();
}

void PluginEditor::showPresetMenu()
{
    juce::PopupMenu menu;
    const int numPrograms = processorRef.getNumPrograms();
    const int current = processorRef.getCurrentProgram();

    for (int i = 0; i < numPrograms; ++i)
        menu.addItem (i + 1, processorRef.getProgramName (i), true, i == current);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (headerLcd),
        [this] (int result)
        {
            if (result > 0)
            {
                processorRef.setCurrentProgram (result - 1);
                refreshLcd();
            }
        });
}

void PluginEditor::paint (juce::Graphics& g)
{
    using namespace PeachyColours;
    auto bounds = getLocalBounds().toFloat();

    // ---- outer window chrome ----
    {
        juce::Path windowPath;
        windowPath.addRoundedRectangle (bounds, 14.0f);
        juce::DropShadow (juce::Colours::black.withAlpha (0.55f), 30, { 0, 14 }).drawForPath (g, windowPath);

        juce::ColourGradient grad (windowTop, bounds.getX(), bounds.getY(), windowBottom, bounds.getX(), bounds.getBottom(), false);
        grad.addColour (0.08, windowMid);
        g.setGradientFill (grad);
        g.fillPath (windowPath);
    }

    // ---- header ----
    {
        auto hb = headerBounds.toFloat();
        juce::ColourGradient grad (headerTop, hb.getX(), hb.getY(), headerBottom, hb.getX(), hb.getBottom(), false);
        grad.addColour (0.6, headerMid);
        g.setGradientFill (grad);
        g.fillRect (hb);
        g.setColour (juce::Colour (0xff0c0d0f));
        g.drawLine (hb.getX(), hb.getBottom(), hb.getRight(), hb.getBottom(), 1.0f);

        auto drawScrew = [&] (juce::Point<float> c, float rotationDeg)
        {
            auto screw = juce::Rectangle<float> (11.0f, 11.0f).withCentre (c);
            juce::ColourGradient sg (juce::Colour (0xff5a5b62), screw.getTopLeft(), juce::Colour (0xff26272b), screw.getBottomRight(), true);
            g.setGradientFill (sg);
            g.fillEllipse (screw);

            g.saveState();
            g.addTransform (juce::AffineTransform::rotation (juce::degreesToRadians (rotationDeg), c.x, c.y));
            g.setColour (juce::Colour (0xff141518));
            g.drawLine (c.x - 4.5f, c.y, c.x + 4.5f, c.y, 1.5f);
            g.restoreState();
        };

        drawScrew ({ hb.getX() + 15.5f, hb.getY() + 15.5f }, 40.0f);
        drawScrew ({ hb.getRight() - 15.5f, hb.getY() + 15.5f }, -25.0f);

        // Big "Peachy" logo (white-hint → peach vertical gradient + soft white
        // halo) and its subtitle sharing one baseline, like the mockup.
        const float logoX = hb.getX() + 34.0f;
        const float baselineY = hb.getCentreY() + 17.0f; // sits low so caps fill the header
        auto logoFont = peachyLookAndFeel.getLogoFont (50.0f);

        peachyLookAndFeel.drawLogo (g, "Peachy", logoFont, logoX, baselineY);

        const float logoRight = peachyLookAndFeel.measureTextRight (logoFont, "Peachy", logoX, baselineY);
        auto subtitleFont = peachyLookAndFeel.getOswaldFontWithKerning (12.5f, 0.32f, false);
        peachyLookAndFeel.drawTextAtBaseline (g, "VOCAL PROCESSOR", subtitleFont, logoRight + 20.0f, baselineY,
                                              juce::Colour (0xff8b8d94));
    }

    // ---- section panels ----
    auto drawPanel = [&] (juce::Rectangle<int> pr, const juce::String& title)
    {
        auto pb = pr.toFloat();

        juce::Path panelPath;
        panelPath.addRoundedRectangle (pb, 10.0f);
        juce::DropShadow (juce::Colours::black.withAlpha (0.35f), 10, { 0, 4 }).drawForPath (g, panelPath);

        juce::ColourGradient grad (panelTop, pb.getX(), pb.getY(), panelBottom, pb.getX(), pb.getBottom(), false);
        g.setGradientFill (grad);
        g.fillPath (panelPath);

        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.drawLine (pb.getX() + 10.0f, pb.getY() + 1.0f, pb.getRight() - 10.0f, pb.getY() + 1.0f, 1.0f);

        // panel titles are bold, like every hardware legend on the mockup
        peachyLookAndFeel.drawBoldText (g, title, pb.removeFromTop (22.0f),
                                        peachyLookAndFeel.getOswaldFontWithKerning (10.0f, 0.2f),
                                        sectionTitle, juce::Justification::centred);
    };

    drawPanel (panelPitchFormant, "PITCH & FORMANT");
    drawPanel (panelMode, "MODE");
    drawPanel (panelDriveMix, "COLOR & OUTPUT");

    // ---- independent Link LED, floating just above the LINK button ----
    {
        const bool linked = processorRef.apvts.getRawParameterValue (ParamIDs::link)->load() > 0.5f;
        PeachyLookAndFeel::drawGlowingLed (g, linkLedBounds.toFloat(), linked);
    }

    // ---- footer ----
    {
        auto fb = footerBounds.toFloat();
        juce::ColourGradient grad (juce::Colour (0xff1b1c20), fb.getX(), fb.getY(), juce::Colour (0xff17181b), fb.getX(), fb.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRect (fb);
        g.setColour (juce::Colours::white.withAlpha (0.05f));
        g.drawLine (fb.getX(), fb.getY(), fb.getRight(), fb.getY(), 1.0f);

        g.setColour (footerText);
        g.setFont (peachyLookAndFeel.getOswaldFontWithKerning (9.0f, 0.2f, false));
        g.drawText ("PEACHY AUDIO " + middleDot + " VST3 / AU", fb.reduced (16.0f, 0.0f), juce::Justification::centredLeft, false);
        g.drawText ("24-BIT " + middleDot + " 96 kHz", fb.reduced (16.0f, 0.0f), juce::Justification::centredRight, false);
    }
}

void PluginEditor::layoutKnobColumn (juce::Rectangle<int> column, juce::Slider& slider,
                                    juce::Label& name, LcdValueLabel& value)
{
    column = column.withSizeKeepingCentre (150, column.getHeight());
    slider.setBounds (column.removeFromTop (150)); // knob only
    column.removeFromTop (6);
    // mockup order: NAME above, VALUE chip below
    name.setBounds (column.removeFromTop (15));
    column.removeFromTop (5);
    value.setBounds (column.removeFromTop (24).withSizeKeepingCentre (86, 24));
}

void PluginEditor::resized()
{
    auto area = getLocalBounds();

    headerBounds = area.removeFromTop (88);
    footerBounds = area.removeFromBottom (32);
    area.reduce (18, 12);

    constexpr int gap = 14;
    constexpr int panelModeWidth = 190;
    // Pitch/Formant and Color/Output flank a fixed-width Mode column
    const int sidePanels = area.getWidth() - panelModeWidth - gap * 2;
    const int panelPFWidth = juce::roundToInt ((float) sidePanels * 0.54f);

    panelPitchFormant = area.removeFromLeft (panelPFWidth);
    area.removeFromLeft (gap);
    panelMode = area.removeFromLeft (panelModeWidth);
    area.removeFromLeft (gap);
    panelDriveMix = area;

    // ---- Panel 1: Pitch & Formant + Link ----
    {
        auto content = panelPitchFormant.reduced (12, 0);
        content.removeFromTop (30); // title bar
        content.removeFromBottom (12);

        constexpr int linkColWidth = 64;
        constexpr int knobHeight = 150; // matches layoutKnobColumn's knob area
        auto pitchCol = content.removeFromLeft ((content.getWidth() - linkColWidth) / 2);
        auto linkCol = content.removeFromLeft (linkColWidth);
        auto formantCol = content;

        layoutKnobColumn (pitchCol, pitchSlider, pitchNameLabel, pitchValueLabel);
        layoutKnobColumn (formantCol, formantSlider, formantNameLabel, formantValueLabel);

        // LINK: a thin horizontal pill exactly midway between the two knobs,
        // vertically centred on them. Its LED floats as an independent dot above.
        const int knobCentreY = content.getY() + knobHeight / 2;
        auto linkButtonBounds = juce::Rectangle<int> (52, 24)
                                    .withCentre ({ linkCol.getCentreX(), knobCentreY });
        linkButton.setBounds (linkButtonBounds);

        constexpr int ledSize = 10;
        linkLedBounds = juce::Rectangle<int> (ledSize, ledSize)
                            .withCentre ({ linkCol.getCentreX(), linkButtonBounds.getY() - 12 });
    }

    // ---- Panel 2: Mode ----
    {
        auto content = panelMode.reduced (16, 0);
        content.removeFromTop (30);
        content.removeFromBottom (12);

        constexpr int buttonHeight = 42;
        constexpr int buttonGap = 12;
        auto totalButtonsHeight = buttonHeight * 3 + buttonGap * 2;
        content = content.withSizeKeepingCentre (content.getWidth(), totalButtonsHeight);

        modeTransposeButton.setBounds (content.removeFromTop (buttonHeight));
        content.removeFromTop (buttonGap);
        modeQuantizeButton.setBounds (content.removeFromTop (buttonHeight));
        content.removeFromTop (buttonGap);
        modeRobotButton.setBounds (content.removeFromTop (buttonHeight));
    }

    // ---- Panel 3: Drive & Mix ----
    {
        auto content = panelDriveMix.reduced (12, 0);
        content.removeFromTop (30);
        content.removeFromBottom (12);

        auto driveCol = content.removeFromLeft (content.getWidth() / 2);
        auto mixCol = content;

        layoutKnobColumn (driveCol, driveSlider, driveNameLabel, driveValueLabel);
        layoutKnobColumn (mixCol, mixSlider, mixNameLabel, mixValueLabel);
    }

    // ---- Header LCD (single block), right-aligned and vertically centred ----
    headerLcd.setBounds (juce::Rectangle<int> (260, 48)
                             .withRightX (headerBounds.getRight() - 26)
                             .withY (headerBounds.getCentreY() - 24));
}
