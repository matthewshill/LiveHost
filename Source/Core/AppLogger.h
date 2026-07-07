#pragma once

#include <JuceHeader.h>

class AppLogger final
{
public:
    static void initialise();
    static void log(juce::String message);
    static void logScanEvent(const juce::String& event, const juce::String& details);
    static void writeLastScanStatus(const juce::String& status,
                                    const juce::String& formatName,
                                    const juce::String& fileOrIdentifier);

    static juce::File getLogFile();
    static juce::File getLastScanStatusFile();

private:
    static juce::File getLogDirectory();
    static juce::String timestamp();
    static juce::CriticalSection& getLock();
};

