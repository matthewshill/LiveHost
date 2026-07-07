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

private:
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    void loadKnownPlugins();
    void saveKnownPlugins();

    juce::ApplicationProperties appProperties;
    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList knownPlugins;
};

