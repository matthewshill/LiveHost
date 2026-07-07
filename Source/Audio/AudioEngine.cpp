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

double AudioEngine::getCurrentSampleRate() const
{
    const juce::ScopedLock lock(pluginLock);
    return currentSampleRate;
}

int AudioEngine::getCurrentBufferSizeSamples() const
{
    const juce::ScopedLock lock(pluginLock);
    return currentBufferSize;
}

juce::String AudioEngine::getActivePluginName() const
{
    const juce::ScopedLock lock(pluginLock);
    return activePluginName;
}

bool AudioEngine::hasActivePlugin() const
{
    const juce::ScopedLock lock(pluginLock);
    return activePlugin != nullptr;
}

bool AudioEngine::isActivePluginBypassed() const
{
    const juce::ScopedLock lock(pluginLock);
    return activePluginBypassed;
}

float AudioEngine::getInputPeakLevel() const
{
    return inputPeakLevel.load();
}

float AudioEngine::getOutputPeakLevel() const
{
    return outputPeakLevel.load();
}

void AudioEngine::setActivePlugin(std::unique_ptr<juce::AudioPluginInstance> plugin)
{
    const juce::ScopedLock lock(pluginLock);

    if (activePlugin != nullptr)
        activePlugin->releaseResources();

    activePlugin = std::move(plugin);
    activePluginName = activePlugin != nullptr ? activePlugin->getName() : "No plugin loaded";
    activePluginBypassed = false;
    prepareActivePlugin();
}

void AudioEngine::clearActivePlugin()
{
    setActivePlugin(nullptr);
}

void AudioEngine::setActivePluginBypassed(bool shouldBeBypassed)
{
    const juce::ScopedLock lock(pluginLock);
    activePluginBypassed = shouldBeBypassed;
}

std::unique_ptr<juce::AudioProcessorEditor> AudioEngine::createActivePluginEditor()
{
    const juce::ScopedLock lock(pluginLock);

    if (activePlugin == nullptr || ! activePlugin->hasEditor())
        return nullptr;

    return std::unique_ptr<juce::AudioProcessorEditor>(activePlugin->createEditorAndMakeActive());
}

void AudioEngine::prepareActivePlugin()
{
    if (activePlugin == nullptr)
        return;

    activePlugin->setPlayConfigDetails(currentInputChannels,
                                       currentOutputChannels,
                                       currentSampleRate,
                                       currentBufferSize);
    activePlugin->prepareToPlay(currentSampleRate, currentBufferSize);
}

void AudioEngine::audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                                   int numInputChannels,
                                                   float* const* outputChannelData,
                                                   int numOutputChannels,
                                                   int numSamples,
                                                   const juce::AudioIODeviceCallbackContext&)
{
    const auto numChannels = juce::jmax(numInputChannels, numOutputChannels);

    if (processBuffer.getNumChannels() < numChannels || processBuffer.getNumSamples() < numSamples)
        processBuffer.setSize(numChannels, numSamples, false, false, true);

    for (auto channel = 0; channel < numChannels; ++channel)
    {
        auto* destination = processBuffer.getWritePointer(channel);

        if (channel < numInputChannels && inputChannelData[channel] != nullptr)
            juce::FloatVectorOperations::copy(destination, inputChannelData[channel], numSamples);
        else
            juce::FloatVectorOperations::clear(destination, numSamples);
    }

    inputPeakLevel.store(smoothMeterValue(inputPeakLevel.load(),
                                          calculatePeakLevel(processBuffer, numInputChannels, numSamples)));

    {
        const juce::ScopedLock lock(pluginLock);

        if (activePlugin != nullptr && ! activePluginBypassed)
        {
            midiBuffer.clear();
            activePlugin->processBlock(processBuffer, midiBuffer);
        }
    }

    for (auto channel = 0; channel < numOutputChannels; ++channel)
        if (auto* output = outputChannelData[channel])
            juce::FloatVectorOperations::copy(output, processBuffer.getReadPointer(channel), numSamples);

    outputPeakLevel.store(smoothMeterValue(outputPeakLevel.load(),
                                           calculatePeakLevel(processBuffer, numOutputChannels, numSamples)));
}

void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    const juce::ScopedLock lock(pluginLock);

    currentSampleRate = device != nullptr ? device->getCurrentSampleRate() : 44100.0;
    currentBufferSize = device != nullptr ? device->getCurrentBufferSizeSamples() : 512;
    currentInputChannels = device != nullptr ? device->getActiveInputChannels().countNumberOfSetBits() : 2;
    currentOutputChannels = device != nullptr ? device->getActiveOutputChannels().countNumberOfSetBits() : 2;

    processBuffer.setSize(juce::jmax(currentInputChannels, currentOutputChannels),
                          currentBufferSize,
                          false,
                          false,
                          true);
    prepareActivePlugin();
}

void AudioEngine::audioDeviceStopped()
{
    const juce::ScopedLock lock(pluginLock);

    if (activePlugin != nullptr)
        activePlugin->releaseResources();

    inputPeakLevel.store(0.0f);
    outputPeakLevel.store(0.0f);
}

float AudioEngine::calculatePeakLevel(const juce::AudioBuffer<float>& buffer, int numChannels, int numSamples)
{
    auto peak = 0.0f;
    const auto channelsToMeasure = juce::jmin(numChannels, buffer.getNumChannels());

    for (auto channel = 0; channel < channelsToMeasure; ++channel)
        peak = juce::jmax(peak, buffer.getMagnitude(channel, 0, numSamples));

    return juce::jlimit(0.0f, 1.0f, peak);
}

float AudioEngine::smoothMeterValue(float previousValue, float nextValue)
{
    if (nextValue >= previousValue)
        return nextValue;

    return previousValue * 0.82f + nextValue * 0.18f;
}
