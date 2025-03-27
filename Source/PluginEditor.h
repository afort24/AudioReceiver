#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class AudioReceiverAudioProcessorEditor  : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    AudioReceiverAudioProcessorEditor (AudioReceiverAudioProcessor&);
    ~AudioReceiverAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    //==============================================================================
    void timerCallback() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AudioReceiverAudioProcessor& audioProcessor;
    
    juce::Label statusLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioReceiverAudioProcessorEditor)
};
