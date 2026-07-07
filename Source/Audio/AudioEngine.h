#pragma once

#include <JuceHeader.h>

class AudioEngine final : private juce::AudioIODeviceCallback
{
public:
    AudioEngine();
    ~AudioEngine() override;

    juce::AudioDeviceManager& getDeviceManager();
    const juce::AudioDeviceManager& getDeviceManager() const;

    juce::String getCurrentDeviceSummary();
    double getCurrentSampleRate() const;
    int getCurrentBufferSizeSamples() const;
    juce::String getActivePluginName() const;
    bool hasActivePlugin() const;
    bool isActivePluginBypassed() const;

    void setActivePlugin(std::unique_ptr<juce::AudioPluginInstance> plugin);
    void clearActivePlugin();
    void setActivePluginBypassed(bool shouldBeBypassed);
    std::unique_ptr<juce::AudioProcessorEditor> createActivePluginEditor();

private:
    void prepareActivePlugin();

    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                          int numInputChannels,
                                          float* const* outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext& context) override;

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

    juce::AudioDeviceManager deviceManager;
    mutable juce::CriticalSection pluginLock;
    std::unique_ptr<juce::AudioPluginInstance> activePlugin;
    juce::String activePluginName = "No plugin loaded";
    bool activePluginBypassed = false;
    juce::AudioBuffer<float> processBuffer;
    juce::MidiBuffer midiBuffer;
    double currentSampleRate = 44100.0;
    int currentBufferSize = 512;
    int currentInputChannels = 2;
    int currentOutputChannels = 2;
};
