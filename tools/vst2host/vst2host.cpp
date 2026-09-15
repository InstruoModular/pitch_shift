// Minimal clean-room VST2 host for offline measurement (no Steinberg SDK): only the ABI the harness needs.
// Same CLI shape as vsthost:
//
//   vst2host info <plugin.dll> [--sr 48000] [--block 32] [--probe Name1,Name2] [--points 101] [--warmup-ms 1500]
//   vst2host run  <plugin.dll> --manifest jobs.tsv [--sr] [--block] [--tail-ms 1000] [--warmup-ms 1500]
//
// `info` lists every parameter (name, label, current value and its display text); only parameters named in
// --probe are swept through [0,1] for their display texts, because touching every parameter of a big plugin
// can load models/IRs. `run` duplicates the mono input to every input channel, writes output channel 0 as
// float32 WAV plus <out>.json with the plugin-reported latency (AEffect::initialDelay).
#include "manifest.hpp"
#include "wav.hpp"

#include <windows.h>
#include <objbase.h>   // CoInitializeEx: WIN32_LEAN_AND_MEAN leaves COM out of windows.h

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace vst2
{
    struct AEffect;
    using HostCallback = intptr_t (__cdecl*)(AEffect*, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt);

    // Field order and widths follow the public VST 2.4 ABI.
    struct AEffect
    {
        int32_t  magic;
        intptr_t (__cdecl* dispatcher)(AEffect*, int32_t, int32_t, intptr_t, void*, float);
        void     (__cdecl* process)(AEffect*, float**, float**, int32_t);
        void     (__cdecl* setParameter)(AEffect*, int32_t, float);
        float    (__cdecl* getParameter)(AEffect*, int32_t);
        int32_t  numPrograms;
        int32_t  numParams;
        int32_t  numInputs;
        int32_t  numOutputs;
        int32_t  flags;
        intptr_t resvd1;
        intptr_t resvd2;
        int32_t  initialDelay;
        int32_t  realQualities;
        int32_t  offQualities;
        float    ioRatio;
        void*    object;
        void*    user;
        int32_t  uniqueID;
        int32_t  version;
        void     (__cdecl* processReplacing)(AEffect*, float**, float**, int32_t);
        void     (__cdecl* processDoubleReplacing)(AEffect*, double**, double**, int32_t);
        char     future[56];
    };

    constexpr int32_t kEffectMagic         = 0x56737450;   // 'VstP'
    constexpr int32_t effFlagsCanReplacing = 1 << 4;

    enum : int32_t
    {
        effOpen = 0, effClose = 1, effGetParamLabel = 6, effGetParamDisplay = 7, effGetParamName = 8,
        effSetSampleRate = 10, effSetBlockSize = 11, effMainsChanged = 12, effGetEffectName = 45,
        effGetVendorString = 47, effGetProductString = 48, effGetVstVersion = 58, effStartProcess = 71,
        effStopProcess = 72, effSetProcessPrecision = 77
    };

    enum : int32_t
    {
        audioMasterAutomate = 0, audioMasterVersion = 1, audioMasterCurrentId = 2, audioMasterIdle = 3,
        audioMasterGetTime = 7, audioMasterIOChanged = 13, audioMasterSizeWindow = 15,
        audioMasterGetSampleRate = 16, audioMasterGetBlockSize = 17, audioMasterGetCurrentProcessLevel = 23,
        audioMasterGetAutomationState = 24, audioMasterGetVendorString = 32, audioMasterGetProductString = 33,
        audioMasterGetVendorVersion = 34, audioMasterCanDo = 37, audioMasterGetLanguage = 38,
        audioMasterUpdateDisplay = 42, audioMasterBeginEdit = 43, audioMasterEndEdit = 44
    };
}

namespace
{
    double g_sr    = 48000.0;
    int    g_block = 32;

    intptr_t __cdecl host_callback(vst2::AEffect*, int32_t opcode, int32_t, intptr_t, void* ptr, float)
    {
        switch(opcode)
        {
            case vst2::audioMasterVersion:                return 2400;
            case vst2::audioMasterGetSampleRate:          return static_cast<intptr_t>(g_sr);
            case vst2::audioMasterGetBlockSize:           return g_block;
            case vst2::audioMasterGetCurrentProcessLevel: return 2;   // realtime: behave as live
            case vst2::audioMasterIOChanged:              return 1;
            case vst2::audioMasterGetLanguage:            return 1;   // English
            case vst2::audioMasterGetVendorVersion:       return 1;
            case vst2::audioMasterGetVendorString:        if(ptr) std::strcpy(static_cast<char*>(ptr), "vst_analysis"); return 1;
            case vst2::audioMasterGetProductString:       if(ptr) std::strcpy(static_cast<char*>(ptr), "vst2host"); return 1;
            default:                                      return 0;   // no time info, no MIDI, no editor
        }
    }

    /** Let async plugin work (message-thread init, licence checks) run while we wait. */
    void pump_messages(int ms)
    {
        const auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
        MSG msg;
        do
        {
            while(PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
            Sleep(5);
        } while(std::chrono::steady_clock::now() < end);
    }

    std::wstring widen(const std::string& s)
    {
        const int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
        std::wstring w(static_cast<size_t>(n), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, w.data(), n);
        w.resize(static_cast<size_t>(n > 0 ? n - 1 : 0));
        return w;
    }

    struct Plugin
    {
        HMODULE        lib{nullptr};
        vst2::AEffect* fx{nullptr};

        intptr_t dispatch(int32_t op, int32_t index = 0, intptr_t value = 0, void* ptr = nullptr, float opt = 0.f) const
        {
            return fx->dispatcher(fx, op, index, value, ptr, opt);
        }

        std::string text(int32_t op, int32_t index = 0) const
        {
            char buf[512] = {};
            dispatch(op, index, 0, buf, 0.f);
            buf[sizeof(buf) - 1] = 0;
            return buf;
        }

        void resume() const  { dispatch(vst2::effMainsChanged, 0, 1); dispatch(vst2::effStartProcess); }
        void suspend() const { dispatch(vst2::effStopProcess); dispatch(vst2::effMainsChanged, 0, 0); }

        int32_t find_param(const std::string& name) const
        {
            for(int32_t i = 0; i < fx->numParams; i++)
                if(_stricmp(text(vst2::effGetParamName, i).c_str(), name.c_str()) == 0) return i;
            return -1;
        }
    };

    bool load(const std::string& path, Plugin& p)
    {
        wchar_t full[MAX_PATH];
        GetFullPathNameW(widen(path).c_str(), MAX_PATH, full, nullptr);
        std::wstring dir(full);
        dir = dir.substr(0, dir.find_last_of(L"\\/"));
        SetDllDirectoryW(dir.c_str());   // plugin-side dependencies next to the DLL

        p.lib = LoadLibraryW(full);
        if(!p.lib) { std::cerr << "LoadLibrary failed, error " << GetLastError() << "\n"; return false; }
        using Entry = vst2::AEffect* (__cdecl*)(vst2::HostCallback);
        auto entry = reinterpret_cast<Entry>(GetProcAddress(p.lib, "VSTPluginMain"));
        if(!entry) entry = reinterpret_cast<Entry>(GetProcAddress(p.lib, "main"));
        if(!entry) { std::cerr << "no VSTPluginMain export\n"; return false; }
        p.fx = entry(host_callback);
        if(!p.fx || p.fx->magic != vst2::kEffectMagic) { std::cerr << "plugin returned no valid AEffect\n"; return false; }
        if(!(p.fx->flags & vst2::effFlagsCanReplacing)) { std::cerr << "plugin lacks processReplacing\n"; return false; }
        p.dispatch(vst2::effOpen);
        return true;
    }

    void setup(const Plugin& p)
    {
        p.dispatch(vst2::effSetSampleRate, 0, 0, nullptr, static_cast<float>(g_sr));
        p.dispatch(vst2::effSetBlockSize, 0, g_block);
        p.dispatch(vst2::effSetProcessPrecision, 0, 0);   // 32-bit float
    }

    std::vector<std::string> split_csv(const std::string& s)
    {
        std::vector<std::string> v;
        for(auto& part : manifest::split(s, ',')) if(!part.empty()) v.push_back(part);
        return v;
    }

    int cmd_info(Plugin& p, const std::vector<std::string>& probe, int points)
    {
        setup(p);
        p.resume();
        std::string o = "{\n";
        o += "  \"name\": " + manifest::json_str(p.text(vst2::effGetEffectName)) + ",\n";
        o += "  \"vendor\": " + manifest::json_str(p.text(vst2::effGetVendorString)) + ",\n";
        o += "  \"product\": " + manifest::json_str(p.text(vst2::effGetProductString)) + ",\n";
        o += "  \"format\": \"vst2\", \"unique_id\": " + std::to_string(p.fx->uniqueID)
           + ", \"version\": " + std::to_string(p.fx->version) + ",\n";
        o += "  \"inputs\": " + std::to_string(p.fx->numInputs) + ", \"outputs\": " + std::to_string(p.fx->numOutputs)
           + ", \"programs\": " + std::to_string(p.fx->numPrograms) + ",\n";
        o += "  \"latency_samples\": " + std::to_string(p.fx->initialDelay) + ",\n  \"tail_seconds\": 0,\n";
        o += "  \"params\": [\n";
        for(int32_t i = 0; i < p.fx->numParams; i++)
        {
            const std::string name = p.text(vst2::effGetParamName, i);
            const float cur = p.fx->getParameter(p.fx, i);
            const bool do_probe = std::any_of(probe.begin(), probe.end(),
                                              [&](const std::string& n) { return _stricmp(n.c_str(), name.c_str()) == 0; });
            o += "    {\"index\": " + std::to_string(i) + ", \"name\": " + manifest::json_str(name)
               + ", \"label\": " + manifest::json_str(p.text(vst2::effGetParamLabel, i))
               + ", \"default\": " + std::to_string(cur) + ", \"steps\": 2147483647, \"discrete\": false"
               + ", \"automatable\": true, \"values\": [";
            if(do_probe)
            {
                for(int k = 0; k < points; k++)
                {
                    const float v = static_cast<float>(k) / static_cast<float>(points - 1);
                    p.fx->setParameter(p.fx, i, v);
                    o += (k ? ", " : "");
                    o += "[" + std::to_string(v) + ", " + manifest::json_str(p.text(vst2::effGetParamDisplay, i)) + "]";
                }
                p.fx->setParameter(p.fx, i, cur);
            }
            else
            {
                o += "[" + std::to_string(cur) + ", " + manifest::json_str(p.text(vst2::effGetParamDisplay, i)) + "]";
            }
            o += "]}";
            o += (i + 1 < p.fx->numParams) ? ",\n" : "\n";
        }
        o += "  ]\n}\n";
        std::cout << o;
        p.suspend();
        return 0;
    }

    /** Some plugins output digital silence for their first seconds after load (async licence/model init;
     *  Archetype: the first ~2 jobs). Feed low-level noise until the output has been non-silent for a
     *  sustained second, so measured jobs never start inside that window. */
    void prime(Plugin& p, int max_ms)
    {
        const int nin  = std::max(1, static_cast<int>(p.fx->numInputs));
        const int nout = std::max(1, static_cast<int>(p.fx->numOutputs));
        std::vector<std::vector<float>> ib(static_cast<size_t>(nin), std::vector<float>(static_cast<size_t>(g_block)));
        std::vector<std::vector<float>> ob(static_cast<size_t>(nout), std::vector<float>(static_cast<size_t>(g_block)));
        std::vector<float*> ins, outs;
        for(auto& b : ib) ins.push_back(b.data());
        for(auto& b : ob) outs.push_back(b.data());

        uint32_t seed = 12345u;
        const size_t max_samples = static_cast<size_t>(max_ms * g_sr / 1000.0);
        const size_t need_live   = static_cast<size_t>(g_sr);   // 1 s of continuously non-silent output
        size_t processed = 0, live = 0;
        const auto t0 = std::chrono::steady_clock::now();

        p.resume();
        while(processed < max_samples && live < need_live)
        {
            for(int i = 0; i < g_block; i++)
            {
                seed = seed * 1664525u + 1013904223u;
                const float x = 0.03f * (static_cast<float>(seed >> 8) / 8388608.f - 1.f);   // ~-36 dBFS noise
                for(auto& b : ib) b[static_cast<size_t>(i)] = x;
            }
            for(auto& b : ob) std::fill(b.begin(), b.end(), 0.f);
            p.fx->processReplacing(p.fx, ins.data(), outs.data(), g_block);
            float e = 0.f;
            for(float v : ob[0]) e += v * v;
            live = (e > 1e-12f) ? live + static_cast<size_t>(g_block) : 0u;
            processed += static_cast<size_t>(g_block);
            if((processed / static_cast<size_t>(g_block)) % 256 == 0) pump_messages(1);
        }
        p.suspend();
        const double wall = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        std::cerr << "vst2host: primed after " << (processed / g_sr) << " s of audio (" << wall << " s wall)"
                  << (live < need_live ? " -- WARNING: output still silent at limit" : "") << "\n";
    }

    int cmd_run(Plugin& p, const std::string& manifest_path, double tail_ms, int prime_max_ms)
    {
        const auto jobs = manifest::load(manifest_path);
        if(jobs.empty()) { std::cerr << "empty manifest\n"; return 2; }
        setup(p);
        std::string last_signature = "\x01";   // never equal to a real parameter set: first job always primes

        const int nin  = std::max(1, static_cast<int>(p.fx->numInputs));
        const int nout = std::max(1, static_cast<int>(p.fx->numOutputs));
        std::vector<std::vector<float>> in_buf(static_cast<size_t>(nin), std::vector<float>(static_cast<size_t>(g_block)));
        std::vector<std::vector<float>> out_buf(static_cast<size_t>(nout), std::vector<float>(static_cast<size_t>(g_block)));
        std::vector<float*> ins, outs;
        for(auto& b : in_buf) ins.push_back(b.data());
        for(auto& b : out_buf) outs.push_back(b.data());
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
                const int32_t idx = p.find_param(name);
                if(idx < 0) { std::cerr << "unknown param '" << name << "'\n"; ok = false; break; }
                if(!value.empty() && value[0] == '@') { std::cerr << "vst2host: '@text' values unsupported ('" << name << "')\n"; ok = false; break; }
                p.fx->setParameter(p.fx, idx, std::clamp(static_cast<float>(std::atof(value.c_str())), 0.f, 1.f));
                applied += (applied.empty() ? "" : ", ") + manifest::json_str(name) + ": "
                         + manifest::json_str(p.text(vst2::effGetParamDisplay, idx));
            }
            if(!ok) { failures++; continue; }

            // Switching sections/bypasses can mute a plugin for seconds (Archetype: the job right after the
            // isolation params were applied came out silent even after load-time priming). Re-prime
            // whenever the parameter set changes, so no measured job starts inside that mute.
            if(prime_max_ms > 0 && applied != last_signature)
            {
                prime(p, prime_max_ms);
                last_signature = applied;
            }

            // Suspend/resume around every job: JUCE-style plugins prepare+reset on resume, and any
            // latency change caused by the parameters is visible in initialDelay afterwards.
            p.suspend();
            p.resume();
            pump_messages(20);
            const int latency = p.fx->initialDelay;

            const size_t tail  = static_cast<size_t>(std::max(latency, 0)) + static_cast<size_t>(tail_ms * g_sr / 1000.0);
            const size_t total = in.samples.size() + tail;
            std::vector<float> out;
            out.reserve(total + static_cast<size_t>(g_block));
            for(size_t pos = 0; pos < total; pos += static_cast<size_t>(g_block))
            {
                for(int i = 0; i < g_block; i++)
                {
                    const size_t s = pos + static_cast<size_t>(i);
                    const float x = s < in.samples.size() ? in.samples[s] : 0.f;
                    for(auto& b : in_buf) b[static_cast<size_t>(i)] = x;
                }
                for(auto& b : out_buf) std::fill(b.begin(), b.end(), 0.f);
                p.fx->processReplacing(p.fx, ins.data(), outs.data(), g_block);
                out.insert(out.end(), out_buf[0].begin(), out_buf[0].end());
            }
            out.resize(total);
            p.suspend();

            if(!wav::write(job.out, out, static_cast<uint32_t>(g_sr))) { std::cerr << "cannot write " << job.out << "\n"; failures++; continue; }
            const std::string meta = "{\"reported_latency\": " + std::to_string(latency) + ", \"block\": " + std::to_string(g_block)
                                   + ", \"sr\": " + std::to_string(static_cast<int>(g_sr)) + ", \"params\": {" + applied + "}}\n";
            if(FILE* f = std::fopen((job.out + ".json").c_str(), "wb")) { std::fputs(meta.c_str(), f); std::fclose(f); }
        }
        std::cerr << "vst2host: " << (jobs.size() - static_cast<size_t>(failures)) << "/" << jobs.size() << " jobs ok\n";
        return failures ? 1 : 0;
    }
}

int main(int argc, char** argv)
{
    const std::string cmd    = argc > 1 ? argv[1] : "";
    const std::string plugin = argc > 2 ? argv[2] : "";
    std::string manifest_path;
    std::vector<std::string> probe;
    int points = 101, warmup_ms = 1500, prime_max_ms = 30000;
    double tail_ms = 1000.0;
    for(int i = 3; i + 1 < argc; i += 2)
    {
        const std::string k = argv[i], v = argv[i + 1];
        if(k == "--sr")             g_sr = std::atof(v.c_str());
        else if(k == "--block")     g_block = std::atoi(v.c_str());
        else if(k == "--manifest")  manifest_path = v;
        else if(k == "--tail-ms")   tail_ms = std::atof(v.c_str());
        else if(k == "--probe")     probe = split_csv(v);
        else if(k == "--points")    points = std::max(2, std::atoi(v.c_str()));
        else if(k == "--warmup-ms") warmup_ms = std::atoi(v.c_str());
        else if(k == "--prime-max-ms") prime_max_ms = std::atoi(v.c_str());   // 0 disables priming
    }
    if((cmd != "info" && cmd != "run") || plugin.empty())
    {
        std::cerr << "usage: vst2host info|run <plugin.dll> [--manifest jobs.tsv] [--probe A,B] [--sr] [--block] [--tail-ms] [--warmup-ms]\n";
        return 2;
    }

    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    Plugin p;
    if(!load(plugin, p)) return 3;
    pump_messages(warmup_ms);

    const int rc = (cmd == "info") ? cmd_info(p, probe, points) : cmd_run(p, manifest_path, tail_ms, prime_max_ms);

    p.dispatch(vst2::effClose);
    pump_messages(50);
    // FreeLibrary deliberately skipped: some plugins crash unloading background threads at exit.
    CoUninitialize();
    return rc;
}
