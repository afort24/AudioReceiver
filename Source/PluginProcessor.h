#pragma once

#include <JuceHeader.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "/Users/alexanderfortunato/Development/JUCE/AudioSender/Source/SharedMemoryManager.h"

class AudioReceiverAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    AudioReceiverAudioProcessor();
    ~AudioReceiverAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    
//    static constexpr const char* SHARED_MEMORY_NAME = "/my_shared_audio_buffer";
//    static constexpr size_t BUFFER_SIZE = 48000 * 2 * sizeof(float); // 1 second of stereo audio at 48kHz
//
//    int shm_fd;
//    float* audioBuffer;
    
    uint64_t lastReadIndex = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioReceiverAudioProcessor)
};
