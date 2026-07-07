#pragma once

#include <JuceHeader.h>

#include "Audio/AudioEngine.h"

class MainComponent final : public juce::Component,
                            private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;
    void refreshDeviceStatus();

    AudioEngine audioEngine;
    juce::Label titleLabel;
    juce::Label statusLabel;
    std::unique_ptr<juce::AudioDeviceSelectorComponent> deviceSelector;
};
