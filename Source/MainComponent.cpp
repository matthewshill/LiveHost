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

    setSize(1280, 760);
    refreshDeviceStatus();
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
    statusLabel.setBounds(bounds.removeFromTop(28));

    bounds.removeFromTop(32);
    deviceSelector->setBounds(bounds.reduced(16));
}

void MainComponent::timerCallback()
{
    refreshDeviceStatus();
}

void MainComponent::refreshDeviceStatus()
{
    statusLabel.setText(audioEngine.getCurrentDeviceSummary(), juce::dontSendNotification);
}
