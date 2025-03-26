#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================

AudioReceiverAudioProcessorEditor::AudioReceiverAudioProcessorEditor (AudioReceiverAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    addAndMakeVisible(statusLabel);
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setFont(juce::Font(16.0f));
    
    setSize (400, 300);
}

void AudioReceiverAudioProcessorEditor::timerCallback()
{
    if (audioProcessor.sharedData != nullptr && audioProcessor.isMemoryInitialized)
    {
        if (audioProcessor.sharedData->isActive.load())
        {
            statusLabel.setText("Connected - Receiving Audio", juce::dontSendNotification);
            statusLabel.setColour(juce::Label::textColourId, juce::Colours::green);
        }
        else
        {
            statusLabel.setText("Sender Not Active", juce::dontSendNotification);
            statusLabel.setColour(juce::Label::textColourId, juce::Colours::red);
        }
    }
    else
    {
        statusLabel.setText("Not Connected", juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::red);
    }
}

AudioReceiverAudioProcessorEditor::~AudioReceiverAudioProcessorEditor()
{
}

//==============================================================================
void AudioReceiverAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (15.0f));
    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void AudioReceiverAudioProcessorEditor::resized()
{
    statusLabel.setBounds(getLocalBounds().reduced(10));
}
