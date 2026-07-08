#include "MainComponent.h"

class MainComponent::PluginEditorWindow final : public juce::DocumentWindow
{
public:
    PluginEditorWindow(juce::String title, std::unique_ptr<juce::AudioProcessorEditor> editor)
        : DocumentWindow(std::move(title),
                         juce::Colour::fromRGB(24, 26, 30),
                         DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar(true);
        setContentOwned(editor.release(), true);
        setResizable(true, true);
        centreWithSize(juce::jmax(480, getWidth()), juce::jmax(320, getHeight()));
        setVisible(true);
    }

    std::function<void()> onClose;

private:
    void closeButtonPressed() override
    {
        if (onClose != nullptr)
            onClose();
    }
};

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

    rackStatusLabel.setText("Rack empty", juce::dontSendNotification);
    rackStatusLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(236, 239, 244));
    rackStatusLabel.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    addAndMakeVisible(rackStatusLabel);

    loadPluginButton.setButtonText("Add Selected");
    loadPluginButton.onClick = [this] { addSelectedPluginToRack(); };
    addAndMakeVisible(loadPluginButton);

    removeSlotButton.setButtonText("Remove");
    removeSlotButton.onClick = [this] { removeSelectedRackSlot(); };
    addAndMakeVisible(removeSlotButton);

    clearPluginButton.setButtonText("Clear Rack");
    clearPluginButton.onClick = [this] { clearRack(); };
    addAndMakeVisible(clearPluginButton);

    bypassPluginButton.setButtonText("Bypass");
    bypassPluginButton.onClick = [this]
    {
        audioEngine.setRackSlotBypassed(getSelectedRackSlotIndex(), bypassPluginButton.getToggleState());
        refreshPluginStatus();
    };
    addAndMakeVisible(bypassPluginButton);

    openEditorButton.setButtonText("Open Editor");
    openEditorButton.onClick = [this] { openSelectedRackSlotEditor(); };
    addAndMakeVisible(openEditorButton);

    rackTitleLabel.setText("Rack 1", juce::dontSendNotification);
    rackTitleLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(236, 239, 244));
    rackTitleLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    addAndMakeVisible(rackTitleLabel);

    rackListBox.setModel(this);
    rackListBox.setRowHeight(34);
    rackListBox.setColour(juce::ListBox::backgroundColourId, juce::Colour::fromRGB(26, 29, 34));
    rackListBox.setColour(juce::ListBox::outlineColourId, juce::Colour::fromRGB(68, 75, 86));
    rackListBox.setOutlineThickness(1);
    addAndMakeVisible(rackListBox);

    scanExclusionsLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(150, 159, 174));
    scanExclusionsLabel.setFont(juce::FontOptions(13.0f));
    addAndMakeVisible(scanExclusionsLabel);

    openScanExclusionsButton.setButtonText("Exclusions");
    openScanExclusionsButton.onClick = [this] { openScanExclusionsFile(); };
    addAndMakeVisible(openScanExclusionsButton);

    reloadScanExclusionsButton.setButtonText("Reload");
    reloadScanExclusionsButton.onClick = [this]
    {
        pluginManager.reloadScanExclusions();
        refreshScanExclusionStatus();
    };
    addAndMakeVisible(reloadScanExclusionsButton);

    inputMeter.setLabel("Input");
    addAndMakeVisible(inputMeter);

    outputMeter.setLabel("Output");
    addAndMakeVisible(outputMeter);

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
    refreshMeters();
    refreshScanExclusionStatus();
    startTimerHz(24);
}

MainComponent::~MainComponent()
{
    stopTimer();
    closePluginEditor();
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
    rackTitleLabel.setBounds(pluginControls.removeFromLeft(84));
    rackStatusLabel.setBounds(pluginControls.removeFromLeft(juce::jmax(180, pluginControls.getWidth() - 584)));
    bypassPluginButton.setBounds(pluginControls.removeFromLeft(92).reduced(4, 0));
    openEditorButton.setBounds(pluginControls.removeFromLeft(124).reduced(4, 0));
    removeSlotButton.setBounds(pluginControls.removeFromLeft(104).reduced(4, 0));
    loadPluginButton.setBounds(pluginControls.removeFromLeft(132).reduced(4, 0));
    clearPluginButton.setBounds(pluginControls.reduced(4, 0));

    content.removeFromTop(12);
    rackListBox.setBounds(content.removeFromTop(132));

    content.removeFromTop(12);
    auto meters = content.removeFromTop(46);
    inputMeter.setBounds(meters.removeFromLeft(meters.getWidth() / 2).reduced(0, 0).withTrimmedRight(8));
    outputMeter.setBounds(meters.reduced(0, 0).withTrimmedLeft(8));

    content.removeFromTop(16);

    auto scanTools = content.removeFromTop(28);
    scanExclusionsLabel.setBounds(scanTools.removeFromLeft(juce::jmax(180, scanTools.getWidth() - 216)));
    openScanExclusionsButton.setBounds(scanTools.removeFromLeft(112).reduced(4, 0));
    reloadScanExclusionsButton.setBounds(scanTools.reduced(4, 0));

    content.removeFromTop(10);
    pluginListComponent->setBounds(content);
}

void MainComponent::timerCallback()
{
    refreshDeviceStatus();
    refreshPluginStatus();
    refreshMeters();
    refreshScanExclusionStatus();
}

int MainComponent::getNumRows()
{
    return audioEngine.getNumRackSlots();
}

void MainComponent::paintListBoxItem(int rowNumber,
                                     juce::Graphics& g,
                                     int width,
                                     int height,
                                     bool rowIsSelected)
{
    const auto slots = audioEngine.getRackSlotInfos();

    if (! juce::isPositiveAndBelow(rowNumber, static_cast<int>(slots.size())))
        return;

    const auto& slot = slots[static_cast<size_t>(rowNumber)];
    const auto background = rowIsSelected ? juce::Colour::fromRGB(48, 92, 128)
                                          : juce::Colour::fromRGB(30, 34, 40);

    g.fillAll(background);

    g.setColour(juce::Colour::fromRGB(74, 82, 94));
    g.drawHorizontalLine(height - 1, 0.0f, static_cast<float>(width));

    auto rowBounds = juce::Rectangle<int>(0, 0, width, height).reduced(10, 0);
    const auto indexText = juce::String(rowNumber + 1).paddedLeft('0', 2);

    g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    g.setColour(juce::Colour::fromRGB(150, 159, 174));
    g.drawText(indexText, rowBounds.removeFromLeft(38), juce::Justification::centredLeft);

    g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    g.setColour(slot.bypassed ? juce::Colour::fromRGB(142, 148, 156)
                              : juce::Colour::fromRGB(236, 239, 244));
    g.drawText(slot.name, rowBounds.removeFromLeft(juce::jmax(80, rowBounds.getWidth() - 110)),
               juce::Justification::centredLeft);

    g.setFont(juce::FontOptions(12.0f));
    g.setColour(slot.bypassed ? juce::Colour::fromRGB(240, 184, 98)
                              : juce::Colour::fromRGB(126, 198, 153));
    g.drawText(slot.bypassed ? "BYPASS" : "ACTIVE", rowBounds, juce::Justification::centredRight);
}

void MainComponent::selectedRowsChanged(int)
{
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
    rackStatusLabel.setText(audioEngine.getRackSummary(), juce::dontSendNotification);

    const auto slots = audioEngine.getRackSlotInfos();
    const auto selectedSlot = getSelectedRackSlotIndex();
    const auto hasSelectedSlot = juce::isPositiveAndBelow(selectedSlot, static_cast<int>(slots.size()));

    bypassPluginButton.setToggleState(hasSelectedSlot && slots[static_cast<size_t>(selectedSlot)].bypassed,
                                      juce::dontSendNotification);
    bypassPluginButton.setEnabled(hasSelectedSlot);
    openEditorButton.setEnabled(hasSelectedSlot && slots[static_cast<size_t>(selectedSlot)].hasEditor);
    removeSlotButton.setEnabled(hasSelectedSlot);
    clearPluginButton.setEnabled(! slots.empty());
    rackListBox.updateContent();
    rackListBox.repaint();
}

void MainComponent::addSelectedPluginToRack()
{
    const auto selectedRow = pluginListComponent->getTableListBox().getSelectedRow();
    const auto description = pluginManager.getPluginDescriptionAt(selectedRow);

    if (! description.has_value())
    {
        rackStatusLabel.setText("Select a scanned plugin first", juce::dontSendNotification);
        return;
    }

    rackStatusLabel.setText("Adding " + description->name + "...", juce::dontSendNotification);

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
        rackStatusLabel.setText("Plugin load failed: " + error, juce::dontSendNotification);
        return;
    }

    audioEngine.addPluginToRack(std::move(plugin));
    rackListBox.selectRow(audioEngine.getNumRackSlots() - 1);
    refreshPluginStatus();
}

void MainComponent::removeSelectedRackSlot()
{
    closePluginEditor();

    const auto selectedSlot = getSelectedRackSlotIndex();
    audioEngine.removeRackSlot(selectedSlot);

    const auto numSlots = audioEngine.getNumRackSlots();

    if (numSlots > 0)
        rackListBox.selectRow(juce::jmin(selectedSlot, numSlots - 1));

    refreshPluginStatus();
}

void MainComponent::clearRack()
{
    closePluginEditor();
    audioEngine.clearRack();
    rackListBox.deselectAllRows();
    refreshPluginStatus();
}

void MainComponent::openSelectedRackSlotEditor()
{
    closePluginEditor();

    const auto selectedSlot = getSelectedRackSlotIndex();
    const auto slots = audioEngine.getRackSlotInfos();

    if (! juce::isPositiveAndBelow(selectedSlot, static_cast<int>(slots.size())))
        return;

    auto editor = audioEngine.createRackSlotEditor(selectedSlot);

    if (editor == nullptr)
    {
        rackStatusLabel.setText(slots[static_cast<size_t>(selectedSlot)].name + " has no editor",
                                juce::dontSendNotification);
        return;
    }

    pluginEditorWindow = std::make_unique<PluginEditorWindow>(slots[static_cast<size_t>(selectedSlot)].name,
                                                              std::move(editor));
    pluginEditorWindow->onClose = [this] { closePluginEditor(); };
}

void MainComponent::closePluginEditor()
{
    pluginEditorWindow = nullptr;
}

int MainComponent::getSelectedRackSlotIndex() const
{
    return rackListBox.getSelectedRow();
}

void MainComponent::refreshMeters()
{
    inputMeter.setLevel(audioEngine.getInputPeakLevel());
    outputMeter.setLevel(audioEngine.getOutputPeakLevel());
}

void MainComponent::refreshScanExclusionStatus()
{
    scanExclusionsLabel.setText(juce::String(pluginManager.getNumScanExclusions()) + " scan exclusions active",
                                juce::dontSendNotification);
}

void MainComponent::openScanExclusionsFile()
{
    pluginManager.getScanExclusionsFile().startAsProcess();
}
