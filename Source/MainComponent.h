#pragma once

#include <JuceHeader.h>

#include "Audio/AudioEngine.h"
#include "Plugins/PluginManager.h"

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
    void refreshPluginStatus();

    AudioEngine audioEngine;
    PluginManager pluginManager;
    juce::Label titleLabel;
    juce::Label statusLabel;
    juce::Label pluginStatusLabel;
    std::unique_ptr<juce::AudioDeviceSelectorComponent> deviceSelector;
    std::unique_ptr<juce::PluginListComponent> pluginListComponent;
};
