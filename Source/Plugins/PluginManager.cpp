#include "PluginManager.h"

#include "../Core/AppLogger.h"

namespace
{
constexpr auto scannerTimeoutMs = 15000;

const juce::StringArray& getDefaultScanExclusions()
{
    static const juce::StringArray defaults {
        "WaveShell",
        "iLok",
        "PACE",
        "Serato Hex FX",
        "Kontakt",
        "Supercharger",
        "Massive X",
        "iZVocalSynth2",
        "VocalSynth2"
    };

    return defaults;
}

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
    explicit EffectOnlyScanner(PluginManager& pluginManager)
        : owner(pluginManager)
    {
    }

    bool findPluginTypesFor(juce::AudioPluginFormat& format,
                            juce::OwnedArray<juce::PluginDescription>& result,
                            const juce::String& fileOrIdentifier) override
    {
        if (owner.shouldSkipScanCandidate(fileOrIdentifier))
        {
            AppLogger::logScanEvent("skip candidate", format.getName() + " | " + fileOrIdentifier);
            AppLogger::writeLastScanStatus("skipped", format.getName(), fileOrIdentifier);
            return true;
        }

        AppLogger::logScanEvent("begin candidate", format.getName() + " | " + fileOrIdentifier);
        AppLogger::writeLastScanStatus("scanning", format.getName(), fileOrIdentifier);

        juce::OwnedArray<juce::PluginDescription> scannedPlugins;

        if (! owner.scanWithExternalProcess(format, scannedPlugins, fileOrIdentifier))
        {
            AppLogger::writeLastScanStatus("failed", format.getName(), fileOrIdentifier);
            return false;
        }

        for (auto* plugin : scannedPlugins)
        {
            if (plugin != nullptr && ! plugin->isInstrument && ! owner.shouldSkipPluginDescription(*plugin))
            {
                result.add(new juce::PluginDescription(*plugin));
                AppLogger::logScanEvent("add effect", plugin->pluginFormatName + " | " + plugin->name);
            }
            else if (plugin != nullptr)
            {
                AppLogger::logScanEvent("skip plugin", plugin->pluginFormatName + " | " + plugin->name);
            }
        }

        AppLogger::writeLastScanStatus("completed", format.getName(), fileOrIdentifier);
        return true;
    }

private:
    PluginManager& owner;
};

PluginManager::PluginManager()
{
    AppLogger::initialise();
    AppLogger::log("Initialising plugin manager");

    appProperties.setStorageParameters(createPropertiesOptions());

    juce::addDefaultFormatsToManager(formatManager);

    ensureScanExclusionsFileExists();
    appendMissingDefaultScanExclusions();
    reloadScanExclusions();
    loadKnownPlugins();
    removeInstrumentPlugins();
    knownPlugins.setCustomScanner(std::make_unique<EffectOnlyScanner>(*this));
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

juce::File PluginManager::getScanExclusionsFile() const
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("LiveHost")
        .getChildFile("PluginScanExclusions.txt");
}

int PluginManager::getNumKnownPlugins() const
{
    return knownPlugins.getNumTypes();
}

int PluginManager::getNumScanExclusions() const
{
    return scanExclusionPatterns.size();
}

std::optional<juce::PluginDescription> PluginManager::getPluginDescriptionAt(int index) const
{
    const auto plugins = knownPlugins.getTypes();

    if (juce::isPositiveAndBelow(index, plugins.size()))
        return plugins[index];

    return std::nullopt;
}

void PluginManager::reloadScanExclusions()
{
    scanExclusionPatterns.clear();

    for (auto line : juce::StringArray::fromLines(getScanExclusionsFile().loadFileAsString()))
    {
        line = line.trim();

        if (line.isNotEmpty() && ! line.startsWithChar('#'))
            scanExclusionPatterns.addIfNotAlreadyThere(line);
    }
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

void PluginManager::ensureScanExclusionsFileExists() const
{
    const auto exclusionsFile = getScanExclusionsFile();
    exclusionsFile.getParentDirectory().createDirectory();

    if (exclusionsFile.existsAsFile())
        return;

    exclusionsFile.replaceWithText(
        "# LiveHost plugin scan exclusions\n"
        "# Add one case-insensitive pattern per line.\n"
        "# Matching plugin paths, identifiers, names, or manufacturers are skipped during scans.\n"
        "# Examples:\n"
        "# iLok\n"
        "# PACE\n",
        false,
        false);
}

void PluginManager::appendMissingDefaultScanExclusions() const
{
    const auto exclusionsFile = getScanExclusionsFile();
    auto patterns = juce::StringArray::fromLines(exclusionsFile.loadFileAsString());
    auto changed = false;

    for (const auto& pattern : getDefaultScanExclusions())
    {
        if (! patterns.contains(pattern, true))
        {
            patterns.add(pattern);
            changed = true;
        }
    }

    if (changed)
        exclusionsFile.replaceWithText(patterns.joinIntoString("\n") + "\n", false, false);
}

juce::File PluginManager::getScannerExecutableFile() const
{
    return juce::File::getSpecialLocation(juce::File::currentExecutableFile)
        .getParentDirectory()
        .getChildFile("LiveHostPluginScanner");
}

bool PluginManager::scanWithExternalProcess(juce::AudioPluginFormat& format,
                                            juce::OwnedArray<juce::PluginDescription>& result,
                                            const juce::String& fileOrIdentifier) const
{
    const auto scannerExecutable = getScannerExecutableFile();

    if (! scannerExecutable.existsAsFile())
    {
        AppLogger::logScanEvent("scanner missing", scannerExecutable.getFullPathName());
        return false;
    }

    const auto resultFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getChildFile("LiveHost")
        .getChildFile("PluginScan")
        .getNonexistentChildFile("scan-result", ".xml", false);

    juce::StringArray args;
    args.add(scannerExecutable.getFullPathName());
    args.add("--format");
    args.add(format.getName());
    args.add("--file");
    args.add(fileOrIdentifier);
    args.add("--out");
    args.add(resultFile.getFullPathName());

    juce::ChildProcess scannerProcess;

    if (! scannerProcess.start(args, juce::ChildProcess::wantStdOut))
    {
        AppLogger::logScanEvent("scanner start failed", format.getName() + " | " + fileOrIdentifier);
        return false;
    }

    if (! scannerProcess.waitForProcessToFinish(scannerTimeoutMs))
    {
        scannerProcess.kill();
        AppLogger::logScanEvent("scanner timeout", format.getName() + " | " + fileOrIdentifier);
        return false;
    }

    const auto exitCode = scannerProcess.getExitCode();

    if (exitCode != 0)
    {
        AppLogger::logScanEvent("scanner failed",
                                format.getName() + " | " + fileOrIdentifier + " | exit " + juce::String(exitCode));
        resultFile.deleteFile();
        return false;
    }

    const auto parsed = addDescriptionsFromScanResult(resultFile, result);
    resultFile.deleteFile();
    return parsed;
}

bool PluginManager::addDescriptionsFromScanResult(const juce::File& resultFile,
                                                  juce::OwnedArray<juce::PluginDescription>& result) const
{
    const auto xml = juce::XmlDocument::parse(resultFile);

    if (xml == nullptr || ! xml->hasTagName("LiveHostPluginScanResult"))
    {
        AppLogger::logScanEvent("scanner result invalid", resultFile.getFullPathName());
        return false;
    }

    for (const auto* child : xml->getChildIterator())
    {
        if (child == nullptr || ! child->hasTagName("PLUGIN"))
            continue;

        auto description = std::make_unique<juce::PluginDescription>();

        if (description->loadFromXml(*child))
            result.add(description.release());
    }

    return true;
}

bool PluginManager::shouldSkipScanCandidate(const juce::String& fileOrIdentifier) const
{
    return matchesAnyPattern(fileOrIdentifier, scanExclusionPatterns);
}

bool PluginManager::shouldSkipPluginDescription(const juce::PluginDescription& description) const
{
    return matchesAnyPattern(description.name, scanExclusionPatterns)
        || matchesAnyPattern(description.descriptiveName, scanExclusionPatterns)
        || matchesAnyPattern(description.manufacturerName, scanExclusionPatterns)
        || matchesAnyPattern(description.fileOrIdentifier, scanExclusionPatterns);
}

bool PluginManager::matchesAnyPattern(const juce::String& text, const juce::StringArray& patterns)
{
    for (const auto& pattern : patterns)
        if (pattern.isNotEmpty() && text.containsIgnoreCase(pattern))
            return true;

    return false;
}

void PluginManager::saveKnownPlugins()
{
    if (auto* properties = appProperties.getUserSettings())
    {
        properties->setValue("knownPluginList", knownPlugins.createXml().get());
        properties->saveIfNeeded();
    }
}
