#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "SharedMemoryManager.h"

//==============================================================================
AudioReceiverAudioProcessor::AudioReceiverAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("1/2", juce::AudioChannelSet::stereo(), true)
                       .withOutput("3/4", juce::AudioChannelSet::stereo(), true)
                       .withOutput("1", juce::AudioChannelSet::mono(), true)
                       .withOutput("2", juce::AudioChannelSet::mono(), true)
                       .withOutput("3", juce::AudioChannelSet::mono(), true)
                       .withOutput("4", juce::AudioChannelSet::mono(), true)
                     #endif
                       )
#endif
{
    // Initialize flag to false
    canWriteToSharedMemory = false;

    //Try initial connection
    //initializeConnection();
    //^Cubase might instantiate the plugin before the audio thread is active. //Deferring shared memory setup until prepareToPlay():
    
    // Set up timer for reconnection if needed
    reconnectionTimer = std::make_unique<ReconnectionTimer>(*this);
    reconnectionTimer->startTimer(1000);
    
}

bool AudioReceiverAudioProcessor::initializeConnection()
{
    // This is for first-time initialization only
    DBG("Initializing connection to shared memory...");
    return connectToSharedMemory();
}

bool AudioReceiverAudioProcessor::attemptReconnection()
{
    // Only try to reconnect if not already connected
    if (!isMemoryInitialized || sharedData == nullptr)
    {
        DBG("Attempting to reconnect to shared memory...");
        return connectToSharedMemory();
    }
    return isMemoryInitialized;
}

//connectToSharedMemory will first try to open the shared memory with read-write permission. If that fails, it falls back to read-only mode. It sets the canWriteToSharedMemory flag appropriately.
bool AudioReceiverAudioProcessor::connectToSharedMemory()
{
    // Clean up any existing connection first
    cleanupSharedMemory();
    
    // First attempt to open the shared memory with read-write access
    shm_fd = shm_open(SHARED_MEMORY_NAME, O_RDWR, 0666);
    
    if (shm_fd == -1)
    {
        DBG("Failed to open shared memory with read-write access: " + juce::String(strerror(errno)));
        
        // Try again with read-only as fallback
        shm_fd = shm_open(SHARED_MEMORY_NAME, O_RDONLY, 0666);
        
        if (shm_fd == -1)
        {
            DBG("Failed to open shared memory: " + juce::String(strerror(errno)));
            return false;
        }
        
        // If we get here, we opened in read-only mode
        void* mappedMemory = mmap(0, MAX_BUFFER_SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
        
        if (mappedMemory == MAP_FAILED)
        {
            DBG("Failed to map shared memory: " + juce::String(strerror(errno)));
            sharedData = nullptr;
            close(shm_fd);
            shm_fd = -1;
            return false;
        }
        
        // Mark that we're in read-only mode
        canWriteToSharedMemory = false;
        DBG("Connected to shared memory in READ-ONLY mode");
        
        // Cast and check if the data appears valid
        sharedData = static_cast<SharedAudioData*>(mappedMemory);
        isMemoryInitialized = true;
        return true;
    }
    else
    {
        // We opened in read-write mode successfully
        void* mappedMemory = mmap(0, MAX_BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
        
        if (mappedMemory == MAP_FAILED)
        {
            DBG("Failed to map shared memory with read-write permissions: " + juce::String(strerror(errno)));
            sharedData = nullptr;
            close(shm_fd);
            shm_fd = -1;
            return false;
        }
        
        // Mark that we can write to the shared memory
        canWriteToSharedMemory = true;
        DBG("Connected to shared memory in READ-WRITE mode");
        
        // Cast and check if the data appears valid
        sharedData = static_cast<SharedAudioData*>(mappedMemory);
        isMemoryInitialized = true;
        return true;
    }
}


AudioReceiverAudioProcessor::~AudioReceiverAudioProcessor()
{
    // Stop the timer before destruction
    if (reconnectionTimer != nullptr)
        reconnectionTimer->stopTimer();
    
    reconnectionTimer = nullptr;
    
    cleanupSharedMemory();
}

//==============================================================================

void SharedMemoryManager::cleanupSharedMemory()
{
    if (sharedData != nullptr)
    {
        munmap(sharedData, MAX_BUFFER_SIZE);
        sharedData = nullptr;
    }

    if (shm_fd != -1)
    {
        close(shm_fd);
        shm_fd = -1;
    }
    
    isMemoryInitialized = false;
}

bool SharedMemoryManager::initializeSharedMemory()
{
    // This is just a placeholder implementation since your
    // receiver initializes memory differently
    return true;
}
//==============================================================================

const juce::String AudioReceiverAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioReceiverAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AudioReceiverAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AudioReceiverAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AudioReceiverAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioReceiverAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AudioReceiverAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioReceiverAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String AudioReceiverAudioProcessor::getProgramName (int index)
{
    return {};
}

void AudioReceiverAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void AudioReceiverAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    DBG("Receiver plugin ready to play");
    
    //Cubase might instantiate the plugin before the audio thread is active. //Deferring shared memory setup until prepareToPlay():
    if (!isMemoryInitialized)
        initializeConnection();
}

void AudioReceiverAudioProcessor::releaseResources()
{
    cleanupSharedMemory();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AudioReceiverAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    // Always require the main output bus to have exactly 8 channels.
    if (layouts.getMainOutputChannelSet().size() != 8)
        return false;

    return true;
#endif
}
#endif


//The key changes here are the addition of the if (canWriteToSharedMemory && sharedData != nullptr) checks before any attempt to write to the shared memory.
void AudioReceiverAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Skip processing if shared memory isn't initialized or not active.
    if (!isMemoryInitialized || sharedData == nullptr || !sharedData->isActive.load())
    {
        buffer.clear();
        return;
    }

    int numSamples = buffer.getNumSamples();
    // Get the number of output buses (should be 6 as per your configuration)
    int numOutputBuses = getBusCount(false);

    // Define a static mapping array for the output buses:
    // Each entry: { starting shared channel offset, number of channels for that bus }
    // Bus 0 ("1/2"): offset 0, 2 channels
    // Bus 1 ("3/4"): offset 2, 2 channels
    // Bus 2 ("1"):   offset 4, 1 channel
    // Bus 3 ("2"):   offset 5, 1 channel
    // Bus 4 ("3"):   offset 6, 1 channel
    // Bus 5 ("4"):   offset 7, 1 channel
    static const int busMapping[6][2] = {
        {0, 2},
        {2, 2},
        {4, 1},
        {5, 1},
        {6, 1},
        {7, 1}
    };

    // Total channels in shared memory is 8.
    const int totalSharedChannels = 8;

    // Get current shared memory indices.
    uint64_t writeIndex = sharedData->writeIndex.load(std::memory_order_acquire);
    uint64_t readIndex  = lastReadIndex;
    uint64_t available  = writeIndex - readIndex;

    // Check if we have enough frames available.
    if (available < (uint64_t) numSamples)
    {
        // Not enough data: clear each output bus.
        for (int bus = 0; bus < numOutputBuses; ++bus)
            getBusBuffer(buffer, false, bus).clear();

        if (canWriteToSharedMemory && sharedData != nullptr)
            sharedData->metrics.bufferUnderruns.fetch_add(1, std::memory_order_relaxed);

        // If severely lagging, adjust the read pointer.
        if (available < (uint64_t) numSamples / 2 && writeIndex > (uint64_t) numSamples * 2)
        {
            lastReadIndex = writeIndex - numSamples * 2;
            if (canWriteToSharedMemory && sharedData != nullptr)
                sharedData->readIndex.store(lastReadIndex, std::memory_order_release);
        }
        return;
    }

    // For each output bus, copy the corresponding shared memory data.
    for (int bus = 0; bus < numOutputBuses; ++bus)
    {
        auto&& outBusBuffer = getBusBuffer(buffer, false, bus);
        outBusBuffer.clear();  // Clear the bus buffer.

        int sharedOffset  = busMapping[bus][0];   // Starting shared channel.
        int channelsToCopy = busMapping[bus][1];    // Number of channels for this bus.

        for (int sample = 0; sample < numSamples; ++sample)
        {
            uint64_t frameIndex = (readIndex + sample) & SharedAudioData::BUFFER_MASK;
            for (int ch = 0; ch < channelsToCopy; ++ch)
            {
                int overallSharedChannel = sharedOffset + ch;
                int sharedBufferIndex = (frameIndex * totalSharedChannels) + overallSharedChannel;
                float sampleValue = sharedData->audioData[sharedBufferIndex];
                outBusBuffer.setSample(ch, sample, sampleValue);
            }
        }
    }

    // Update the local read position.
    lastReadIndex = readIndex + numSamples;
    if (canWriteToSharedMemory && sharedData != nullptr)
        sharedData->readIndex.store(lastReadIndex, std::memory_order_release);

    // Apply gain to each output bus and compute RMS level.
    {
        const juce::ScopedLock sl(levelLock);
        for (int bus = 0; bus < numOutputBuses; ++bus)
        {
            auto&& outBusBuffer = getBusBuffer(buffer, false, bus);
            // Removed redundant clear() here so we retain the copied data.
            outBusBuffer.applyGain(0, outBusBuffer.getNumSamples(), gain);
        }
        // Compute overall RMS level (this could be adapted to merge channels if needed).
        currentLevel = AudioLevelUtils::calculateRMSLevel(buffer);
    }
}



//==============================================================================
bool AudioReceiverAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioReceiverAudioProcessor::createEditor()
{
    return new AudioReceiverAudioProcessorEditor (*this);
}

//==============================================================================
void AudioReceiverAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void AudioReceiverAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioReceiverAudioProcessor();
}



