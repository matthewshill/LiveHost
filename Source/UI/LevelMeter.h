#pragma once

#include <JuceHeader.h>

class LevelMeter final : public juce::Component
{
public:
    void setLabel(juce::String newLabel);
    void setLevel(float newLevel);

    void paint(juce::Graphics& g) override;

private:
    juce::String label;
    float level = 0.0f;
};

