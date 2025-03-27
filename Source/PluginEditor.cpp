#include "PluginProcessor.h"
#include "PluginEditor.h"


//==============================================================================

AudioReceiverAudioProcessorEditor::AudioReceiverAudioProcessorEditor (AudioReceiverAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Configure the status label
    addAndMakeVisible(statusLabel);
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setFont(juce::Font(16.0f));
    statusLabel.setText("Connecting...", juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    
    // Start the timer to update status
    startTimer(500); // Update every 500ms
    
    setSize(400, 300);
}


void AudioReceiverAudioProcessorEditor::timerCallback()
{
    if (audioProcessor.isConnectionActive())
    {
        statusLabel.setText("Connected - Receiving Audio", juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::green);
    }
    else if (audioProcessor.isMemoryInitializedAndActive())
    {
        statusLabel.setText("Sender Not Active", juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::red);
    }
    else
    {
        statusLabel.setText("Not Connected", juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::red);
    }
}

AudioReceiverAudioProcessorEditor::~AudioReceiverAudioProcessorEditor()
{
    stopTimer();
}

//==============================================================================
void AudioReceiverAudioProcessorEditor::paint (juce::Graphics& g)
{
        // Background
        g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
        
        // Draw a title at the top
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(20.0f, juce::Font::bold));
        g.drawFittedText("Audio Receiver", getLocalBounds().removeFromTop(50), juce::Justification::centred, 1);
        
        // Draw a border around the status label for visibility
        juce::Rectangle<int> statusBounds = getLocalBounds().reduced(20).removeFromTop(40);
        g.setColour(juce::Colours::darkgrey);
        g.drawRoundedRectangle(statusBounds.toFloat(), 4.0f, 1.0f);
}

void AudioReceiverAudioProcessorEditor::resized()
{
    // Position the status label in the upper part of the UI
    juce::Rectangle<int> area = getLocalBounds();
    area.removeFromTop(70); // Space for the title
    statusLabel.setBounds(area.removeFromTop(40).reduced(20, 0));
}
