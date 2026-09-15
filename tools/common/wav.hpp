#pragma once
// Minimal mono WAV I/O shared by vsthost and shiftbench (no library deps).
// Reads PCM16/24/32 or float32 (first channel); writes float32 mono.
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace wav
{
    struct Mono
    {
        std::vector<float> samples;
        uint32_t           sample_rate{48000};
    };

    inline bool read(const std::string& path, Mono& out, std::string& err)
    {
        FILE* f = std::fopen(path.c_str(), "rb");
        if(!f) { err = "cannot open " + path; return false; }
        std::vector<uint8_t> d;
        std::fseek(f, 0, SEEK_END);
        d.resize(static_cast<size_t>(std::ftell(f)));
        std::fseek(f, 0, SEEK_SET);
        size_t got = std::fread(d.data(), 1, d.size(), f);
        std::fclose(f);
        if(got != d.size() || d.size() < 12 || std::memcmp(d.data(), "RIFF", 4) || std::memcmp(d.data() + 8, "WAVE", 4))
        { err = "not a RIFF/WAVE file: " + path; return false; }

        auto u16 = [&](size_t o) { return static_cast<uint32_t>(d[o] | (d[o + 1] << 8)); };
        auto u32 = [&](size_t o) { return u16(o) | (u16(o + 2) << 16); };

        uint16_t fmt = 0, ch = 0, bits = 0;
        size_t pos = 12;
        while(pos + 8 <= d.size())
        {
            const uint32_t len  = u32(pos + 4);
            const size_t   body = pos + 8;
            if(!std::memcmp(d.data() + pos, "fmt ", 4))
            {
                fmt  = static_cast<uint16_t>(u16(body));
                ch   = static_cast<uint16_t>(u16(body + 2));
                out.sample_rate = u32(body + 4);
                bits = static_cast<uint16_t>(u16(body + 14));
                if(fmt == 0xFFFE && len >= 26) fmt = static_cast<uint16_t>(u16(body + 24));   // extensible
            }
            else if(!std::memcmp(d.data() + pos, "data", 4))
            {
                if(!ch || !bits) { err = "data before fmt"; return false; }
                const size_t bps   = bits / 8u;
                const size_t avail = std::min<size_t>(len, d.size() - body);
                const size_t n     = avail / (bps * ch);
                out.samples.resize(n);
                for(size_t i = 0; i < n; i++)
                {
                    const size_t o = body + i * bps * ch;
                    float v = 0.f;
                    if(fmt == 3 && bits == 32)      std::memcpy(&v, d.data() + o, 4);
                    else if(fmt == 1 && bits == 16) v = static_cast<int16_t>(u16(o)) / 32768.f;
                    else if(fmt == 1 && bits == 24) v = static_cast<float>(static_cast<int32_t>((u32(o) & 0xFFFFFF) << 8) >> 8) / 8388608.f;
                    else if(fmt == 1 && bits == 32) v = static_cast<float>(static_cast<int32_t>(u32(o))) / 2147483648.f;
                    else { err = "unsupported wav format"; return false; }
                    out.samples[i] = v;
                }
                return true;
            }
            pos = body + len + (len & 1u);
        }
        err = "no data chunk";
        return false;
    }

    inline bool write(const std::string& path, const std::vector<float>& s, uint32_t sr)
    {
        FILE* f = std::fopen(path.c_str(), "wb");
        if(!f) return false;
        auto p32 = [&](uint32_t v) { std::fwrite(&v, 4, 1, f); };
        auto p16 = [&](uint16_t v) { std::fwrite(&v, 2, 1, f); };
        const uint32_t data_len = static_cast<uint32_t>(s.size() * 4u);
        std::fwrite("RIFF", 1, 4, f); p32(36u + data_len); std::fwrite("WAVE", 1, 4, f);
        std::fwrite("fmt ", 1, 4, f); p32(16); p16(3); p16(1); p32(sr); p32(sr * 4u); p16(4); p16(32);
        std::fwrite("data", 1, 4, f); p32(data_len);
        std::fwrite(s.data(), 4, s.size(), f);
        std::fclose(f);
        return true;
    }
}
