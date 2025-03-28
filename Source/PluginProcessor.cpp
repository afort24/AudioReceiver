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

//The key changes here are the addition of the if (canWriteToSharedMemory && sharedData != nullptr) checks before any attempt to write to the shared memory.
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
        
        // Update local read position
        lastReadIndex = readIndex + numSamples;
        
        // Only try to write to shared memory if we have permission
        if (canWriteToSharedMemory && sharedData != nullptr)
        {
            sharedData->readIndex.store(lastReadIndex, std::memory_order_release);
        }

        // =============================
        // ✅ Apply gain and compute level (thread-safe)
        // =============================
        {
            const juce::ScopedLock sl(levelLock); // lock access to gain and level
            buffer.applyGain(gain); // gain is a float member variable set by the editor
            currentLevel = AudioLevelUtils::calculateRMSLevel(buffer);
        }
    }
    else
    {
        // Not enough data - underrun situation
        buffer.clear();
        
        // Only try to write to shared memory if we have permission
        if (canWriteToSharedMemory && sharedData != nullptr)
        {
            sharedData->metrics.bufferUnderruns.fetch_add(1, std::memory_order_relaxed);
        }
        
        // If we're seriously lagging, skip ahead to avoid major audio artifacts
        if (available < numSamples/2 && writeIndex > numSamples*2)
        {
            // Skip ahead to keep latency manageable, but stay behind writeIndex
            lastReadIndex = writeIndex - numSamples*2;
            
            // Only try to write to shared memory if we have permission
            if (canWriteToSharedMemory && sharedData != nullptr)
            {
                sharedData->readIndex.store(lastReadIndex, std::memory_order_release);
            }
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



