#include "LevelMeter.h"

void LevelMeter::setLabel(juce::String newLabel)
{
    label = std::move(newLabel);
    repaint();
}

void LevelMeter::setLevel(float newLevel)
{
    const auto clamped = juce::jlimit(0.0f, 1.0f, newLevel);

    if (std::abs(level - clamped) > 0.001f)
    {
        level = clamped;
        repaint();
    }
}

void LevelMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    const auto labelArea = bounds.removeFromTop(18);

    g.setColour(juce::Colour::fromRGB(150, 159, 174));
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    g.drawText(label, labelArea, juce::Justification::centredLeft);

    auto meterArea = bounds.reduced(0, 3);
    g.setColour(juce::Colour::fromRGB(17, 19, 23));
    g.fillRoundedRectangle(meterArea.toFloat(), 4.0f);

    const auto fillWidth = juce::roundToInt(static_cast<float>(meterArea.getWidth()) * level);
    auto fillArea = meterArea.withWidth(fillWidth);

    const auto meterColour = level > 0.9f ? juce::Colour::fromRGB(235, 89, 80)
                          : level > 0.7f ? juce::Colour::fromRGB(236, 183, 82)
                                         : juce::Colour::fromRGB(66, 190, 138);

    g.setColour(meterColour);
    g.fillRoundedRectangle(fillArea.toFloat(), 4.0f);

    g.setColour(juce::Colour::fromRGB(68, 75, 86));
    g.drawRoundedRectangle(meterArea.toFloat().reduced(0.5f), 4.0f, 1.0f);
}
