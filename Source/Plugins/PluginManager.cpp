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

PluginManager::PluginManager()
{
    appProperties.setStorageParameters(createPropertiesOptions());

    juce::addDefaultFormatsToManager(formatManager);

    loadKnownPlugins();
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

void PluginManager::saveKnownPlugins()
{
    if (auto* properties = appProperties.getUserSettings())
    {
        properties->setValue("knownPluginList", knownPlugins.createXml().get());
        properties->saveIfNeeded();
    }
}

