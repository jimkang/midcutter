/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

float calcNextAvg(const float prevAvg, const float smoothingFactor, const float currentValue);

//==============================================================================
MidcutterAudioProcessor::MidcutterAudioProcessor()
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
}

MidcutterAudioProcessor::~MidcutterAudioProcessor()
{
}

//==============================================================================
const juce::String MidcutterAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MidcutterAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool MidcutterAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool MidcutterAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double MidcutterAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MidcutterAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int MidcutterAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MidcutterAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String MidcutterAudioProcessor::getProgramName (int index)
{
    return {};
}

void MidcutterAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void MidcutterAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void MidcutterAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MidcutterAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void MidcutterAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    const float smoothingFactorUp = 0.1;
    const float smoothingFactorDown = 0.7;

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);
        const int bufLength = buffer.getNumSamples();
        for (int i = 0; i < bufLength; ++i) {
          const float sample = channelData[i];
          const float squared = sample * sample;
          if (prevAvg[channel] == -1) {
            prevAvg[channel] = squared;
            continue;
          }
          float smoothingFactor = smoothingFactorDown;
          if (squared > prevAvg[channel]) {
            smoothingFactor = smoothingFactorUp;
          }
          const float avg = calcNextAvg(prevAvg[channel], smoothingFactor, squared);
          channelData[i] = avg;
          prevAvg[channel] = avg;
      }
    }
}

//==============================================================================
bool MidcutterAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* MidcutterAudioProcessor::createEditor()
{
    return new MidcutterAudioProcessorEditor (*this);
}

//==============================================================================
void MidcutterAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void MidcutterAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MidcutterAudioProcessor();
}

float calcNextAvg(const float prevAvg, const float smoothingFactor, const float currentValue) {
  const float inverseSmoothingFactor = 1.0 - smoothingFactor;
  return smoothingFactor * prevAvg + inverseSmoothingFactor * currentValue;
}
