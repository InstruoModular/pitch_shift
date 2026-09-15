// Headless VST3 host for offline, deterministic measurement runs.
//
//   vsthost info <plugin.vst3> [--sr 48000] [--block 32]           -> JSON to stdout
//   vsthost run  <plugin.vst3> --manifest jobs.tsv [--sr] [--block] [--tail-ms 1000]
//
// Each job: reset, set params, prepare, feed the mono input (duplicated to every input
// channel) in fixed blocks, append reported-latency + tail of silence, write channel 0
// as float32 WAV, plus <out>.json with the reported latency and the applied param texts.
#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#include "manifest.hpp"
#include "wav.hpp"

#include <iostream>

namespace
{
    struct Args
    {
        juce::String cmd, plugin, manifest_path;
        double sr{48000.0};
        int    block{32};
        double tail_ms{1000.0};
    };

    Args parse(int argc, char** argv)
    {
        Args a;
        if(argc > 1) a.cmd = argv[1];
        if(argc > 2) a.plugin = argv[2];
        for(int i = 3; i + 1 < argc; i += 2)
        {
            const juce::String k = argv[i], v = argv[i + 1];
            if(k == "--sr")            a.sr = v.getDoubleValue();
            else if(k == "--block")    a.block = v.getIntValue();
            else if(k == "--manifest") a.manifest_path = v;
            else if(k == "--tail-ms")  a.tail_ms = v.getDoubleValue();
        }
        return a;
    }

    std::unique_ptr<juce::AudioPluginInstance> load(const Args& a)
    {
        juce::VST3PluginFormatHeadless format;
        juce::OwnedArray<juce::PluginDescription> types;
        const auto path = juce::File::getCurrentWorkingDirectory().getChildFile(a.plugin).getFullPathName();
        format.findAllTypesForFile(types, path);
        if(types.isEmpty())
        {
            std::cerr << "no plugin types found in " << path << "\n";
            return nullptr;
        }
        juce::String err;
        auto inst = format.createInstanceFromDescription(*types[0], a.sr, a.block, err);
        if(!inst) std::cerr << "createInstance failed: " << err << "\n";
        return inst;
    }

    /** Enable one mono (or failing that, stereo) main in/out bus where possible. */
    void configure_buses(juce::AudioPluginInstance& p)
    {
        for(auto layout : { juce::AudioChannelSet::mono(), juce::AudioChannelSet::stereo() })
        {
            auto buses = p.getBusesLayout();
            if(buses.inputBuses.size() > 0)  buses.inputBuses.getReference(0)  = layout;
            if(buses.outputBuses.size() > 0) buses.outputBuses.getReference(0) = layout;
            if(p.setBusesLayout(buses)) return;
        }
    }

    juce::AudioProcessorParameter* find_param(juce::AudioPluginInstance& p, const juce::String& name)
    {
        for(auto* prm : p.getParameters())
            if(prm->getName(128).equalsIgnoreCase(name)) return prm;
        return nullptr;
    }

    int cmd_info(juce::AudioPluginInstance& p, const Args& a)
    {
        configure_buses(p);
        p.setRateAndBufferSizeDetails(a.sr, a.block);
        p.prepareToPlay(a.sr, a.block);

        std::string o = "{\n";
        o += "  \"name\": " + manifest::json_str(p.getName().toStdString()) + ",\n";
        o += "  \"inputs\": " + std::to_string(p.getTotalNumInputChannels())
           + ", \"outputs\": " + std::to_string(p.getTotalNumOutputChannels()) + ",\n";
        o += "  \"latency_samples\": " + std::to_string(p.getLatencySamples()) + ",\n";
        o += "  \"tail_seconds\": " + std::to_string(p.getTailLengthSeconds()) + ",\n";
        o += "  \"params\": [\n";

        const auto& params = p.getParameters();
        for(int i = 0; i < params.size(); i++)
        {
            auto* prm = params[i];
            const int steps = prm->getNumSteps();
            const bool enumerate = prm->isDiscrete() || steps <= 128;
            const int n = enumerate ? juce::jlimit(2, 128, steps) : 101;

            o += "    {\"index\": " + std::to_string(i)
               + ", \"name\": " + manifest::json_str(prm->getName(128).toStdString())
               + ", \"label\": " + manifest::json_str(prm->getLabel().toStdString())
               + ", \"default\": " + std::to_string(prm->getDefaultValue())
               + ", \"steps\": " + std::to_string(steps)
               + ", \"discrete\": " + (prm->isDiscrete() ? "true" : "false")
               + ", \"automatable\": " + (prm->isAutomatable() ? "true" : "false")
               + ", \"values\": [";
            for(int k = 0; k < n; k++)
            {
                const float v = static_cast<float>(k) / static_cast<float>(n - 1);
                o += (k ? ", " : "");
                o += "[" + std::to_string(v) + ", " + manifest::json_str(prm->getText(v, 64).toStdString()) + "]";
            }
            o += "]}";
            o += (i + 1 < params.size()) ? ",\n" : "\n";
        }
        o += "  ]\n}\n";
        std::cout << o;
        p.releaseResources();
        return 0;
    }

    int cmd_run(juce::AudioPluginInstance& p, const Args& a)
    {
        const auto jobs = manifest::load(a.manifest_path.toStdString());
        if(jobs.empty()) { std::cerr << "empty manifest\n"; return 2; }

        configure_buses(p);
        const int in_ch  = p.getTotalNumInputChannels();
        const int out_ch = p.getTotalNumOutputChannels();
        const int nch    = juce::jmax(1, in_ch, out_ch);
        juce::AudioBuffer<float> buf(nch, a.block);
        juce::MidiBuffer midi;
        int failures = 0;

        for(const auto& job : jobs)
        {
            wav::Mono in;
            std::string err;
            if(!wav::read(job.in, in, err)) { std::cerr << err << "\n"; failures++; continue; }

            std::string applied;
            bool ok = true;
            for(const auto& [name, value] : job.params)
            {
                auto* prm = find_param(p, name);
                if(!prm) { std::cerr << "unknown param '" << name << "'\n"; ok = false; break; }
                const juce::String v(value);
                const float norm = v.startsWithChar('@') ? prm->getValueForText(v.substring(1))
                                                         : v.getFloatValue();
                prm->setValueNotifyingHost(juce::jlimit(0.f, 1.f, norm));
                applied += (applied.empty() ? "" : ", ")
                         + manifest::json_str(name) + ": " + manifest::json_str(prm->getCurrentValueAsText().toStdString());
            }
            if(!ok) { failures++; continue; }

            // Prepare after params so any latency change they cause is reported.
            p.setNonRealtime(false);
            p.setRateAndBufferSizeDetails(a.sr, a.block);
            p.prepareToPlay(a.sr, a.block);
            p.reset();
            const int latency = p.getLatencySamples();

            const size_t tail  = static_cast<size_t>(latency) + static_cast<size_t>(a.tail_ms * a.sr / 1000.0);
            const size_t total = in.samples.size() + tail;
            std::vector<float> out;
            out.reserve(total + static_cast<size_t>(a.block));

            for(size_t pos = 0; pos < total; pos += static_cast<size_t>(a.block))
            {
                buf.clear();
                for(int i = 0; i < a.block; i++)
                {
                    const size_t s = pos + static_cast<size_t>(i);
                    const float x = s < in.samples.size() ? in.samples[s] : 0.f;
                    for(int c = 0; c < juce::jmax(1, in_ch); c++) buf.setSample(c, i, x);
                }
                p.processBlock(buf, midi);
                midi.clear();
                const auto* y = buf.getReadPointer(0);
                out.insert(out.end(), y, y + a.block);
            }
            out.resize(total);
            p.releaseResources();

            if(!wav::write(job.out, out, static_cast<uint32_t>(a.sr))) { std::cerr << "cannot write " << job.out << "\n"; failures++; continue; }

            std::string meta = "{\"reported_latency\": " + std::to_string(latency)
                             + ", \"block\": " + std::to_string(a.block)
                             + ", \"sr\": " + std::to_string(static_cast<int>(a.sr))
                             + ", \"params\": {" + applied + "}}\n";
            if(FILE* f = std::fopen((job.out + ".json").c_str(), "wb")) { std::fputs(meta.c_str(), f); std::fclose(f); }
        }

        std::cerr << "vsthost: " << (jobs.size() - static_cast<size_t>(failures)) << "/" << jobs.size() << " jobs ok\n";
        return failures ? 1 : 0;
    }
}

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juce_init;   // message manager for VST3 instantiation
    const Args a = parse(argc, argv);
    if((a.cmd != "info" && a.cmd != "run") || a.plugin.isEmpty())
    {
        std::cerr << "usage: vsthost info|run <plugin.vst3> [--manifest jobs.tsv] [--sr 48000] [--block 32] [--tail-ms 1000]\n";
        return 2;
    }
    auto plugin = load(a);
    if(!plugin) return 3;
    const int rc = (a.cmd == "info") ? cmd_info(*plugin, a) : cmd_run(*plugin, a);
    plugin.reset();
    return rc;
}
