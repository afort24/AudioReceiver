#pragma once

#include <JuceHeader.h>
#include "SharedMemoryManager.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>



class AudioReceiverAudioProcessor  : public juce::AudioProcessor, public SharedMemoryManager
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
    
    //New:
    bool isMemoryInitializedAndActive() const
    {
        return isMemoryInitialized && sharedData != nullptr && sharedData->isActive.load();
    }
    
    bool isConnectionActive() const
       {
           return isMemoryInitialized && sharedData != nullptr && sharedData->isActive.load();
       }
    
    double getCurrentLatency() const
        {
            return isMemoryInitialized && sharedData != nullptr ?
                   sharedData->metrics.currentLatency.load() : 0.0;
        }
    
    //New:
    bool attemptReconnection();  // For subsequent reconnection attempts
    
    //Meter/gain stuff
    float gain = 1.0f;           // Linear gain multiplier
    float currentLevel = -60.0f; // Post-gain RMS level in dB
    juce::CriticalSection levelLock;

private:
    
    // Flag to indicate if we can write to the shared memory
    bool canWriteToSharedMemory = false;
    
    uint64_t lastReadIndex = 0;

    // Timer for reconnection attempts
    class ReconnectionTimer : public juce::Timer
        {
        public:
            ReconnectionTimer(AudioReceiverAudioProcessor& processor) : owner(processor) {}
            void timerCallback() override { owner.attemptReconnection(); }
        private:
            AudioReceiverAudioProcessor& owner;
        };
        
    bool initializeConnection(); // First-time initialization
    bool connectToSharedMemory(); // Common connection logic
    
    std::unique_ptr<ReconnectionTimer> reconnectionTimer;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioReceiverAudioProcessor)
};
