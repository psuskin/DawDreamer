#include "../../../Source/RenderEngine.h"
#include "../../../Source/PlaybackProcessor.h"
#include "../../../Source/PluginProcessor.h"

#include <cstdio>

#include <chrono>

#define SAMPLE_RATE 44100
#define BLOCK_SIZE 512

#define BPM 160

#define save 1

// Plugins
std::string Serum = "C:/Program Files/Common Files/VST2/Serum_x64.dll";
std::string LFOTool = "C:/Program Files/Common Files/VST2/LFOTool_x64.dll";
std::string OTT = "C:/Program Files/Common Files/VST2/OTT_x64.dll";
std::string DJMFilter = "C:/Program Files/Common Files/VST2/DJMFilter_x64.dll";

std::string VOS = "C:/Program Files/Steinberg/VSTPlugins/TDR VOS SlickEQ GE.dll";
std::string Kotelnikov = "C:/Program Files/Steinberg/VSTPlugins/TDR Kotelnikov GE.dll";
std::string Limiter = "C:/Program Files/Steinberg/VSTPlugins/TDR Limiter 6 GE.dll";
std::string Nova = "C:/Program Files/Steinberg/VSTPlugins/TDR Nova GE.dll";

std::string Sitala = "C:/Program Files/Steinberg/VSTPlugins/Sitala.dll";

std::string drumsMidi = "C:/Users/psusk/source/repos/Python/vize/Int/stem-drums.mid";
std::string bassMidi = "C:/Users/psusk/source/repos/Python/vize/Int/stem-bass.mid";
std::string melodyLoop = "C:/Users/psusk/source/repos/Python/vize/Int/melodyNoEffects.wav";
//std::string SFXLoop = "C:/Users/psusk/source/repos/Python/vize/Int/SFXNoEffects.wav";
//std::string SFX2Loop = "C:/Users/psusk/source/repos/Python/vize/Int/SFX2NoEffects.wav";
std::string SFXLoop = "C:/Users/psusk/source/repos/Python/vize/Int/SFX.wav";
std::string SFX2Loop = "C:/Users/psusk/source/repos/Python/vize/Int/SFX2.wav";

std::string sitalaState = "C:/Users/psusk/source/repos/Python/vize/states/Sitala/Drum Kit - Trap 001.json";
std::string serumPreset = "C:/Users/psusk/source/repos/Python/vize/Serum Presets/All Presets/THH_Tonal808_Punchy_1_Long.fxp";

std::string VOSStateDir = "C:/Users/psusk/source/repos/Python/vize/states/VOS/";
std::string KotelnikovStateDir = "C:/Users/psusk/source/repos/Python/vize/states/Kotelnikov/";
std::string LimiterStateDir = "C:/Users/psusk/source/repos/Python/vize/states/Limiter/";

struct PluginData {
  std::string preset = "";
  std::string state = "";
  std::vector<std::pair<int, float>> parameters;
  std::vector<std::pair<int, pybind11::array>> automation;
};

void saveToFile(juce::AudioSampleBuffer& audio, std::string outputPath = "C:/Users/psusk/Downloads/out.wav") {
  if (!save) {
    return;
  }

  std::filesystem::remove(outputPath);

  juce::File outputFile(outputPath);
  auto outStream = outputFile.createOutputStream();

  AudioFormatManager formatManager;
  formatManager.registerBasicFormats();

  std::unique_ptr<juce::AudioFormatWriter> writer(formatManager.findFormatForFileExtension("wav")->createWriterFor(outStream.get(), SAMPLE_RATE, audio.getNumChannels(), 32, juce::StringPairArray(), 0));

  writer->writeFromAudioSampleBuffer(audio, 0, audio.getNumSamples());

  outStream.release();
}

juce::AudioSampleBuffer loadFromFile(std::string inputPath) {
  juce::File inputFile(inputPath);
  auto inStream = inputFile.createInputStream();

  juce::AudioFormatManager formatManager;
  formatManager.registerBasicFormats();

  std::unique_ptr<juce::AudioFormatReader> reader(formatManager.findFormatForFileExtension("wav")->createReaderFor(inStream.get(), true));

  juce::AudioSampleBuffer buffer(reader->numChannels, reader->lengthInSamples);
  reader->read(&buffer, 0, reader->lengthInSamples, 0, true, true);

  inStream.release();

  return buffer;
}

PluginProcessorWrapper* loadPluginProcessor(RenderEngine& engine, std::string pluginPath, PluginData pluginData, std::string processorName) {
  PluginProcessorWrapper* plugin = engine.makePluginProcessor(processorName, pluginPath);

  if (pluginData.preset != "") {
    plugin->loadPreset(pluginData.preset);
  }
  else if (pluginData.state != "") {
    plugin->loadStateInformation(pluginData.state);
  }
  if (!pluginData.parameters.empty()) {
    for (const auto& pair : pluginData.parameters) {
      plugin->setParameter(pair.first, pair.second);
    }
  }
  if (!pluginData.automation.empty()) {
    for (const auto& pair : pluginData.automation) {
      int index = pair.first;
      plugin->setAutomationByIndex(index, pair.second, 0);
    }
  }

  return plugin;
}

void loadGraph(RenderEngine& engine, std::pair<std::string, PluginData> synthPlugin, std::vector<std::pair<std::string, PluginData>> effectPlugins, std::string midi) {
  DAG graph;

  auto& [synthPluginPath, synthPluginData] = synthPlugin;

  auto synth = loadPluginProcessor(engine, synthPluginPath, synthPluginData, "synth");

  synth->loadMidi(midi, true, false, true);

  graph.nodes.push_back({ synth, {} });

  size_t i = 0;
  std::string previousNode = "synth";
  for (const auto& pluginPair : effectPlugins) {
    auto& [pluginPath, pluginData] = pluginPair;

    auto effect = loadPluginProcessor(engine, pluginPath, pluginData, "effect" + std::to_string(i));

    graph.nodes.push_back({ effect, {previousNode} });

    i++;
    //previousNode = "effect" + std::to_string(i);
  }

  engine.loadGraph(graph);
}

void loadGraph(RenderEngine& engine, std::vector<std::pair<std::string, PluginData>> effectPlugins, juce::AudioSampleBuffer audio) {
  DAG graph;

  auto playback = engine.makePlaybackProcessor("playback", audio);

  graph.nodes.push_back({ playback, {} });

  size_t i = 0;
  std::string previousNode = "playback";
  for (const auto& pluginPair : effectPlugins) {
    auto& [pluginPath, pluginData] = pluginPair;

    auto effect = loadPluginProcessor(engine, pluginPath, pluginData, "effect" + std::to_string(i));

    graph.nodes.push_back({ effect, {previousNode} });

    i++;
    //previousNode = "effect" + std::to_string(i);
  }

  engine.loadGraph(graph);
}

std::tuple<std::pair<std::string, PluginData>, std::vector<std::pair<std::string, PluginData>>, std::string> getDrums() {
  PluginData synthPluginData;
  synthPluginData.state = sitalaState;
  std::pair<std::string, PluginData> synthPlugin = std::make_pair(Sitala, synthPluginData);

  std::vector<std::pair<std::string, PluginData>> effectPlugins;

  PluginData effect1PluginData;
  effect1PluginData.state = VOSStateDir + "LowCut40Hz.json";
  effectPlugins.push_back(std::make_pair(VOS, effect1PluginData));

  PluginData effect2PluginData;
  effect2PluginData.state = LimiterStateDir + "Limit-0dB.json";
  effectPlugins.push_back(std::make_pair(Limiter, effect2PluginData));

  PluginData effect3PluginData;
  effect3PluginData.state = VOSStateDir + "LowCut40Hz.json";
  effectPlugins.push_back(std::make_pair(VOS, effect3PluginData));

  PluginData effect4PluginData;
  effect4PluginData.state = LimiterStateDir + "Limit-6dBDrums.json";
  effectPlugins.push_back(std::make_pair(Limiter, effect4PluginData));

  return std::make_tuple(synthPlugin, effectPlugins, drumsMidi);
}

std::tuple<std::pair<std::string, PluginData>, std::vector<std::pair<std::string, PluginData>>, std::string> getBass() {
  PluginData synthPluginData;
  synthPluginData.preset = serumPreset;
  std::pair<std::string, PluginData> synthPlugin = std::make_pair(Serum, synthPluginData);

  std::vector<std::pair<std::string, PluginData>> effectPlugins;

  PluginData effect1PluginData;
  effect1PluginData.state = LimiterStateDir + "Limit-12dB.json";
  effectPlugins.push_back(std::make_pair(Limiter, effect1PluginData));

  PluginData effect2PluginData;
  effect2PluginData.state = LimiterStateDir + "Limit-8dB.json";
  effectPlugins.push_back(std::make_pair(Limiter, effect2PluginData));

  return std::make_tuple(synthPlugin, effectPlugins, bassMidi);
}

std::tuple<std::vector<std::pair<std::string, PluginData>>, juce::AudioSampleBuffer> getMelody() {
  std::vector<std::pair<std::string, PluginData>> effectPlugins;

  PluginData effect1PluginData;
  effect1PluginData.state = VOSStateDir + "LowCut200Hz.json";
  effectPlugins.push_back(std::make_pair(VOS, effect1PluginData));

  PluginData effect2PluginData;
  effect2PluginData.state = LimiterStateDir + "Limit-18dB.json";
  effectPlugins.push_back(std::make_pair(Limiter, effect2PluginData));

  PluginData effect3PluginData;
  effect3PluginData.state = VOSStateDir + "LowCut200Hz.json";
  effectPlugins.push_back(std::make_pair(VOS, effect3PluginData));

  PluginData effect4PluginData;
  effect4PluginData.state = LimiterStateDir + "Limit-12dB.json";
  effectPlugins.push_back(std::make_pair(Limiter, effect4PluginData));

  auto melodyBuffer = loadFromFile(melodyLoop);

  return std::make_tuple(effectPlugins, melodyBuffer);
}

std::tuple<std::vector<std::pair<std::string, PluginData>>, juce::AudioSampleBuffer> getSFX() {
  std::vector<std::pair<std::string, PluginData>> effectPlugins;

  PluginData effect1PluginData;
  effect1PluginData.state = VOSStateDir + "LowCut350Hz.json";
  effectPlugins.push_back(std::make_pair(VOS, effect1PluginData));

  PluginData effect2PluginData;
  effect2PluginData.state = LimiterStateDir + "Limit-24dB.json";
  effectPlugins.push_back(std::make_pair(Limiter, effect2PluginData));

  PluginData effect3PluginData;
  effect3PluginData.state = VOSStateDir + "LowCut350Hz.json";
  effectPlugins.push_back(std::make_pair(VOS, effect3PluginData));

  PluginData effect4PluginData;
  effect4PluginData.state = KotelnikovStateDir + "SSLCompression2BusTight.json";
  effectPlugins.push_back(std::make_pair(Kotelnikov, effect4PluginData));

  auto SFXBuffer = loadFromFile(SFXLoop);

  return std::make_tuple(effectPlugins, SFXBuffer);
}

std::tuple<std::vector<std::pair<std::string, PluginData>>, juce::AudioSampleBuffer> getSFX2() {
  std::vector<std::pair<std::string, PluginData>> effectPlugins;

  PluginData effect1PluginData;
  effect1PluginData.state = LimiterStateDir + "Limit-24dB.json";
  effectPlugins.push_back(std::make_pair(Limiter, effect1PluginData));

  auto SFX2Buffer = loadFromFile(SFX2Loop);

  return std::make_tuple(effectPlugins, SFX2Buffer);
}

void multithread() {
  juce::MessageManager::getInstance();

  auto& [drumsSynthPlugin, drumsEffectPlugins, drumsMidiFile] = getDrums();
  RenderEngine engineDrums(SAMPLE_RATE, BLOCK_SIZE);
  engineDrums.setBPM(BPM);
  loadGraph(engineDrums, drumsSynthPlugin, drumsEffectPlugins, drumsMidiFile);

  auto& [bassSynthPlugin, bassEffectPlugins, bassMidiFile] = getBass();
  RenderEngine engineBass(SAMPLE_RATE, BLOCK_SIZE);
  engineBass.setBPM(BPM);
  loadGraph(engineBass, bassSynthPlugin, bassEffectPlugins, bassMidiFile);

  auto& [melodyEffectPlugins, melodyBuffer] = getMelody();
  RenderEngine engineMelody(SAMPLE_RATE, BLOCK_SIZE);
  engineMelody.setBPM(BPM);
  loadGraph(engineMelody, melodyEffectPlugins, melodyBuffer);

  auto& [SFXEffectPlugins, SFXBuffer] = getSFX();
  RenderEngine engineSFX(SAMPLE_RATE, BLOCK_SIZE);
  engineSFX.setBPM(BPM);
  loadGraph(engineSFX, SFXEffectPlugins, SFXBuffer);

  auto& [SFX2EffectPlugins, SFX2Buffer] = getSFX2();
  RenderEngine engineSFX2(SAMPLE_RATE, BLOCK_SIZE);
  engineSFX2.setBPM(BPM);
  loadGraph(engineSFX2, SFX2EffectPlugins, SFX2Buffer);

  auto func = static_cast<bool (RenderEngine::*)(double, bool)>(&RenderEngine::render);
  std::thread t1(func, &engineDrums, 150., false);
  std::thread t2(func, &engineBass, 150., false);
  std::thread t3(func, &engineMelody, 150., false);
  std::thread t4(func, &engineSFX, 150., false);
  std::thread t5(func, &engineSFX2, 150., false);
  t1.join();
  t2.join();
  t3.join();
  t4.join();
  t5.join();

  auto drums = engineDrums.getAudioFrames();
  auto bass = engineBass.getAudioFrames();
  auto melody = engineMelody.getAudioFrames();
  auto SFX = engineSFX.getAudioFrames();
  auto SFX2 = engineSFX2.getAudioFrames();

  saveToFile(drums, "C:/Users/psusk/Downloads/drums.wav");
  saveToFile(bass, "C:/Users/psusk/Downloads/bass.wav");
  saveToFile(melody, "C:/Users/psusk/Downloads/melody.wav");
  saveToFile(SFX, "C:/Users/psusk/Downloads/SFX.wav");
  saveToFile(SFX2, "C:/Users/psusk/Downloads/SFX2.wav");

  int numChannels = drums.getNumChannels();
  int numSamples = drums.getNumSamples();

  juce::AudioSampleBuffer mixdown(numChannels, numSamples);
  mixdown.clear();

  for (int channel = 0; channel < numChannels; channel++) {
    mixdown.addFrom(channel, 0, drums.getReadPointer(channel), numSamples);
    mixdown.addFrom(channel, 0, bass.getReadPointer(channel), numSamples);
    mixdown.addFrom(channel, 0, melody.getReadPointer(channel), numSamples);
    mixdown.addFrom(channel, 0, SFX.getReadPointer(channel), numSamples);
    mixdown.addFrom(channel, 0, SFX2.getReadPointer(channel), numSamples);
  }

  saveToFile(mixdown, "C:/Users/psusk/Downloads/mixdown.wav");

  std::vector<std::pair<std::string, PluginData>> masterEffectPlugins;

  PluginData masterEffect1PluginData;
  masterEffect1PluginData.state = LimiterStateDir + "Limit-0dB.json";
  masterEffectPlugins.push_back(std::make_pair(Limiter, masterEffect1PluginData));

  RenderEngine engineMaster(SAMPLE_RATE, BLOCK_SIZE);
  engineMaster.setBPM(BPM);
  loadGraph(engineMaster, masterEffectPlugins, mixdown);

  saveToFile(mixdown, "C:/Users/psusk/Downloads/master.wav");
}

void nonmultithread() {
  juce::MessageManager::getInstance();

  auto& [drumsSynthPlugin, drumsEffectPlugins, drumsMidiFile] = getDrums();
  RenderEngine engineDrums(SAMPLE_RATE, BLOCK_SIZE);
  engineDrums.setBPM(BPM);
  loadGraph(engineDrums, drumsSynthPlugin, drumsEffectPlugins, drumsMidiFile);

  auto& [bassSynthPlugin, bassEffectPlugins, bassMidiFile] = getBass();
  RenderEngine engineBass(SAMPLE_RATE, BLOCK_SIZE);
  engineBass.setBPM(BPM);
  loadGraph(engineBass, bassSynthPlugin, bassEffectPlugins, bassMidiFile);

  auto& [melodyEffectPlugins, melodyBuffer] = getMelody();
  RenderEngine engineMelody(SAMPLE_RATE, BLOCK_SIZE);
  engineMelody.setBPM(BPM);
  loadGraph(engineMelody, melodyEffectPlugins, melodyBuffer);

  auto& [SFXEffectPlugins, SFXBuffer] = getSFX();
  RenderEngine engineSFX(SAMPLE_RATE, BLOCK_SIZE);
  engineSFX.setBPM(BPM);
  loadGraph(engineSFX, SFXEffectPlugins, SFXBuffer);

  auto& [SFX2EffectPlugins, SFX2Buffer] = getSFX2();
  RenderEngine engineSFX2(SAMPLE_RATE, BLOCK_SIZE);
  engineSFX2.setBPM(BPM);
  loadGraph(engineSFX2, SFX2EffectPlugins, SFX2Buffer);

  engineDrums.render(150., false);
  engineBass.render(150., false);
  engineMelody.render(150., false);
  engineSFX.render(150., false);
  engineSFX2.render(150., false);

  auto drums = engineDrums.getAudioFrames();
  auto bass = engineBass.getAudioFrames();
  auto melody = engineMelody.getAudioFrames();
  auto SFX = engineSFX.getAudioFrames();
  auto SFX2 = engineSFX2.getAudioFrames();

  saveToFile(drums, "C:/Users/psusk/Downloads/drums.wav");
  saveToFile(bass, "C:/Users/psusk/Downloads/bass.wav");
  saveToFile(melody, "C:/Users/psusk/Downloads/melody.wav");
  saveToFile(SFX, "C:/Users/psusk/Downloads/SFX.wav");
  saveToFile(SFX2, "C:/Users/psusk/Downloads/SFX2.wav");

  int numChannels = drums.getNumChannels();
  int numSamples = drums.getNumSamples();

  juce::AudioSampleBuffer mixdown(numChannels, numSamples);
  mixdown.clear();

  for (int channel = 0; channel < numChannels; channel++) {
    mixdown.addFrom(channel, 0, drums.getReadPointer(channel), numSamples);
    mixdown.addFrom(channel, 0, bass.getReadPointer(channel), numSamples);
    mixdown.addFrom(channel, 0, melody.getReadPointer(channel), numSamples);
    mixdown.addFrom(channel, 0, SFX.getReadPointer(channel), numSamples);
    mixdown.addFrom(channel, 0, SFX2.getReadPointer(channel), numSamples);
  }

  saveToFile(mixdown, "C:/Users/psusk/Downloads/mixdown.wav");

  std::vector<std::pair<std::string, PluginData>> masterEffectPlugins;

  PluginData masterEffect1PluginData;
  masterEffect1PluginData.state = LimiterStateDir + "Limit-0dB.json";
  masterEffectPlugins.push_back(std::make_pair(Limiter, masterEffect1PluginData));

  RenderEngine engineMaster(SAMPLE_RATE, BLOCK_SIZE);
  engineMaster.setBPM(BPM);
  loadGraph(engineMaster, masterEffectPlugins, mixdown);

  saveToFile(mixdown, "C:/Users/psusk/Downloads/master.wav");
}

int main() {
  //Py_SetPythonHome(L"C:/Users/psusk/AppData/Local/Programs/Python/Python39");
  //Py_Initialize();

  std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
  //nonmultithread();
  std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

  std::cout << std::endl << "Non-Multithread elapsed time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() / 1000. << std::endl << std::endl;

  begin = std::chrono::steady_clock::now();
  multithread();
  end = std::chrono::steady_clock::now();

  std::cout << std::endl << "Multithread elapsed time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() / 1000. << std::endl;

  return 0;
}