#include <JuceHeader.h>

#include <iostream>

namespace
{
struct ScanRequest
{
    juce::String formatName;
    juce::String fileOrIdentifier;
    juce::File outputFile;
};

void printUsage()
{
    std::cerr << "Usage: LiveHostPluginScanner --format <format> --file <path-or-id> --out <xml-file>\n";
}

std::optional<ScanRequest> parseArgs(int argc, char* argv[])
{
    ScanRequest request;

    for (auto i = 1; i < argc; ++i)
    {
        const juce::String arg(argv[i]);

        if (arg == "--format" && i + 1 < argc)
            request.formatName = argv[++i];
        else if (arg == "--file" && i + 1 < argc)
            request.fileOrIdentifier = argv[++i];
        else if (arg == "--out" && i + 1 < argc)
            request.outputFile = juce::File(argv[++i]);
        else
            return std::nullopt;
    }

    if (request.formatName.isEmpty() || request.fileOrIdentifier.isEmpty() || request.outputFile == juce::File())
        return std::nullopt;

    return request;
}

juce::AudioPluginFormat* findFormat(juce::AudioPluginFormatManager& formatManager,
                                    const juce::String& formatName)
{
    for (auto i = 0; i < formatManager.getNumFormats(); ++i)
        if (auto* format = formatManager.getFormat(i))
            if (format->getName().equalsIgnoreCase(formatName))
                return format;

    return nullptr;
}

int scanPlugin(const ScanRequest& request)
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    juce::AudioPluginFormatManager formatManager;
    juce::addDefaultFormatsToManager(formatManager);

    auto* format = findFormat(formatManager, request.formatName);

    if (format == nullptr)
    {
        std::cerr << "Unsupported plugin format: " << request.formatName << "\n";
        return 2;
    }

    juce::OwnedArray<juce::PluginDescription> scannedPlugins;
    format->findAllTypesForFile(scannedPlugins, request.fileOrIdentifier);

    auto root = std::make_unique<juce::XmlElement>("LiveHostPluginScanResult");
    root->setAttribute("format", request.formatName);
    root->setAttribute("fileOrIdentifier", request.fileOrIdentifier);

    for (auto* plugin : scannedPlugins)
    {
        if (plugin == nullptr || plugin->isInstrument)
            continue;

        root->addChildElement(plugin->createXml().release());
    }

    request.outputFile.getParentDirectory().createDirectory();

    if (! root->writeTo(request.outputFile))
    {
        std::cerr << "Failed to write scan result: " << request.outputFile.getFullPathName() << "\n";
        return 3;
    }

    return 0;
}
} // namespace

int main(int argc, char* argv[])
{
    const auto request = parseArgs(argc, argv);

    if (! request.has_value())
    {
        printUsage();
        return 1;
    }

    return scanPlugin(*request);
}
