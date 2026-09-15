// Shift Listen: the firmware pitch shifter (repo-root shift.hpp/shift.cpp) as a one-knob plugin for
// listening tests in a DAW.
//
// - One parameter: Transpose, -12..+12 semitones (integer, like the firmware control).
// - The DSP is the GCC-built shift_capi.dll (see shift_capi.h), loaded from the plugin's own folder, so the plugin
//   runs byte-for-byte the code the harness measured.
// - Shift runs at the firmware block size and sample rate. Host buffers of any size go through a sample FIFO, which
//   adds exactly one firmware block of delay; that is reported to the host for plugin delay compensation.
// - Input is summed to mono (a guitar DI is mono); output goes to every output channel.
// - If the DLL is missing or the host rate isn't the firmware rate, audio passes through (still FIFO-delayed) and
//   the editor says why.
#include <JuceHeader.h>

#include "shift_capi.h"

#include <memory>

namespace
{
    const juce::String transpose_id = "transpose";

    /** shift_capi.dll, resolved next to this plugin binary. */
    struct ShiftLib
    {
        juce::DynamicLibrary lib;
        int          (*block_size)()                                  = nullptr;
        double       (*sample_rate)()                                 = nullptr;
        ShiftHandle* (*create)()                                      = nullptr;
        void         (*destroy)(ShiftHandle*)                         = nullptr;
        void         (*set_semitones)(ShiftHandle*, int)              = nullptr;
        void         (*process)(ShiftHandle*, const float*, float*)   = nullptr;
        juce::String error;

        bool load()
        {
            const auto dll = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getSiblingFile("shift_capi.dll");
            if(!dll.existsAsFile()) { error = "shift_capi.dll not found next to the plugin"; return false; }
            if(!lib.open(dll.getFullPathName())) { error = "shift_capi.dll failed to load"; return false; }

            auto fn = [this](const char* name) { return lib.getFunction(name); };
            const auto version = static_cast<int (*)()>(fn("shift_capi_version"));
            if(!version || version() != 1) { error = "shift_capi.dll version mismatch"; return false; }
            block_size    = static_cast<int (*)()>(fn("shift_block_size"));
            sample_rate   = static_cast<double (*)()>(fn("shift_sample_rate"));
            create        = static_cast<ShiftHandle* (*)()>(fn("shift_create"));
            destroy       = static_cast<void (*)(ShiftHandle*)>(fn("shift_destroy"));
            set_semitones = static_cast<void (*)(ShiftHandle*, int)>(fn("shift_set_semitones"));
            process       = static_cast<void (*)(ShiftHandle*, const float*, float*)>(fn("shift_process"));
            if(!(block_size && sample_rate && create && destroy && set_semitones && process))
            {
                error = "shift_capi.dll is missing functions";
                return false;
            }
            return true;
        }
    };
}

class ShiftListenProcessor : public juce::AudioProcessor
{
    public:
        ShiftListenProcessor()
        : AudioProcessor(BusesProperties()
                           .withInput("Input", juce::AudioChannelSet::stereo(), true)
                           .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
          params(*this, nullptr, "params",
                 { std::make_unique<juce::AudioParameterInt>(juce::ParameterID{transpose_id, 1}, "Transpose", -12, 12, 0,
                       juce::AudioParameterIntAttributes().withLabel("st")) })
        {
            transpose = params.getRawParameterValue(transpose_id);
            lib_ok = shift.load();
            if(lib_ok) block = shift.block_size();
        }

        ~ShiftListenProcessor() override { release_handle(); }

        const juce::String getName() const override { return "Shift Listen"; }
        bool acceptsMidi() const override { return false; }
        bool producesMidi() const override { return false; }
        double getTailLengthSeconds() const override { return 0.1; }
        int getNumPrograms() override { return 1; }
        int getCurrentProgram() override { return 0; }
        void setCurrentProgram(int) override {}
        const juce::String getProgramName(int) override { return {}; }
        void changeProgramName(int, const juce::String&) override {}
        bool hasEditor() const override { return true; }
        juce::AudioProcessorEditor* createEditor() override;

        bool isBusesLayoutSupported(const BusesLayout& layouts) const override
        {
            const auto ok = [](const juce::AudioChannelSet& s) {
                return s == juce::AudioChannelSet::mono() || s == juce::AudioChannelSet::stereo();
            };
            return ok(layouts.getMainInputChannelSet()) && ok(layouts.getMainOutputChannelSet());
        }

        void prepareToPlay(double host_sample_rate, int) override
        {
            host_rate.store(host_sample_rate);
            release_handle();
            if(lib_ok)
            {
                handle = shift.create();
                rate_ok.store(std::abs(host_sample_rate - shift.sample_rate()) < 1.0);
            }
            in_block.assign(static_cast<size_t>(block), 0.f);
            out_block.assign(static_cast<size_t>(block), 0.f);
            fill = 0;
            setLatencySamples(block);
        }

        void releaseResources() override { release_handle(); }

        void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
        {
            juce::ScopedNoDenormals no_denormals;
            const int n_in  = getTotalNumInputChannels();
            const int n_out = getTotalNumOutputChannels();
            const int n     = buffer.getNumSamples();
            if(in_block.empty()) { buffer.clear(); return; }

            const bool run = handle != nullptr && rate_ok.load();
            const int semis = juce::roundToInt(transpose->load());
            const float in_gain = n_in > 1 ? 1.f / static_cast<float>(n_in) : 1.f;

            for(int i = 0; i < n; i++)
            {
                float x = 0.f;
                for(int c = 0; c < n_in; c++) x += buffer.getSample(c, i);
                x *= in_gain;

                const float y = out_block[static_cast<size_t>(fill)];   // one firmware block behind the input
                in_block[static_cast<size_t>(fill)] = x;
                for(int c = 0; c < n_out; c++) buffer.setSample(c, i, y);

                if(++fill == block)
                {
                    fill = 0;
                    if(run)
                    {
                        shift.set_semitones(handle, semis);
                        shift.process(handle, in_block.data(), out_block.data());
                    }
                    else
                    {
                        std::copy(in_block.begin(), in_block.end(), out_block.begin());   // delayed pass-through
                    }
                }
            }
        }

        void getStateInformation(juce::MemoryBlock& dest) override
        {
            if(auto xml = params.copyState().createXml()) copyXmlToBinary(*xml, dest);
        }

        void setStateInformation(const void* data, int size) override
        {
            if(auto xml = getXmlFromBinary(data, size))
                if(xml->hasTagName(params.state.getType())) params.replaceState(juce::ValueTree::fromXml(*xml));
        }

        juce::String status() const
        {
            if(!lib_ok) return "BYPASSED: " + shift.error;
            if(!rate_ok.load())
                return "BYPASSED: host at " + juce::String(host_rate.load(), 0) + " Hz - set project to "
                     + juce::String(shift.sample_rate(), 0) + " Hz";
            return "shift.hpp/cpp @ " + juce::String(shift.sample_rate() / 1000.0, 0) + " kHz, block " + juce::String(block);
        }

        bool running() const { return lib_ok && rate_ok.load(); }

        juce::AudioProcessorValueTreeState params;

    private:
        void release_handle()
        {
            if(handle) shift.destroy(handle);
            handle = nullptr;
        }

        ShiftLib             shift;
        bool                 lib_ok{false};
        ShiftHandle*         handle{nullptr};
        int                  block{32};
        std::atomic<bool>    rate_ok{false};
        std::atomic<double>  host_rate{0.0};
        std::atomic<float>*  transpose{nullptr};
        std::vector<float>   in_block, out_block;
        int                  fill{0};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ShiftListenProcessor)
};

class ShiftListenEditor : public juce::AudioProcessorEditor, private juce::Timer
{
    public:
        explicit ShiftListenEditor(ShiftListenProcessor& p)
        : AudioProcessorEditor(p), proc(p), attachment(p.params, transpose_id, knob)
        {
            knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 24);
            knob.setTextValueSuffix(" st");
            addAndMakeVisible(knob);

            title.setText("Transpose", juce::dontSendNotification);
            title.setJustificationType(juce::Justification::centred);
            title.setFont(juce::FontOptions(18.f, juce::Font::bold));
            addAndMakeVisible(title);

            status.setJustificationType(juce::Justification::centred);
            addAndMakeVisible(status);

            setSize(280, 300);
            startTimerHz(4);
            timerCallback();
        }

        void paint(juce::Graphics& g) override { g.fillAll(juce::Colour(0xff1e1f22)); }

        void resized() override
        {
            auto r = getLocalBounds().reduced(16);
            title.setBounds(r.removeFromTop(28));
            status.setBounds(r.removeFromBottom(40));
            knob.setBounds(r.reduced(8));
        }

    private:
        void timerCallback() override
        {
            status.setText(proc.status(), juce::dontSendNotification);
            status.setColour(juce::Label::textColourId, proc.running() ? juce::Colours::lightgrey : juce::Colours::orange);
        }

        ShiftListenProcessor& proc;
        juce::Slider knob;
        juce::Label title, status;
        juce::AudioProcessorValueTreeState::SliderAttachment attachment;
};

juce::AudioProcessorEditor* ShiftListenProcessor::createEditor() { return new ShiftListenEditor(*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ShiftListenProcessor(); }
