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
    const juce::ScopedLock lock(rackLock);
    return currentSampleRate;
}

int AudioEngine::getCurrentBufferSizeSamples() const
{
    const juce::ScopedLock lock(rackLock);
    return currentBufferSize;
}

juce::String AudioEngine::getRackSummary() const
{
    const juce::ScopedLock lock(rackLock);

    if (rackSlots.empty())
        return "Rack empty";

    return juce::String(rackSlots.size()) + (rackSlots.size() == 1 ? " plugin loaded" : " plugins loaded");
}

std::vector<AudioEngine::RackSlotInfo> AudioEngine::getRackSlotInfos() const
{
    const juce::ScopedLock lock(rackLock);

    std::vector<RackSlotInfo> slots;
    slots.reserve(rackSlots.size());

    for (const auto& slot : rackSlots)
    {
        RackSlotInfo info;
        info.name = slot.name;
        info.bypassed = slot.bypassed;
        info.hasEditor = slot.plugin != nullptr && slot.plugin->hasEditor();
        slots.push_back(info);
    }

    return slots;
}

int AudioEngine::getNumRackSlots() const
{
    const juce::ScopedLock lock(rackLock);
    return static_cast<int>(rackSlots.size());
}

float AudioEngine::getInputPeakLevel() const
{
    return inputPeakLevel.load();
}

float AudioEngine::getOutputPeakLevel() const
{
    return outputPeakLevel.load();
}

void AudioEngine::addPluginToRack(std::unique_ptr<juce::AudioPluginInstance> plugin)
{
    if (plugin == nullptr)
        return;

    const juce::ScopedLock lock(rackLock);

    RackSlot slot;
    slot.name = plugin->getName();
    slot.plugin = std::move(plugin);
    preparePlugin(*slot.plugin);
    rackSlots.push_back(std::move(slot));
}

void AudioEngine::removeRackSlot(int slotIndex)
{
    const juce::ScopedLock lock(rackLock);

    if (! juce::isPositiveAndBelow(slotIndex, static_cast<int>(rackSlots.size())))
        return;

    if (rackSlots[static_cast<size_t>(slotIndex)].plugin != nullptr)
        rackSlots[static_cast<size_t>(slotIndex)].plugin->releaseResources();

    rackSlots.erase(rackSlots.begin() + slotIndex);
}

void AudioEngine::clearRack()
{
    const juce::ScopedLock lock(rackLock);
    releaseRackResources();
    rackSlots.clear();
}

void AudioEngine::setRackSlotBypassed(int slotIndex, bool shouldBeBypassed)
{
    const juce::ScopedLock lock(rackLock);

    if (! juce::isPositiveAndBelow(slotIndex, static_cast<int>(rackSlots.size())))
        return;

    rackSlots[static_cast<size_t>(slotIndex)].bypassed = shouldBeBypassed;
}

std::unique_ptr<juce::AudioProcessorEditor> AudioEngine::createRackSlotEditor(int slotIndex)
{
    const juce::ScopedLock lock(rackLock);

    if (! juce::isPositiveAndBelow(slotIndex, static_cast<int>(rackSlots.size())))
        return nullptr;

    auto& slot = rackSlots[static_cast<size_t>(slotIndex)];

    if (slot.plugin == nullptr || ! slot.plugin->hasEditor())
        return nullptr;

    return std::unique_ptr<juce::AudioProcessorEditor>(slot.plugin->createEditorAndMakeActive());
}

void AudioEngine::preparePlugin(juce::AudioPluginInstance& plugin)
{
    plugin.setPlayConfigDetails(currentInputChannels,
                                currentOutputChannels,
                                currentSampleRate,
                                currentBufferSize);
    plugin.prepareToPlay(currentSampleRate, currentBufferSize);
}

void AudioEngine::releaseRackResources()
{
    for (auto& slot : rackSlots)
        if (slot.plugin != nullptr)
            slot.plugin->releaseResources();
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
        const juce::ScopedLock lock(rackLock);

        for (auto& slot : rackSlots)
        {
            if (slot.plugin != nullptr && ! slot.bypassed)
            {
                midiBuffer.clear();
                slot.plugin->processBlock(processBuffer, midiBuffer);
            }
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
    const juce::ScopedLock lock(rackLock);

    currentSampleRate = device != nullptr ? device->getCurrentSampleRate() : 44100.0;
    currentBufferSize = device != nullptr ? device->getCurrentBufferSizeSamples() : 512;
    currentInputChannels = device != nullptr ? device->getActiveInputChannels().countNumberOfSetBits() : 2;
    currentOutputChannels = device != nullptr ? device->getActiveOutputChannels().countNumberOfSetBits() : 2;

    processBuffer.setSize(juce::jmax(currentInputChannels, currentOutputChannels),
                          currentBufferSize,
                          false,
                          false,
                          true);
    for (auto& slot : rackSlots)
        if (slot.plugin != nullptr)
            preparePlugin(*slot.plugin);
}

void AudioEngine::audioDeviceStopped()
{
    const juce::ScopedLock lock(rackLock);
    releaseRackResources();

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
