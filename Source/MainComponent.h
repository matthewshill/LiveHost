#pragma once

#include <JuceHeader.h>

#include "Audio/AudioEngine.h"
#include "Plugins/PluginManager.h"
#include "UI/LevelMeter.h"

class MainComponent final : public juce::Component,
                            private juce::ListBoxModel,
                            private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    class PluginEditorWindow;

    int getNumRows() override;
    void paintListBoxItem(int rowNumber,
                          juce::Graphics& g,
                          int width,
                          int height,
                          bool rowIsSelected) override;
    void selectedRowsChanged(int lastRowSelected) override;

    void timerCallback() override;
    void refreshDeviceStatus();
    void refreshPluginStatus();
    void addSelectedPluginToRack();
    void handlePluginCreated(std::unique_ptr<juce::AudioPluginInstance> plugin, const juce::String& error);
    void removeSelectedRackSlot();
    void clearRack();
    void openSelectedRackSlotEditor();
    void closePluginEditor();
    int getSelectedRackSlotIndex() const;
    void refreshMeters();
    void refreshScanExclusionStatus();
    void openScanExclusionsFile();

    AudioEngine audioEngine;
    PluginManager pluginManager;
    juce::Label titleLabel;
    juce::Label statusLabel;
    juce::Label pluginStatusLabel;
    juce::Label rackStatusLabel;
    juce::TextButton loadPluginButton;
    juce::TextButton removeSlotButton;
    juce::TextButton clearPluginButton;
    juce::ToggleButton bypassPluginButton;
    juce::TextButton openEditorButton;
    juce::Label rackTitleLabel;
    juce::ListBox rackListBox;
    juce::Label scanExclusionsLabel;
    juce::TextButton openScanExclusionsButton;
    juce::TextButton reloadScanExclusionsButton;
    LevelMeter inputMeter;
    LevelMeter outputMeter;
    std::unique_ptr<juce::AudioDeviceSelectorComponent> deviceSelector;
    std::unique_ptr<juce::PluginListComponent> pluginListComponent;
    std::unique_ptr<PluginEditorWindow> pluginEditorWindow;
};
