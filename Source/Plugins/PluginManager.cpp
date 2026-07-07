#include "PluginManager.h"

namespace
{
juce::PropertiesFile::Options createPropertiesOptions()
{
    juce::PropertiesFile::Options options;
    options.applicationName = "LiveHost";
    options.commonToAllUsers = false;
    options.doNotSave = false;
    options.filenameSuffix = ".settings";
    options.ignoreCaseOfKeyNames = false;
    options.osxLibrarySubFolder = "Application Support";
    options.storageFormat = juce::PropertiesFile::StorageFormat::storeAsXML;
    return options;
}
} // namespace

class PluginManager::EffectOnlyScanner final : public juce::KnownPluginList::CustomScanner
{
public:
    bool findPluginTypesFor(juce::AudioPluginFormat& format,
                            juce::OwnedArray<juce::PluginDescription>& result,
                            const juce::String& fileOrIdentifier) override
    {
        juce::OwnedArray<juce::PluginDescription> scannedPlugins;
        format.findAllTypesForFile(scannedPlugins, fileOrIdentifier);

        for (auto* plugin : scannedPlugins)
        {
            if (plugin != nullptr && ! plugin->isInstrument)
                result.add(new juce::PluginDescription(*plugin));
        }

        return true;
    }
};

PluginManager::PluginManager()
{
    appProperties.setStorageParameters(createPropertiesOptions());

    juce::addDefaultFormatsToManager(formatManager);

    loadKnownPlugins();
    removeInstrumentPlugins();
    knownPlugins.setCustomScanner(std::make_unique<EffectOnlyScanner>());
    knownPlugins.addChangeListener(this);
}

PluginManager::~PluginManager()
{
    knownPlugins.removeChangeListener(this);
    saveKnownPlugins();
}

juce::AudioPluginFormatManager& PluginManager::getFormatManager()
{
    return formatManager;
}

juce::KnownPluginList& PluginManager::getKnownPluginList()
{
    return knownPlugins;
}

juce::PropertiesFile* PluginManager::getPropertiesFile()
{
    return appProperties.getUserSettings();
}

juce::File PluginManager::getDeadMansPedalFile() const
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("LiveHost")
        .getChildFile("PluginScanInProgress.txt");
}

int PluginManager::getNumKnownPlugins() const
{
    return knownPlugins.getNumTypes();
}

std::optional<juce::PluginDescription> PluginManager::getPluginDescriptionAt(int index) const
{
    const auto plugins = knownPlugins.getTypes();

    if (juce::isPositiveAndBelow(index, plugins.size()))
        return plugins[index];

    return std::nullopt;
}

void PluginManager::createPluginInstanceAsync(const juce::PluginDescription& description,
                                              double initialSampleRate,
                                              int initialBufferSize,
                                              juce::AudioPluginFormat::PluginCreationCallback callback)
{
    formatManager.createPluginInstanceAsync(description,
                                            initialSampleRate,
                                            initialBufferSize,
                                            std::move(callback));
}

void PluginManager::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &knownPlugins)
        saveKnownPlugins();
}

void PluginManager::loadKnownPlugins()
{
    if (auto* properties = appProperties.getUserSettings())
        if (auto savedPluginList = properties->getXmlValue("knownPluginList"))
            knownPlugins.recreateFromXml(*savedPluginList);
}

void PluginManager::removeInstrumentPlugins()
{
    for (const auto& plugin : knownPlugins.getTypes())
        if (plugin.isInstrument)
            knownPlugins.removeType(plugin);
}

void PluginManager::saveKnownPlugins()
{
    if (auto* properties = appProperties.getUserSettings())
    {
        properties->setValue("knownPluginList", knownPlugins.createXml().get());
        properties->saveIfNeeded();
    }
}
