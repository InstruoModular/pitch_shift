#pragma once
// Job manifest shared by vsthost and shiftbench.
// One job per line: <in.wav>\t<out.wav>\t<name=value;name=value>   ('#' lines ignored)
// value: a number = normalised [0,1] for VST params / raw value for shiftbench;
//        "@text"  = VST param set from its display text (getValueForText).
#include <cstdio>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

namespace manifest
{
    struct Job
    {
        std::string in, out;
        std::vector<std::pair<std::string, std::string>> params;
    };

    inline std::vector<std::string> split(const std::string& s, char sep)
    {
        std::vector<std::string> parts;
        std::string cur;
        for(char c : s)
        {
            if(c == sep) { parts.push_back(cur); cur.clear(); }
            else if(c != '\r') cur += c;
        }
        parts.push_back(cur);
        return parts;
    }

    inline std::vector<Job> load(const std::string& path)
    {
        std::vector<Job> jobs;
        std::ifstream f(path);
        std::string line;
        while(std::getline(f, line))
        {
            if(line.empty() || line[0] == '#') continue;
            auto cols = split(line, '\t');
            if(cols.size() < 2) continue;
            Job j{cols[0], cols[1], {}};
            if(cols.size() > 2 && !cols[2].empty())
            {
                for(auto& kv : split(cols[2], ';'))
                {
                    auto eq = kv.find('=');
                    if(eq != std::string::npos) j.params.emplace_back(kv.substr(0, eq), kv.substr(eq + 1));
                }
            }
            jobs.push_back(std::move(j));
        }
        return jobs;
    }

    /** Minimal JSON string escaping for the tools' hand-written JSON output. */
    inline std::string json_str(const std::string& s)
    {
        std::string o = "\"";
        for(unsigned char c : s)
        {
            if(c == '"' || c == '\\') { o += '\\'; o += static_cast<char>(c); }
            else if(c < 0x20) { char b[8]; std::snprintf(b, sizeof b, "\\u%04x", c); o += b; }
            else o += static_cast<char>(c);
        }
        return o + "\"";
    }
}
