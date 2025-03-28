#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

#include "AudioMeterFader.h"
#include "AudioLevelUtils.h"

//==============================================================================
/**
*/
class AudioReceiverAudioProcessorEditor  : public juce::AudioProcessorEditor, private juce::Timer, private juce::Slider::Listener
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
    
    void sliderValueChanged(juce::Slider* slider) override;
    AudioMeterFader audioMeter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioReceiverAudioProcessorEditor)
};
