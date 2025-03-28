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
    
    //Meter:
    addAndMakeVisible(audioMeter);
    audioMeter.setGainEnabled(true);  // Enable gain control
    audioMeter.getSlider().addListener(this);

    
    // Start the timer to update status
//    startTimer(500); // Update every 500ms
    
    startTimerHz(24);
    
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
    
    {
        float level;
        {
            const juce::ScopedLock sl(audioProcessor.levelLock);
            level = audioProcessor.currentLevel;
        }

        audioMeter.setLevel(level);
    }
}

AudioReceiverAudioProcessorEditor::~AudioReceiverAudioProcessorEditor()
{
    audioMeter.getSlider().removeListener(this);
    stopTimer();
}

//==============================================================================
//Meter:
void AudioReceiverAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &audioMeter.getSlider())
    {
        // Pass slider value to processor (linear gain)
        audioProcessor.gain = audioMeter.getGainLinear();
    }
}

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
    
    // UTF-8 Encoded String (First Part with Copyright Symbol)
    const char* utf8Text1 = u8"Alex Fortunato Music ©";
    juce::String footerText1 = juce::String::fromUTF8(utf8Text1, static_cast<int>(std::strlen(utf8Text1)));
    
    // Second part of string ("2025") separately
    juce::String footerText2 = " 2025";
    
    // Website label
    juce::String websiteText = "alexfortunatomusic.com";

    // Combine full footer lines
    juce::String footerLine1 = footerText1 + footerText2;
    juce::String footerLine2 = websiteText;

    // Set font and color
    g.setFont(juce::Font(12.0f, juce::Font::plain));
    g.setColour(juce::Colours::goldenrod);

    // Define area near bottom for footer
    auto bounds = getLocalBounds().reduced(10);
    auto footerArea = bounds.removeFromBottom(30); // Adjust height if needed

    // Split footer area for two lines
    juce::Rectangle<int> line1 = footerArea.removeFromTop(15);
    juce::Rectangle<int> line2 = footerArea;

    g.drawFittedText(footerLine1, line1, juce::Justification::centred, 1);
    g.drawFittedText(footerLine2, line2, juce::Justification::centred, 1);
}



void AudioReceiverAudioProcessorEditor::resized()
{
    
    
    // Position the status label in the upper part of the UI
    juce::Rectangle<int> area = getLocalBounds();
    area.removeFromTop(70); // Space for the title
    statusLabel.setBounds(area.removeFromTop(40).reduced(20, 0));
    
    //Meter:
    // Place the audio meter on the left side
    audioMeter.setBounds(area.removeFromRight(80));
}
