#include "MainComponent.h"

MainComponent::MainComponent()
{
    titleLabel.setText("LiveHost", juce::dontSendNotification);
    titleLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(236, 239, 244));
    titleLabel.setFont(juce::FontOptions(32.0f, juce::Font::bold));
    addAndMakeVisible(titleLabel);

    statusLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(150, 159, 174));
    statusLabel.setFont(juce::FontOptions(15.0f));
    addAndMakeVisible(statusLabel);

    pluginStatusLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(150, 159, 174));
    pluginStatusLabel.setFont(juce::FontOptions(15.0f));
    pluginStatusLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(pluginStatusLabel);

    deviceSelector = std::make_unique<juce::AudioDeviceSelectorComponent>(
        audioEngine.getDeviceManager(),
        0,
        64,
        0,
        64,
        true,
        true,
        true,
        false);
    deviceSelector->setItemHeight(24);
    addAndMakeVisible(*deviceSelector);

    pluginListComponent = std::make_unique<juce::PluginListComponent>(
        pluginManager.getFormatManager(),
        pluginManager.getKnownPluginList(),
        pluginManager.getDeadMansPedalFile(),
        pluginManager.getPropertiesFile(),
        true);
    pluginListComponent->setOptionsButtonText("Scan Plugins");
    pluginListComponent->setScanDialogText("Scanning plugins", "LiveHost is scanning Audio Unit and VST3 plugins.");
    pluginListComponent->setNumberOfThreadsForScanning(1);
    addAndMakeVisible(*pluginListComponent);

    setSize(1280, 760);
    refreshDeviceStatus();
    refreshPluginStatus();
    startTimerHz(2);
}

MainComponent::~MainComponent()
{
    stopTimer();
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(22, 24, 28));

    const auto panelArea = getLocalBounds().reduced(32).withTrimmedTop(88);
    g.setColour(juce::Colour::fromRGB(35, 39, 45));
    g.fillRoundedRectangle(panelArea.toFloat(), 8.0f);

    g.setColour(juce::Colour::fromRGB(68, 75, 86));
    g.drawRoundedRectangle(panelArea.toFloat().reduced(0.5f), 8.0f, 1.0f);
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds().reduced(32);

    titleLabel.setBounds(bounds.removeFromTop(44));
    auto statusArea = bounds.removeFromTop(28);
    statusLabel.setBounds(statusArea.removeFromLeft(statusArea.getWidth() / 2));
    pluginStatusLabel.setBounds(statusArea);

    bounds.removeFromTop(32);

    auto content = bounds.reduced(16);
    auto deviceArea = content.removeFromLeft(juce::jmax(320, content.getWidth() / 3));
    content.removeFromLeft(16);

    deviceSelector->setBounds(deviceArea);
    pluginListComponent->setBounds(content);
}

void MainComponent::timerCallback()
{
    refreshDeviceStatus();
    refreshPluginStatus();
}

void MainComponent::refreshDeviceStatus()
{
    statusLabel.setText(audioEngine.getCurrentDeviceSummary(), juce::dontSendNotification);
}

void MainComponent::refreshPluginStatus()
{
    pluginStatusLabel.setText(juce::String(pluginManager.getNumKnownPlugins()) + " plugins indexed",
                              juce::dontSendNotification);
}
