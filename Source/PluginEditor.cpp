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
        statusLabel.setText("Connected to AudioSender", juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::lime);
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
    // Gradient background (same as TransportSender)
    juce::ColourGradient backgroundGradient(
        juce::Colour::fromRGB(30, 30, 30), // Dark grey top
        0, 0,
        juce::Colours::black,              // Black bottom
        0, static_cast<float>(getHeight()),
        false
    );

    backgroundGradient.addColour(0.8, juce::Colours::black);
    g.setGradientFill(backgroundGradient);
    g.fillRect(getLocalBounds());

    // Title Text (without box)
    g.setColour(juce::Colours::goldenrod);
    g.setFont(juce::Font(20.0f, juce::Font::bold));

    // Create a smaller rectangle and center the text inside it
    juce::Rectangle<int> titleArea = juce::Rectangle<int>(0, 15, getWidth(), 30);
    g.drawFittedText("Audio Receiver", titleArea, juce::Justification::centred, 1);
}



void AudioReceiverAudioProcessorEditor::resized()
{
    // Position the status label in the upper part of the UI
    juce::Rectangle<int> area = getLocalBounds();
    area.removeFromTop(70); // Space for the title
    statusLabel.setBounds(area.removeFromTop(40).reduced(20, 0));
}
