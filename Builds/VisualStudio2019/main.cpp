#include "../../../Source/RenderEngine.h"
#include "../../../Source/PlaybackProcessor.h"
#include "../../../Source/PluginProcessor.h"

#include <cstdio>

#include <chrono>

#define SAMPLE_RATE 44100
#define BLOCK_SIZE 512

#define BPM 160

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

struct PluginData {
  std::string preset = "";
  std::string state = "";
  std::vector<std::pair<int, float>> parameters;
  std::vector<std::pair<int, pybind11::array>> automation;
};

void render(RenderEngine& engine, std::pair<std::string, PluginData> synthPlugin, std::vector<std::pair<std::string, PluginData>> effectPlugins, std::string midi) {
  DAG graph;

  auto& [synthPluginPath, synthPluginData] = synthPlugin;

  auto synth = engine.makePluginProcessor("synth", synthPluginPath);

  if (synthPluginData.preset != "") {
    synth->loadPreset(synthPluginData.preset);
  }
  else if (synthPluginData.state != "") {
    synth->loadStateInformation(synthPluginData.state);
  }
  if (!synthPluginData.parameters.empty()) {
    for (const auto& pair : synthPluginData.parameters) {
      synth->setParameter(pair.first, pair.second);
    }
  }
  if (!synthPluginData.automation.empty()) {
    for (const auto& pair : synthPluginData.automation) {
      int index = pair.first;
      synth->setAutomationByIndex(index, pair.second, 0);
    }
  }

  synth->loadMidi(midi, true, false, true);

  graph.nodes.push_back({ synth, {} });

  size_t i = 0;
  std::string previousNode = "synth";
  for (const auto& pluginPair : effectPlugins) {
    auto& [pluginPath, pluginData] = pluginPair;

    auto effect = engine.makePluginProcessor("effect" + std::to_string(i), pluginPath);

    if (pluginData.preset != "") {
      effect->loadPreset(pluginData.preset);
    }
    else if (pluginData.state != "") {
      effect->loadStateInformation(pluginData.state);
    }
    if (!pluginData.parameters.empty()) {
      for (const auto& pair : pluginData.parameters) {
        effect->setParameter(pair.first, pair.second);
      }
    }
    if (!pluginData.automation.empty()) {
      for (const auto& pair : pluginData.automation) {
        int index = pair.first;
        effect->setAutomationByIndex(index, pair.second, 0);
      }
    }

    graph.nodes.push_back({ effect, {previousNode} });

    i++;
    //previousNode = "effect" + std::to_string(i);
  }


  engine.loadGraph(graph);

  engine.render(180, false);

  auto audio = engine.getAudioFrames();

  std::string outputPath = "C:/Users/psusk/Downloads/out.wav";
  std::filesystem::remove(outputPath);
  juce::File outputFile(outputPath);
  auto outStream = outputFile.createOutputStream();
  AudioFormatManager formatManager;
  formatManager.registerBasicFormats();
  std::unique_ptr<juce::AudioFormatWriter> writer(formatManager.findFormatForFileExtension("wav")->createWriterFor(outStream.get(), SAMPLE_RATE, audio.getNumChannels(), 32, juce::StringPairArray(), 0));
  writer->writeFromAudioSampleBuffer(audio, 0, audio.getNumSamples());
  outStream.release();
}

int main() {
  juce::MessageManager::getInstance();

  Py_SetPythonHome(L"C:/Users/psusk/AppData/Local/Programs/Python/Python39");
  Py_Initialize();

  RenderEngine engine(SAMPLE_RATE, BLOCK_SIZE);
  engine.setBPM(BPM);

  PluginData synthPluginData;
  synthPluginData.state = "C:/Users/psusk/source/repos/Python/vize/states/Sitala/Drum Kit - Trap 001.json";
  std::pair<std::string, PluginData> synth = std::make_pair(Sitala, synthPluginData);

  PluginData effect1PluginData;
  effect1PluginData.state = "C:/Users/psusk/source/repos/Python/vize/states/VOS/LowCut40Hz.json";
  std::pair<std::string, PluginData> effect1 = std::make_pair(VOS, effect1PluginData);

  PluginData effect2PluginData;
  effect2PluginData.state = "C:/Users/psusk/source/repos/Python/vize/states/Limiter/Limit-0dB.json";
  std::pair<std::string, PluginData> effect2 = std::make_pair(VOS, effect2PluginData);

  PluginData effect3PluginData;
  effect3PluginData.state = "C:/Users/psusk/source/repos/Python/vize/states/VOS/LowCut40Hz.json";
  std::pair<std::string, PluginData> effect3 = std::make_pair(VOS, effect3PluginData);

  PluginData effect4PluginData;
  effect4PluginData.state = "C:/Users/psusk/source/repos/Python/vize/states/Limiter/Limit-6dBDrums.json";
  std::pair<std::string, PluginData> effect4 = std::make_pair(VOS, effect4PluginData);

  //std::string midi = "C:/Users/psusk/source/repos/Python/vize/MIDIs/THH - 160 BPM - 4 Bars - Drum Pattern 1 - Intro 1a.mid";
  std::string midi = "C:/Users/psusk/source/repos/Python/vize/Int/stem-drums.mid";

  std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
  render(engine, synth, {effect1, effect2, effect3, effect4}, midi);
  std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

  std::cout << "Elapsed time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() / 1000. << std::endl;

  return 0;
}