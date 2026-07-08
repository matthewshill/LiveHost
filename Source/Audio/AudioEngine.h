#pragma once

#include <JuceHeader.h>

class AudioEngine final : private juce::AudioIODeviceCallback
{
public:
    struct RackSlotInfo
    {
        juce::String name;
        bool bypassed = false;
        bool hasEditor = false;
    };

    AudioEngine();
    ~AudioEngine() override;

    juce::AudioDeviceManager& getDeviceManager();
    const juce::AudioDeviceManager& getDeviceManager() const;

    juce::String getCurrentDeviceSummary();
    double getCurrentSampleRate() const;
    int getCurrentBufferSizeSamples() const;
    int getCurrentInputChannels() const;
    int getCurrentOutputChannels() const;
    int getSelectedInputPairStartChannel() const;
    int getSelectedOutputPairStartChannel() const;
    bool isTestToneEnabled() const;
    float getTestToneGain() const;
    juce::String getRackSummary() const;
    std::vector<RackSlotInfo> getRackSlotInfos() const;
    int getNumRackSlots() const;
    float getInputPeakLevel() const;
    float getOutputPeakLevel() const;

    void addPluginToRack(std::unique_ptr<juce::AudioPluginInstance> plugin);
    void removeRackSlot(int slotIndex);
    void clearRack();
    void setRackSlotBypassed(int slotIndex, bool shouldBeBypassed);
    std::unique_ptr<juce::AudioProcessorEditor> createRackSlotEditor(int slotIndex);
    void setInputPairStartChannel(int channelIndex);
    void setOutputPairStartChannel(int channelIndex);
    void setTestToneEnabled(bool shouldBeEnabled);
    void setTestToneGain(float gain);

private:
    struct RackSlot
    {
        std::unique_ptr<juce::AudioPluginInstance> plugin;
        juce::String name;
        bool bypassed = false;
    };

    void preparePlugin(juce::AudioPluginInstance& plugin);
    void releaseRackResources();
    void updateRoutingAfterDeviceChange();
    void renderTestTone(int numSamples);

    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                          int numInputChannels,
                                          float* const* outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext& context) override;

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;
    static float calculatePeakLevel(const juce::AudioBuffer<float>& buffer, int numChannels, int numSamples);
    static float smoothMeterValue(float previousValue, float nextValue);

    juce::AudioDeviceManager deviceManager;
    mutable juce::CriticalSection rackLock;
    std::vector<RackSlot> rackSlots;
    juce::AudioBuffer<float> processBuffer;
    juce::MidiBuffer midiBuffer;
    double currentSampleRate = 44100.0;
    int currentBufferSize = 512;
    int currentInputChannels = 2;
    int currentOutputChannels = 2;
    int selectedInputPairStartChannel = 0;
    int selectedOutputPairStartChannel = 0;
    bool testToneEnabled = false;
    float testToneGain = 0.125f;
    double testTonePhase = 0.0;
    std::atomic<float> inputPeakLevel { 0.0f };
    std::atomic<float> outputPeakLevel { 0.0f };
};
