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
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{

    //Try initial connection
    initializeConnection();
    
    // Set up timer for reconnection if needed
    reconnectionTimer = std::make_unique<ReconnectionTimer>(*this);
    reconnectionTimer->startTimer(1000);
    
//    // Open shared memory in read-only mode
//    shm_fd = shm_open(SHARED_MEMORY_NAME, O_RDONLY, 0666);
//
//    if (shm_fd == -1)
//        return;
//
//    // Map shared memory
//    void* mappedMemory = mmap(0, MAX_BUFFER_SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
//
//    if (mappedMemory == MAP_FAILED)
//    {
//        close(shm_fd);
//        shm_fd = -1;
//        return;
//    }
//
//    // Cast to shared data structure
//    sharedData = static_cast<SharedAudioData*>(mappedMemory);
//    isMemoryInitialized = true;

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

bool AudioReceiverAudioProcessor::connectToSharedMemory()
{
    // Clean up any existing connection first
    cleanupSharedMemory();
    
    // Attempt to open the shared memory
    shm_fd = shm_open(SHARED_MEMORY_NAME, O_RDONLY, 0666);
    
    if (shm_fd == -1)
    {
        DBG("Failed to open shared memory: " + juce::String(strerror(errno)));
        return false;
    }

    // Map shared memory
    void* mappedMemory = mmap(0, MAX_BUFFER_SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
    
    if (mappedMemory == MAP_FAILED)
    {
        DBG("Failed to map shared memory: " + juce::String(strerror(errno)));
        close(shm_fd);
        shm_fd = -1;
        return false;
    }

    // Cast and check if the data appears valid
    sharedData = static_cast<SharedAudioData*>(mappedMemory);
    isMemoryInitialized = true;
    DBG("Successfully connected to shared memory");
    return true;
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
    juce::Logger::writeToLog("Receiver plugin ready to play");
}

void AudioReceiverAudioProcessor::releaseResources()
{
    cleanupSharedMemory();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AudioReceiverAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void AudioReceiverAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    // Skip processing if shared memory isn't initialized or not active
    if (!isMemoryInitialized || sharedData == nullptr || !sharedData->isActive.load())
    {
        buffer.clear();
        return;
    }
    
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    auto numSamples = buffer.getNumSamples();
    
    // Get current read and write positions
    uint64_t writeIndex = sharedData->writeIndex.load(std::memory_order_acquire);
    uint64_t readIndex = lastReadIndex;
    
    // Calculate how many frames are available to read
    uint64_t available = writeIndex - readIndex;
    
    if (available >= numSamples)
    {
        // Read audio data from ring buffer
        int numChannels = std::min(totalNumOutputChannels,
                                   sharedData->numChannels.load(std::memory_order_acquire));
        
        for (int sample = 0; sample < numSamples; ++sample)
        {
            uint64_t frameIndex = (readIndex + sample) & SharedAudioData::BUFFER_MASK;
            
            for (int channel = 0; channel < numChannels; ++channel)
            {
                int bufferIndex = (frameIndex * numChannels) + channel;
                buffer.setSample(channel, sample, sharedData->audioData[bufferIndex]);
            }
        }
        
        // Update read position
        lastReadIndex = readIndex + numSamples;
        sharedData->readIndex.store(lastReadIndex, std::memory_order_release);
    }
    else
    {
        // Not enough data - underrun situation
        buffer.clear();
        sharedData->metrics.bufferUnderruns.fetch_add(1, std::memory_order_relaxed);
        
        // If we're seriously lagging, skip ahead to avoid major audio artifacts
        if (available < numSamples/2 && writeIndex > numSamples*2)
        {
            // Skip ahead to keep latency manageable, but stay behind writeIndex
            lastReadIndex = writeIndex - numSamples*2;
            sharedData->readIndex.store(lastReadIndex, std::memory_order_release);
        }
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



