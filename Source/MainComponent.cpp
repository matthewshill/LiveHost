#include "MainComponent.h"

MainComponent::MainComponent()
{
    setSize(1280, 760);
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(22, 24, 28));

    auto bounds = getLocalBounds().reduced(32);

    g.setColour(juce::Colour::fromRGB(236, 239, 244));
    g.setFont(juce::FontOptions(32.0f, juce::Font::bold));
    g.drawText("LiveHost", bounds.removeFromTop(44), juce::Justification::centredLeft);

    g.setColour(juce::Colour::fromRGB(150, 159, 174));
    g.setFont(juce::FontOptions(16.0f));
    g.drawText("JUCE app shell", bounds.removeFromTop(28), juce::Justification::centredLeft);

    auto rackArea = bounds.withTrimmedTop(32);
    g.setColour(juce::Colour::fromRGB(35, 39, 45));
    g.fillRoundedRectangle(rackArea.toFloat(), 8.0f);

    g.setColour(juce::Colour::fromRGB(68, 75, 86));
    g.drawRoundedRectangle(rackArea.toFloat().reduced(0.5f), 8.0f, 1.0f);
}

void MainComponent::resized() {}

