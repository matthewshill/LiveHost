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
    class PluginEditorWindow;

    void timerCallback() override;
    void refreshDeviceStatus();
    void refreshPluginStatus();
    void loadSelectedPlugin();
    void handlePluginCreated(std::unique_ptr<juce::AudioPluginInstance> plugin, const juce::String& error);
    void clearLoadedPlugin();
    void openActivePluginEditor();
    void closePluginEditor();

    AudioEngine audioEngine;
    PluginManager pluginManager;
    juce::Label titleLabel;
    juce::Label statusLabel;
    juce::Label pluginStatusLabel;
    juce::Label activePluginLabel;
    juce::TextButton loadPluginButton;
    juce::TextButton clearPluginButton;
    juce::ToggleButton bypassPluginButton;
    juce::TextButton openEditorButton;
    std::unique_ptr<juce::AudioDeviceSelectorComponent> deviceSelector;
    std::unique_ptr<juce::PluginListComponent> pluginListComponent;
    std::unique_ptr<PluginEditorWindow> pluginEditorWindow;
};
