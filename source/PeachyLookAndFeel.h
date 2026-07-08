#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <melatonin_blur/melatonin_blur.h>
#include "BinaryData.h"

// Palette extracted from the plugin's HTML/CSS mockup (dark anthracite chassis,
// peach/orange accent, LCD amber readouts). Shared between the LookAndFeel and
// the editor's chrome painting so both stay in sync with one source of truth.
namespace PeachyColours
{
    const juce::Colour windowTop        { 0xff2b2c31 };
    const juce::Colour windowMid        { 0xff222327 };
    const juce::Colour windowBottom     { 0xff1d1e22 };
    const juce::Colour headerTop        { 0xff33343a };
    const juce::Colour headerMid        { 0xff26272c };
    const juce::Colour headerBottom     { 0xff202126 };
    const juce::Colour panelTop         { 0xff2c2d33 };
    const juce::Colour panelBottom      { 0xff242529 };
    const juce::Colour bezelTop         { 0xff43444b };
    const juce::Colour bezelBottom     { 0xff111215 };
    const juce::Colour knurlBase        { 0xff34353b };
    const juce::Colour knurlEdge        { 0xff1e1f24 };
    const juce::Colour centreDiscTop    { 0xff33343a };
    const juce::Colour centreDiscEdge   { 0xff141519 };
    const juce::Colour tickInactive     { 0xff4a4b51 };
    const juce::Colour accent           { 0xffff9e6d };
    const juce::Colour logoTop          { 0xffffd3b0 }; // mockup CSS gradient stops
    const juce::Colour logoMid          { 0xffff9e6d };
    const juce::Colour logoBottom       { 0xffff7e5f };
    const juce::Colour lcdBg            { 0xff101408 }; // the mockup's main LCD fill
    const juce::Colour lcdBgBottom      { 0xff161c0c };
    const juce::Colour valueBg          { 0xff141508 }; // the small value-chip fill
    const juce::Colour lcdBorder        { 0xff0a0b08 };
    const juce::Colour lcdText          { 0xffffb98a };
    const juce::Colour lcdTextDim       { 0xffa06b4a };
    const juce::Colour knobLabel        { 0xffc9cbd1 };
    const juce::Colour sectionTitle     { 0xff8a8c93 };
    const juce::Colour footerText       { 0xff5c5e65 };
    const juce::Colour buttonOffTop     { 0xff3b3c42 };
    const juce::Colour buttonOffBottom  { 0xff26272c };
    const juce::Colour buttonOnTop      { 0xff232428 };
    const juce::Colour buttonOnBottom   { 0xff2a2b30 };
    const juce::Colour ledOff           { 0xff2a2b2f };
    const juce::Colour buttonLabelOff   { 0xff9a9ca3 };
    const juce::Colour buttonLabelOn    { 0xffffd6b8 };

    // The signature glow: a peach aura around lit elements (the mockup's box-
    // and text-shadows are all accent-tinted). White only ever appears as a
    // 1px inner highlight on the hottest elements (LED ring, pointer core).
    const juce::Colour accentGlow       { 0x8cff9e6d }; // accent @ ~0.55 alpha
    const juce::Colour innerHighlight   { 0x80ffffff }; // 1px white inset @ 0.5 alpha
}

// Component property keys read by PeachyLookAndFeel::drawRotarySlider, set on
// each juce::Slider via getProperties() so one generic paint routine can
// handle every knob (tick count, VU-style glow ramp for Drive).
namespace PeachyKnobProps
{
    static constexpr const char* numTicks = "peachyNumTicks";
    static constexpr const char* vuStyle = "peachyVuStyle";
    static constexpr const char* noInternalLed = "peachyNoInternalLed";
}

class PeachyLookAndFeel;

// A single dark LCD block: two lines of text (bright preset line + dim mode
// line), < / > preset navigation arrows and a dice button that rolls random
// parameters. melatonin::InnerShadow gives it real inset depth.
class LcdDisplay : public juce::Component
{
public:
    std::function<void()> onClick;     // main text area → preset menu
    std::function<void()> onPrevious;  // '<'
    std::function<void()> onNext;      // '>'
    std::function<void()> onDice;      // dice icon → randomise

    void setLines (const juce::String& line1, const juce::String& line2)
    {
        if (topLine != line1 || bottomLine != line2)
        {
            topLine = line1;
            bottomLine = line2;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent& e) override;

    void mouseEnter (const juce::MouseEvent&) override
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

private:
    juce::String topLine, bottomLine;
    juce::Rectangle<float> textZone, prevZone, nextZone, diceZone;
    melatonin::InnerShadow innerShadow { PeachyColours::lcdBorder, 6, { 0, 2 }, 0 };
    melatonin::DropShadow textHalo { PeachyColours::accentGlow, 6, { 0, 0 }, 0 };
};

// The small amber value chip under each knob (+0.9 st, 75 %, ...). Editable by
// double-click; painted as a rounded LCD block (#141508) with Share Tech Mono
// peach digits and a faint white halo, matching the mockup.
class LcdValueLabel : public juce::Label
{
public:
    LcdValueLabel()
    {
        setJustificationType (juce::Justification::centred);
        setEditable (false, true, false); // double-click to type a value
        setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Label::textColourId, PeachyColours::lcdText);
        setColour (juce::TextEditor::textColourId, PeachyColours::lcdText);
        setColour (juce::TextEditor::backgroundColourId, PeachyColours::valueBg);
        setColour (juce::TextEditor::highlightColourId, PeachyColours::accent.withAlpha (0.4f));
        setColour (juce::TextEditor::highlightedTextColourId, juce::Colours::black);
        setColour (juce::CaretComponent::caretColourId, PeachyColours::accent);
        setMinimumHorizontalScale (1.0f);
    }

    void paint (juce::Graphics& g) override;

private:
    melatonin::DropShadow textHalo { PeachyColours::accentGlow, 5, { 0, 0 }, 0 };
};

// Recreates the mockup's skeuomorphic hardware: knurled dark-metal knobs with a
// neon pointer, LCD-styled menus and chips, faint white halos around every lit
// element. Loads the three custom typefaces (logo / LCD / labels) from
// BinaryData with sensible system fallbacks.
class PeachyLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PeachyLookAndFeel()
    {
        logoTypeface = loadTypeface (BinaryData::RighteousRegular_ttf, BinaryData::RighteousRegular_ttfSize);
        lcdTypeface = loadTypeface (BinaryData::ShareTechMonoRegular_ttf, BinaryData::ShareTechMonoRegular_ttfSize);
        labelTypeface = loadTypeface (BinaryData::OswaldRegular_ttf, BinaryData::OswaldRegular_ttfSize);

        setColour (juce::ResizableWindow::backgroundColourId, PeachyColours::windowBottom);

        setColour (juce::Slider::textBoxTextColourId, PeachyColours::lcdText);
        setColour (juce::Slider::textBoxBackgroundColourId, PeachyColours::valueBg);
        setColour (juce::Slider::textBoxOutlineColourId, PeachyColours::lcdBorder);
        setColour (juce::Slider::textBoxHighlightColourId, PeachyColours::accent.withAlpha (0.4f));
        setColour (juce::TextEditor::textColourId, PeachyColours::lcdText);
        setColour (juce::TextEditor::highlightColourId, PeachyColours::accent.withAlpha (0.4f));
        setColour (juce::TextEditor::highlightedTextColourId, juce::Colours::black);
        setColour (juce::CaretComponent::caretColourId, PeachyColours::accent);
    }

    // ------------------------------------------------------------------ fonts
    juce::Font getLogoFont (float size) const
    {
        if (logoTypeface != nullptr)
            return juce::Font (juce::FontOptions (logoTypeface)).withHeight (size);

        return juce::Font (juce::FontOptions (size)).boldened();
    }

    juce::Font getLcdFont (float size) const
    {
        if (lcdTypeface != nullptr)
            return juce::Font (juce::FontOptions (lcdTypeface)).withHeight (size);

        return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), size, juce::Font::plain));
    }

    juce::Font getOswaldFont (float size, bool bold = true) const
    {
        auto f = labelTypeface != nullptr ? juce::Font (juce::FontOptions (labelTypeface)).withHeight (size)
                                          : juce::Font (juce::FontOptions (size));
        return bold ? f.boldened() : f;
    }

    juce::Font getOswaldFontWithKerning (float size, float kerning, bool bold = true) const
    {
        return getOswaldFont (size, bold).withExtraKerningFactor (kerning);
    }

    // ------------------------------------------------------------- bold text
    // Custom typefaces have no real bold cut, so bold is synthesised by filling
    // AND stroking the glyph path — reliable at every size, on every platform.
    void drawBoldText (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area,
                       const juce::Font& font, juce::Colour colour, juce::Justification justification,
                       float boldness = 0.55f) const
    {
        juce::GlyphArrangement arrangement;
        arrangement.addFittedText (font, text, area.getX(), area.getY(),
                                   area.getWidth(), area.getHeight(), justification, 1);

        juce::Path path;
        arrangement.createPath (path);

        g.setColour (colour);
        g.fillPath (path);

        if (boldness > 0.0f)
            g.strokePath (path, juce::PathStrokeType (boldness));
    }

    // Bold text with the signature faint white halo (active mode labels, etc.)
    void drawBoldGlowText (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area,
                           const juce::Font& font, juce::Colour colour, juce::Justification justification)
    {
        juce::GlyphArrangement arrangement;
        arrangement.addFittedText (font, text, area.getX(), area.getY(),
                                   area.getWidth(), area.getHeight(), justification, 1);

        juce::Path path;
        arrangement.createPath (path);

        textHalo.render (g, path);

        g.setColour (colour);
        g.fillPath (path);
        g.strokePath (path, juce::PathStrokeType (0.55f));
    }

    // ------------------------------------------------------------------ logo
    // "Peachy": vertical gradient from a hint of white down into the peach
    // orange (mockup stops), wrapped in a soft diffuse white halo.
    void drawLogo (juce::Graphics& g, const juce::String& text, const juce::Font& font,
                   float x, float baselineY)
    {
        using namespace PeachyColours;

        juce::GlyphArrangement arrangement;
        arrangement.addLineOfText (font, text, x, baselineY);

        juce::Path path;
        arrangement.createPath (path);

        logoHalo.render (g, path);

        const auto bounds = path.getBounds();
        juce::ColourGradient gradient (logoTop, bounds.getX(), bounds.getY(),
                                       logoBottom, bounds.getX(), bounds.getBottom(), false);
        gradient.addColour (0.45, logoMid);
        g.setGradientFill (gradient);
        g.fillPath (path);
    }

    void drawTextAtBaseline (juce::Graphics& g, const juce::String& text, const juce::Font& font,
                             float x, float baselineY, juce::Colour colour) const
    {
        juce::GlyphArrangement arrangement;
        arrangement.addLineOfText (font, text, x, baselineY);
        g.setColour (colour);
        arrangement.draw (g);
    }

    float measureTextRight (const juce::Font& font, const juce::String& text, float x, float baselineY) const
    {
        juce::GlyphArrangement arrangement;
        arrangement.addLineOfText (font, text, x, baselineY);
        return arrangement.getBoundingBox (0, -1, true).getRight();
    }

    // ------------------------------------------------------------------- LED
    // Peach element, faint diffuse *white* halo (never a white core).
    static void drawGlowingLed (juce::Graphics& g, juce::Rectangle<float> ledBounds, bool on)
    {
        using namespace PeachyColours;

        if (on)
        {
            juce::Path ledPath;
            ledPath.addEllipse (ledBounds);

            // peach aura outside...
            melatonin::DropShadow halo { accentGlow, 9, { 0, 0 }, 1 };
            halo.render (g, ledPath);

            g.setColour (accent);
            g.fillEllipse (ledBounds);

            // ...and the mockup's 1px white *inner* highlight
            g.setColour (innerHighlight);
            g.drawEllipse (ledBounds.reduced (1.0f), 1.0f);
        }
        else
        {
            g.setColour (ledOff);
            g.fillEllipse (ledBounds);
            g.setColour (juce::Colours::black.withAlpha (0.5f));
            g.drawEllipse (ledBounds, 1.0f);
        }
    }

    // ---------------------------------------------------------------- rotary
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override
    {
        using namespace PeachyColours;

        auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();
        const float diameter = juce::jmin (bounds.getWidth(), bounds.getHeight());
        const auto centre = bounds.getCentre();
        const float R = diameter * 0.5f;

        const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        const int numTicks = (int) slider.getProperties().getWithDefault (PeachyKnobProps::numTicks, 21);
        const bool vuStyle = (bool) slider.getProperties().getWithDefault (PeachyKnobProps::vuStyle, false);

        // Fixed pixel radii: the knob keeps a constant hardware size, extra
        // component room goes to the tick ring / labels breathing space.
        const float knobR = 39.0f;    // outer smooth margin ring (joint with the faceplate)
        const float textureR = 33.0f; // knurled ring (horizontal striations)
        const float discR = 18.0f;    // very dark centre disc
        const float tickInnerR = knobR + 6.0f;
        const float tickOuterR = knobR + 13.0f;
        const float labelR = juce::jmin (R - 6.0f, tickOuterR + 17.0f);

        // ---- tick ring (lit ticks: peach, faint white halo) ----
        for (int i = 0; i < numTicks; ++i)
        {
            const float ta = rotaryStartAngle + (rotaryEndAngle - rotaryStartAngle) * (float) i / (float) (numTicks - 1);
            const bool lit = ta <= angle + 0.001f;

            const auto p1 = centre.getPointOnCircumference (tickInnerR, ta);
            const auto p2 = centre.getPointOnCircumference (tickOuterR, ta);

            if (lit)
            {
                const float glow = vuStyle ? (3.0f + 10.0f * sliderPos) : 5.0f;
                const float alpha = vuStyle ? (0.55f + 0.45f * sliderPos) : 1.0f;
                drawGlowingLine (g, p1, p2, 2.0f, accent.withAlpha (alpha), accentGlow, glow);
            }
            else
            {
                g.setColour (tickInactive);
                strokeRoundedLine (g, p1, p2, 2.0f);
            }
        }

        // ---- soft realistic drop shadow under the knob body ----
        auto knobBounds = juce::Rectangle<float> (knobR * 2.0f, knobR * 2.0f).withCentre (centre);
        {
            juce::Path knobPath;
            knobPath.addEllipse (knobBounds);
            knobShadow.render (g, knobPath);
        }

        // ---- outer margin ring: smooth, makes the joint with the faceplate ----
        {
            juce::ColourGradient grad (bezelTop, knobBounds.getTopLeft(), bezelBottom, knobBounds.getBottomRight(), false);
            g.setGradientFill (grad);
            g.fillEllipse (knobBounds);
        }

        // ---- knurled ring: horizontal machined striations ----
        auto textureBounds = juce::Rectangle<float> (textureR * 2.0f, textureR * 2.0f).withCentre (centre);
        {
            juce::ColourGradient grad (knurlBase,
                                       textureBounds.getX() + textureBounds.getWidth() * 0.35f,
                                       textureBounds.getY() + textureBounds.getHeight() * 0.30f,
                                       knurlEdge, textureBounds.getCentreX(), textureBounds.getBottom(), true);
            g.setGradientFill (grad);
            g.fillEllipse (textureBounds);

            // Fine radial knurl: many hairline spokes running in the same
            // direction as the surrounding ticks (the mockup's
            // repeating-conic-gradient, ~3.2° per ridge). Static — the texture
            // belongs to the knob face, only the pointer rotates.
            constexpr int numSpokes = 112;
            for (int i = 0; i < numSpokes; ++i)
            {
                const float spokeAngle = juce::MathConstants<float>::twoPi * (float) i / (float) numSpokes;
                const auto pIn = centre.getPointOnCircumference (discR + 0.5f, spokeAngle);
                const auto pOut = centre.getPointOnCircumference (textureR - 0.5f, spokeAngle);

                g.setColour ((i & 1) == 0 ? juce::Colours::black.withAlpha (0.11f)
                                          : juce::Colours::white.withAlpha (0.04f));
                g.drawLine ({ pIn, pOut }, 1.0f);
            }
        }

        // ---- very dark centre disc, with relief ----
        auto discBounds = juce::Rectangle<float> (discR * 2.0f, discR * 2.0f).withCentre (centre);
        {
            juce::ColourGradient grad (centreDiscTop,
                                       discBounds.getX() + discBounds.getWidth() * 0.35f,
                                       discBounds.getY() + discBounds.getHeight() * 0.25f,
                                       centreDiscEdge, discBounds.getCentreX(), discBounds.getBottom(), true);
            g.setGradientFill (grad);
            g.fillEllipse (discBounds);

            // inset relief: dark upper lip, faint light catch below
            juce::Path topArc, bottomArc;
            topArc.addArc (discBounds.getX(), discBounds.getY(), discBounds.getWidth(), discBounds.getHeight(),
                           juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 1.75f, true);
            bottomArc.addArc (discBounds.getX(), discBounds.getY(), discBounds.getWidth(), discBounds.getHeight(),
                              juce::MathConstants<float>::pi * 0.25f, juce::MathConstants<float>::pi * 0.75f, true);
            g.setColour (juce::Colours::black.withAlpha (0.55f));
            g.strokePath (topArc, juce::PathStrokeType (1.4f));
            g.setColour (juce::Colours::white.withAlpha (0.08f));
            g.strokePath (bottomArc, juce::PathStrokeType (1.2f));
        }

        // ---- neon pointer: from the edge of the centre disc to the knurl edge ----
        {
            const float pinWidth = 4.5f;
            const auto pinStart = centre.getPointOnCircumference (discR + 1.0f, angle);
            const auto pinEnd = centre.getPointOnCircumference (textureR - 1.0f, angle);
            drawGlowingLine (g, pinStart, pinEnd, pinWidth, accent, accentGlow, 9.0f);

            // the mockup's inset white highlight: a 1px hot core inside the bar
            const auto coreStart = centre.getPointOnCircumference (discR + 3.0f, angle);
            const auto coreEnd = centre.getPointOnCircumference (textureR - 3.0f, angle);
            g.setColour (innerHighlight);
            strokeRoundedLine (g, coreStart, coreEnd, 1.0f);
        }

        // ---- min / mid / max labels, clear of the tick ring ----
        g.setColour (juce::Colour (0xff84868d));
        g.setFont (getLcdFont (11.0f));

        const bool bipolar = slider.getMinimum() < 0.0;

        auto drawLabelAt = [&] (double value, float labelAngle)
        {
            const int rounded = (int) std::lround (value);
            const auto text = (bipolar && rounded > 0 ? juce::String ("+") : juce::String()) + juce::String (rounded);
            auto p = centre.getPointOnCircumference (labelR, labelAngle);
            juce::Rectangle<float> textBox (34.0f, 14.0f);
            g.drawText (text, textBox.withCentre (p), juce::Justification::centred, false);
        };

        const double mid = (slider.getMinimum() + slider.getMaximum()) * 0.5;
        drawLabelAt (slider.getMinimum(), rotaryStartAngle);
        drawLabelAt (mid, (rotaryStartAngle + rotaryEndAngle) * 0.5f);
        drawLabelAt (slider.getMaximum(), rotaryEndAngle);

        if (bipolar)
        {
            // quarter labels (-6 / +6 on a ±12 knob), like the mockup's five-step scale
            const double quarter = (slider.getMaximum() - slider.getMinimum()) * 0.25;
            const float angleSpan = rotaryEndAngle - rotaryStartAngle;
            drawLabelAt (mid - quarter, rotaryStartAngle + angleSpan * 0.25f);
            drawLabelAt (mid + quarter, rotaryStartAngle + angleSpan * 0.75f);
        }
    }

    // --------------------------------------------------------------- buttons
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                           bool /*shouldDrawButtonAsHighlighted*/, bool shouldDrawButtonAsDown) override
    {
        using namespace PeachyColours;
        auto bounds = button.getLocalBounds().toFloat();
        const bool on = button.getToggleState();
        const bool noLed = (bool) button.getProperties().getWithDefault (PeachyKnobProps::noInternalLed, false);

        drawPanelButtonBackground (g, bounds, on, shouldDrawButtonAsDown);

        const auto font = getOswaldFontWithKerning (11.0f, 0.12f);
        const auto colour = on ? buttonLabelOn : buttonLabelOff;

        if (noLed)
        {
            // LINK: no internal LED (its LED floats above the button), text centred
            if (on)
                drawBoldGlowText (g, button.getButtonText().toUpperCase(), bounds, font, colour, juce::Justification::centred);
            else
                drawBoldText (g, button.getButtonText().toUpperCase(), bounds, font, colour, juce::Justification::centred);
            return;
        }

        const float ledDiam = 8.0f;
        auto ledBounds = juce::Rectangle<float> (ledDiam, ledDiam).withCentre ({ bounds.getX() + 14.0f, bounds.getCentreY() });
        drawGlowingLed (g, ledBounds, on);

        drawBoldText (g, button.getButtonText().toUpperCase(), bounds.withTrimmedLeft (24.0f),
                      font, colour, juce::Justification::centredLeft);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                              bool /*shouldDrawButtonAsHighlighted*/, bool shouldDrawButtonAsDown) override
    {
        using namespace PeachyColours;
        auto bounds = button.getLocalBounds().toFloat();
        const bool on = button.getToggleState();

        drawPanelButtonBackground (g, bounds, on, shouldDrawButtonAsDown);

        const float ledDiam = 9.0f;
        auto ledBounds = juce::Rectangle<float> (ledDiam, ledDiam).withCentre ({ bounds.getX() + 18.0f, bounds.getCentreY() });
        drawGlowingLed (g, ledBounds, on);
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool) override
    {
        using namespace PeachyColours;
        const bool on = button.getToggleState();
        const auto font = getOswaldFontWithKerning (12.0f, 0.1f);
        const auto area = button.getLocalBounds().toFloat().withTrimmedLeft (32.0f);

        // Active mode names carry the faint white halo, like the mockup
        if (on)
            drawBoldGlowText (g, button.getButtonText(), area, font, buttonLabelOn, juce::Justification::centredLeft);
        else
            drawBoldText (g, button.getButtonText(), area, font, buttonLabelOff, juce::Justification::centredLeft);
    }

    // ---------------------------------------------------------------- labels
    // Knob names (PITCH, FORMANT, DRIVE, MIX) render bold via the path trick
    void drawLabel (juce::Graphics& g, juce::Label& label) override
    {
        if (label.isBeingEdited())
        {
            juce::LookAndFeel_V4::drawLabel (g, label);
            return;
        }

        drawBoldText (g, label.getText(), label.getLocalBounds().toFloat(), label.getFont(),
                      label.findColour (juce::Label::textColourId), label.getJustificationType());
    }

    // ------------------------------------------------------------ popup menu
    // The preset list inherits the LCD's dark/amber identity instead of the
    // stock grey JUCE menu.
    juce::Font getPopupMenuFont() override
    {
        return getLcdFont (15.0f);
    }

    void drawPopupMenuBackground (juce::Graphics& g, int width, int height) override
    {
        using namespace PeachyColours;
        juce::ColourGradient grad (lcdBg, 0.0f, 0.0f, lcdBgBottom, 0.0f, (float) height, false);
        g.setGradientFill (grad);
        g.fillAll();

        g.setColour (lcdBorder);
        g.drawRect (0, 0, width, height, 1);
        g.setColour (accent.withAlpha (0.12f));
        g.drawRect (1, 1, width - 2, height - 2, 1);
    }

    void drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                            bool isSeparator, bool isActive, bool isHighlighted,
                            bool isTicked, bool /*hasSubMenu*/, const juce::String& text,
                            const juce::String& /*shortcutKeyText*/, const juce::Drawable* /*icon*/,
                            const juce::Colour* textColourToUse) override
    {
        using namespace PeachyColours;

        if (isSeparator)
        {
            g.setColour (lcdTextDim.withAlpha (0.3f));
            g.fillRect (area.reduced (10, 0).withHeight (1).withY (area.getCentreY()));
            return;
        }

        auto r = area.reduced (2, 1).toFloat();

        if (isHighlighted && isActive)
        {
            g.setColour (accent.withAlpha (0.16f));
            g.fillRoundedRectangle (r, 3.0f);
        }

        auto colour = textColourToUse != nullptr ? *textColourToUse
                                                 : (isTicked ? lcdText : lcdTextDim.brighter (0.35f));
        if (isHighlighted)
            colour = lcdText;
        if (! isActive)
            colour = colour.withAlpha (0.4f);

        if (isTicked)
        {
            auto led = juce::Rectangle<float> (6.0f, 6.0f).withCentre ({ r.getX() + 12.0f, r.getCentreY() });
            g.setColour (accent);
            g.fillEllipse (led);
        }

        g.setColour (colour);
        g.setFont (getLcdFont (15.0f));
        g.drawText (text, area.reduced (26, 0), juce::Justification::centredLeft, true);
    }

    // Rounded-cap stroke, like the mockup's neon bars (drawLine has butt ends)
    static void strokeRoundedLine (juce::Graphics& g, juce::Point<float> p1, juce::Point<float> p2,
                                   float thickness)
    {
        juce::Path path;
        path.startNewSubPath (p1);
        path.lineTo (p2);
        g.strokePath (path, juce::PathStrokeType (thickness, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
    }

    // Cheap glow: a few progressively wider, fainter strokes behind the solid
    // line — element keeps its own colour, the halo has its own (white).
    static void drawGlowingLine (juce::Graphics& g, juce::Point<float> p1, juce::Point<float> p2,
                                float thickness, juce::Colour colour, juce::Colour glowColour,
                                float glowAmount)
    {
        if (glowAmount > 0.0f)
        {
            constexpr int steps = 3;
            for (int i = steps; i >= 1; --i)
            {
                const float t = (float) i / (float) steps;
                g.setColour (glowColour.withAlpha (glowColour.getFloatAlpha() * 0.45f * t));
                strokeRoundedLine (g, p1, p2, thickness + glowAmount * t * 0.6f);
            }
        }

        g.setColour (colour);
        strokeRoundedLine (g, p1, p2, thickness);
    }

private:
    static juce::Typeface::Ptr loadTypeface (const char* data, int size)
    {
        if (data != nullptr && size > 0)
            return juce::Typeface::createSystemTypefaceFor (data, (size_t) size);

        return nullptr;
    }

    static void drawPanelButtonBackground (juce::Graphics& g, juce::Rectangle<float> bounds, bool on, bool pressed)
    {
        using namespace PeachyColours;

        juce::ColourGradient grad (on ? buttonOnTop : buttonOffTop, bounds.getTopLeft(),
                                   on ? buttonOnBottom : buttonOffBottom, bounds.getBottomLeft(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (bounds, 7.0f);

        if (!on)
        {
            juce::DropShadow (juce::Colours::black.withAlpha (0.35f), 6, { 0, 3 }).drawForRectangle (
                g, bounds.getSmallestIntegerContainer());
        }

        g.setColour (on ? juce::Colour (0xff0e0f11) : juce::Colour (0xff17181b));
        g.drawRoundedRectangle (bounds.reduced (0.5f), 7.0f, 1.0f);

        if (pressed)
            g.fillAll (juce::Colours::black.withAlpha (0.08f));
    }

    juce::Typeface::Ptr logoTypeface, lcdTypeface, labelTypeface;

    // Cached shadows/halos (melatonin re-uses the blur while the path repeats)
    melatonin::DropShadow knobShadow { juce::Colours::black.withAlpha (0.6f), 14, { 0, 6 }, 0 };
    // mockup CSS: text-shadow 0 3px 8px rgba(255,140,90,.25) — a warm drop, not white
    melatonin::DropShadow logoHalo { PeachyColours::accent.withAlpha (0.30f), 10, { 0, 2 }, 1 };
    melatonin::DropShadow textHalo { PeachyColours::accentGlow, 6, { 0, 0 }, 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PeachyLookAndFeel)
};

// ------------------------------------------------------------- LcdDisplay impl

inline void LcdDisplay::resized()
{
    auto bounds = getLocalBounds().toFloat();

    // right-hand control strip: [<] [>] [dice]
    constexpr float cell = 20.0f;
    const float cy = bounds.getCentreY();
    diceZone = juce::Rectangle<float> (cell, cell).withCentre ({ bounds.getRight() - 18.0f, cy });
    nextZone = juce::Rectangle<float> (cell, cell).withCentre ({ bounds.getRight() - 40.0f, cy });
    prevZone = juce::Rectangle<float> (cell, cell).withCentre ({ bounds.getRight() - 62.0f, cy });

    textZone = bounds.withTrimmedRight (76.0f).withTrimmedLeft (12.0f).reduced (0.0f, 7.0f);
}

inline void LcdDisplay::mouseUp (const juce::MouseEvent& e)
{
    if (! e.mouseWasClicked())
        return;

    const auto pos = e.position;

    if (prevZone.contains (pos)) { if (onPrevious != nullptr) onPrevious(); return; }
    if (nextZone.contains (pos)) { if (onNext != nullptr) onNext(); return; }
    if (diceZone.contains (pos)) { if (onDice != nullptr) onDice(); return; }

    if (onClick != nullptr)
        onClick();
}

inline void LcdDisplay::paint (juce::Graphics& g)
{
    using namespace PeachyColours;
    auto bounds = getLocalBounds().toFloat();

    juce::Path panel;
    panel.addRoundedRectangle (bounds, 5.0f);

    juce::ColourGradient grad (lcdBg, bounds.getTopLeft(), lcdBgBottom, bounds.getBottomLeft(), false);
    g.setGradientFill (grad);
    g.fillPath (panel);

    innerShadow.render (g, panel);

    g.setColour (lcdBorder);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 5.0f, 1.0f);

    auto* laf = dynamic_cast<PeachyLookAndFeel*> (&getLookAndFeel());
    auto lcdFont = [&] (float size)
    {
        return laf != nullptr ? laf->getLcdFont (size)
                              : juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), size, juce::Font::plain));
    };

    auto content = textZone;
    auto topArea = content.removeFromTop (content.getHeight() * 0.58f);
    auto bottomArea = content;

    // top line: bright preset name with the faint white halo
    {
        const auto font = lcdFont (15.0f);
        juce::GlyphArrangement arrangement;
        arrangement.addFittedText (font, topLine, topArea.getX(), topArea.getY(),
                                   topArea.getWidth(), topArea.getHeight(), juce::Justification::centredLeft, 1);
        juce::Path path;
        arrangement.createPath (path);

        textHalo.render (g, path);
        g.setColour (lcdText);
        g.fillPath (path);
    }

    // bottom line: dim mode line
    {
        g.setFont (lcdFont (10.5f));
        g.setColour (lcdTextDim);
        g.drawText (bottomLine, bottomArea, juce::Justification::centredLeft, false);
    }

    // ---- controls: < > arrows + dice ----
    g.setFont (lcdFont (15.0f));
    g.setColour (lcdTextDim.brighter (0.25f));
    g.drawText ("<", prevZone, juce::Justification::centred, false);
    g.drawText (">", nextZone, juce::Justification::centred, false);

    // vector die: rounded square + 3 diagonal pips
    {
        auto die = diceZone.reduced (4.0f);
        g.setColour (lcdTextDim.brighter (0.25f));
        g.drawRoundedRectangle (die, 2.5f, 1.2f);

        auto pip = [&] (float px, float py)
        {
            g.fillEllipse (juce::Rectangle<float> (2.0f, 2.0f).withCentre ({ px, py }));
        };
        pip (die.getX() + 3.0f, die.getY() + 3.0f);
        pip (die.getCentreX(), die.getCentreY());
        pip (die.getRight() - 3.0f, die.getBottom() - 3.0f);
    }

    // subtle separator before the control strip
    g.setColour (lcdTextDim.withAlpha (0.25f));
    g.fillRect (juce::Rectangle<float> (bounds.getRight() - 76.0f, bounds.getY() + 8.0f, 1.0f, bounds.getHeight() - 16.0f));
}

inline void LcdValueLabel::paint (juce::Graphics& g)
{
    using namespace PeachyColours;
    auto bounds = getLocalBounds().toFloat();

    g.setColour (valueBg);
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (lcdBorder);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);

    // subtle inset top edge, like the mockup's inner box-shadow
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawLine (bounds.getX() + 3.0f, bounds.getY() + 1.5f, bounds.getRight() - 3.0f, bounds.getY() + 1.5f, 1.5f);

    if (! isBeingEdited())
    {
        // amber digits with the faint white halo
        juce::GlyphArrangement arrangement;
        arrangement.addFittedText (getFont(), getText(), bounds.getX(), bounds.getY(),
                                   bounds.getWidth(), bounds.getHeight(), juce::Justification::centred, 1);
        juce::Path path;
        arrangement.createPath (path);

        textHalo.render (g, path);
        g.setColour (findColour (juce::Label::textColourId));
        g.fillPath (path);
    }
}
