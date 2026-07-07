#pragma once

#include <JuceHeader.h>

class PluginManager final : private juce::ChangeListener
{
public:
    PluginManager();
    ~PluginManager() override;

    juce::AudioPluginFormatManager& getFormatManager();
    juce::KnownPluginList& getKnownPluginList();
    juce::PropertiesFile* getPropertiesFile();
    juce::File getDeadMansPedalFile() const;

    int getNumKnownPlugins() const;
    std::optional<juce::PluginDescription> getPluginDescriptionAt(int index) const;

    void createPluginInstanceAsync(const juce::PluginDescription& description,
                                   double initialSampleRate,
                                   int initialBufferSize,
                                   juce::AudioPluginFormat::PluginCreationCallback callback);

private:
    class EffectOnlyScanner;

    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    void loadKnownPlugins();
    void saveKnownPlugins();
    void removeInstrumentPlugins();

    juce::ApplicationProperties appProperties;
    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList knownPlugins;
};
