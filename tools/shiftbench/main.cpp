// Offline harness for firmware pitch shifters; same CLI shape as vsthost.
//
//   shiftbench info
//   shiftbench run <variant> --manifest jobs.tsv [--tail-ms 1000]
//
// Manifest params: shift=<semitones>; anything else goes to the variant's set_param().
// Block size and sample rate are the firmware's (constants.hpp), not negotiable here.
// Each job writes <out>.json with cpu_ns_per_sample and cpu_worst_block_pct.
#include "variant.hpp"
#include "manifest.hpp"
#include "wav.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>

int main(int argc, char** argv)
{
    const std::string cmd = argc > 1 ? argv[1] : "";
    if(cmd == "info")
    {
        std::string names;
        for(auto& [name, _] : shifter_registry()) names += (names.empty() ? "" : ", ") + manifest::json_str(name);
        std::printf("{\"name\": \"shiftbench\", \"block\": %zu, \"sr\": %d, \"latency_samples\": 0, \"variants\": [%s]}\n",
                    audio_block_size, static_cast<int>(sample_rate), names.c_str());
        return 0;
    }
    if(cmd != "run" || argc < 3)
    {
        std::cerr << "usage: shiftbench info | run <variant> --manifest jobs.tsv [--tail-ms 1000]\n";
        return 2;
    }

    const std::string variant = argv[2];
    std::string manifest_path;
    double tail_ms = 1000.0;
    for(int i = 3; i + 1 < argc; i += 2)
    {
        const std::string k = argv[i];
        if(k == "--manifest")     manifest_path = argv[i + 1];
        else if(k == "--tail-ms") tail_ms = std::atof(argv[i + 1]);
        // --block/--sr accepted and ignored: the firmware constants rule
    }
    auto it = shifter_registry().find(variant);
    if(it == shifter_registry().end()) { std::cerr << "unknown variant '" << variant << "'\n"; return 2; }

    const auto jobs = manifest::load(manifest_path);
    if(jobs.empty()) { std::cerr << "empty manifest\n"; return 2; }

    const size_t B = audio_block_size;
    const double block_period_ns = 1e9 * static_cast<double>(B) / static_cast<double>(sample_rate);
    int failures = 0;

    for(const auto& job : jobs)
    {
        wav::Mono in;
        std::string err;
        if(!wav::read(job.in, in, err)) { std::cerr << err << "\n"; failures++; continue; }
        if(in.sample_rate != static_cast<uint32_t>(sample_rate))
        { std::cerr << job.in << ": sample rate " << in.sample_rate << " != firmware rate\n"; failures++; continue; }

        auto shifter = it->second();
        std::string applied;
        bool ok = true;
        int shift = 0;
        for(const auto& [name, value] : job.params)
        {
            if(name == "shift") shift = std::atoi(value.c_str());
            else if(!shifter->set_param(name, std::atof(value.c_str())))
            { std::cerr << "variant '" << variant << "' has no param '" << name << "'\n"; ok = false; break; }
            applied += (applied.empty() ? "" : ", ") + manifest::json_str(name) + ": " + manifest::json_str(value);
        }
        if(!ok) { failures++; continue; }

        shifter->reset();
        shifter->set_shift(shift);

        const size_t total = in.samples.size() + static_cast<size_t>(tail_ms * sample_rate / 1000.0);
        std::vector<float> out;
        out.reserve(total + B);
        MonoDspBuffer bin{}, bout{};
        double ns_sum = 0.0, ns_worst = 0.0;
        size_t blocks = 0;

        for(size_t pos = 0; pos < total; pos += B)
        {
            for(size_t i = 0; i < B; i++)
            {
                const size_t s = pos + i;
                bin[i] = s < in.samples.size() ? in.samples[s] : 0.f;
            }
            const auto t0 = std::chrono::steady_clock::now();
            shifter->process(bin, bout);
            const double ns = static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                  std::chrono::steady_clock::now() - t0).count());
            ns_sum += ns;
            ns_worst = std::max(ns_worst, ns);
            blocks++;
            for(size_t i = 0; i < B; i++) out.push_back(bout[i]);
        }
        out.resize(total);

        if(!wav::write(job.out, out, static_cast<uint32_t>(sample_rate)))
        { std::cerr << "cannot write " << job.out << "\n"; failures++; continue; }

        char meta[512];
        std::snprintf(meta, sizeof meta,
                      "{\"reported_latency\": null, \"block\": %zu, \"sr\": %d, \"variant\": %s, "
                      "\"cpu_ns_per_sample\": %.2f, \"cpu_worst_block_pct\": %.3f, \"params\": {%s}}\n",
                      B, static_cast<int>(sample_rate), manifest::json_str(variant).c_str(),
                      ns_sum / static_cast<double>(blocks * B), 100.0 * ns_worst / block_period_ns, applied.c_str());
        if(FILE* f = std::fopen((job.out + ".json").c_str(), "wb")) { std::fputs(meta, f); std::fclose(f); }
    }

    std::cerr << "shiftbench: " << (jobs.size() - static_cast<size_t>(failures)) << "/" << jobs.size() << " jobs ok\n";
    return failures ? 1 : 0;
}
