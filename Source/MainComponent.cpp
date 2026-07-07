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

    activePluginLabel.setText("No plugin loaded", juce::dontSendNotification);
    activePluginLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(236, 239, 244));
    activePluginLabel.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    addAndMakeVisible(activePluginLabel);

    loadPluginButton.setButtonText("Load Selected");
    loadPluginButton.onClick = [this] { loadSelectedPlugin(); };
    addAndMakeVisible(loadPluginButton);

    clearPluginButton.setButtonText("Clear");
    clearPluginButton.onClick = [this]
    {
        audioEngine.clearActivePlugin();
        refreshPluginStatus();
    };
    addAndMakeVisible(clearPluginButton);

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

    auto pluginControls = content.removeFromTop(32);
    activePluginLabel.setBounds(pluginControls.removeFromLeft(juce::jmax(260, pluginControls.getWidth() - 260)));
    loadPluginButton.setBounds(pluginControls.removeFromLeft(140).reduced(4, 0));
    clearPluginButton.setBounds(pluginControls.reduced(4, 0));

    content.removeFromTop(12);
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
    activePluginLabel.setText(audioEngine.getActivePluginName(), juce::dontSendNotification);
}

void MainComponent::loadSelectedPlugin()
{
    const auto selectedRow = pluginListComponent->getTableListBox().getSelectedRow();
    const auto description = pluginManager.getPluginDescriptionAt(selectedRow);

    if (! description.has_value())
    {
        activePluginLabel.setText("Select a scanned plugin first", juce::dontSendNotification);
        return;
    }

    activePluginLabel.setText("Loading " + description->name + "...", juce::dontSendNotification);

    juce::Component::SafePointer safeThis(this);

    pluginManager.createPluginInstanceAsync(*description,
                                            audioEngine.getCurrentSampleRate(),
                                            audioEngine.getCurrentBufferSizeSamples(),
                                            [safeThis](std::unique_ptr<juce::AudioPluginInstance> plugin,
                                                       const juce::String& error)
                                            {
                                                if (safeThis != nullptr)
                                                    safeThis->handlePluginCreated(std::move(plugin), error);
                                            });
}

void MainComponent::handlePluginCreated(std::unique_ptr<juce::AudioPluginInstance> plugin,
                                        const juce::String& error)
{
    if (plugin == nullptr)
    {
        activePluginLabel.setText("Plugin load failed: " + error, juce::dontSendNotification);
        return;
    }

    audioEngine.setActivePlugin(std::move(plugin));
    refreshPluginStatus();
}
