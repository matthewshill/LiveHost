#include "AudioEngine.h"

AudioEngine::AudioEngine()
{
    constexpr int defaultInputs = 2;
    constexpr int defaultOutputs = 2;

    deviceManager.initialise(defaultInputs, defaultOutputs, nullptr, true);
    deviceManager.addAudioCallback(this);
}

AudioEngine::~AudioEngine()
{
    deviceManager.removeAudioCallback(this);
    deviceManager.closeAudioDevice();
}

juce::AudioDeviceManager& AudioEngine::getDeviceManager()
{
    return deviceManager;
}

const juce::AudioDeviceManager& AudioEngine::getDeviceManager() const
{
    return deviceManager;
}

juce::String AudioEngine::getCurrentDeviceSummary()
{
    if (auto* device = deviceManager.getCurrentAudioDevice())
    {
        return device->getName()
            + " | " + juce::String(device->getCurrentSampleRate(), 0) + " Hz"
            + " | " + juce::String(device->getCurrentBufferSizeSamples()) + " samples";
    }

    return "No audio device open";
}

void AudioEngine::audioDeviceIOCallbackWithContext(const float* const*,
                                                   int,
                                                   float* const* outputChannelData,
                                                   int numOutputChannels,
                                                   int numSamples,
                                                   const juce::AudioIODeviceCallbackContext&)
{
    for (auto channel = 0; channel < numOutputChannels; ++channel)
        if (auto* output = outputChannelData[channel])
            juce::FloatVectorOperations::clear(output, numSamples);
}

void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice*) {}

void AudioEngine::audioDeviceStopped() {}
