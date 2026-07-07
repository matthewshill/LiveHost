#include "AppLogger.h"

void AppLogger::initialise()
{
    getLogDirectory().createDirectory();
    log("LiveHost starting");
}

void AppLogger::log(juce::String message)
{
    const juce::ScopedLock lock(getLock());

    getLogDirectory().createDirectory();
    getLogFile().appendText(timestamp() + " " + message + "\n", false, false, nullptr);
}

void AppLogger::logScanEvent(const juce::String& event, const juce::String& details)
{
    log("[scan] " + event + " | " + details);
}

void AppLogger::writeLastScanStatus(const juce::String& status,
                                    const juce::String& formatName,
                                    const juce::String& fileOrIdentifier)
{
    const juce::ScopedLock lock(getLock());

    getLogDirectory().createDirectory();

    juce::DynamicObject::Ptr object = new juce::DynamicObject();
    object->setProperty("timestamp", timestamp());
    object->setProperty("status", status);
    object->setProperty("format", formatName);
    object->setProperty("fileOrIdentifier", fileOrIdentifier);

    getLastScanStatusFile().replaceWithText(juce::JSON::toString(juce::var(object.get())),
                                            false,
                                            false);
}

juce::File AppLogger::getLogFile()
{
    return getLogDirectory().getChildFile("LiveHost.log");
}

juce::File AppLogger::getLastScanStatusFile()
{
    return getLogDirectory().getChildFile("LastScanStatus.json");
}

juce::File AppLogger::getLogDirectory()
{
    return juce::File::getSpecialLocation(juce::File::userHomeDirectory)
        .getChildFile("Library")
        .getChildFile("Logs")
        .getChildFile("LiveHost");
}

juce::String AppLogger::timestamp()
{
    return juce::Time::getCurrentTime().formatted("[%Y-%m-%d %H:%M:%S]");
}

juce::CriticalSection& AppLogger::getLock()
{
    static juce::CriticalSection lock;
    return lock;
}

