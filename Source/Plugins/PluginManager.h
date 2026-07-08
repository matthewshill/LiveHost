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
    juce::File getScanExclusionsFile() const;

    int getNumKnownPlugins() const;
    int getNumScanExclusions() const;
    std::optional<juce::PluginDescription> getPluginDescriptionAt(int index) const;
    void reloadScanExclusions();

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
    void ensureScanExclusionsFileExists() const;
    void appendMissingDefaultScanExclusions() const;
    juce::File getScannerExecutableFile() const;
    bool scanWithExternalProcess(juce::AudioPluginFormat& format,
                                 juce::OwnedArray<juce::PluginDescription>& result,
                                 const juce::String& fileOrIdentifier) const;
    bool addDescriptionsFromScanResult(const juce::File& resultFile,
                                       juce::OwnedArray<juce::PluginDescription>& result) const;
    bool shouldSkipScanCandidate(const juce::String& fileOrIdentifier) const;
    bool shouldSkipPluginDescription(const juce::PluginDescription& description) const;
    static bool matchesAnyPattern(const juce::String& text, const juce::StringArray& patterns);

    juce::ApplicationProperties appProperties;
    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList knownPlugins;
    juce::StringArray scanExclusionPatterns;
};
