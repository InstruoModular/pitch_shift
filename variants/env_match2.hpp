#ifndef SHIFT_ENV_MATCH2_HPP
#define SHIFT_ENV_MATCH2_HPP

#include "constants.hpp"
#include "functions.hpp"
#include "idsp/filter.hpp"
#include "idsp/functions.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

/* Fixed-interval pitch shifter for a live monophonic instrument feed.
 *
 * The signal chain is the textbook decomposition of pitch shifting into the two
 * halves that fail for different reasons:
 *
 *   resampling  -- read the delay line at `ratio` samples per output sample.
 *                  This is the part that is exactly solvable: a windowed-sinc
 *                  polyphase interpolator makes it transparent, and pre-filtering
 *                  the write side at Nyquist/ratio makes the upshift alias-free.
 *   time repair -- the read head drifts against the write head, so periodically
 *                  splice it back. This is the part that makes the artefacts,
 *                  and it is the only part worth being clever about.
 *
 * The splice is pitch-synchronous: a YIN period tracker runs on a 4x decimated
 * copy of the line, the splice distance is snapped to an integer number of
 * periods, and a normalised cross-correlation refines it to a fraction of a
 * sample. Two heads exactly one period family apart are phase-coherent, so the
 * crossfade between them is amplitude-complementary rather than a comb filter.
 * That is what a phase vocoder spends 40 ms of latency to achieve, and it costs
 * about one pitch period here.
 *
 * Transients get their own path. A pick attack is precisely where a repeated
 * grain is audible as a flam, so an onset detector re-seats the read head at the
 * far end of its excursion, buying a full grain of splice-free runway across the
 * attack. The seek is masked by the attack itself.
 *
 * Latency is not constant, and deliberately so: it is roughly one pitch period
 * for the interval in use, so a low E an octave up costs ~15 ms while a shift of
 * a semitone on the high strings costs under 3 ms. Nothing here is block-based,
 * so there is no framing latency on top of that.
 */
namespace shift_env_match2
{
    /* Fractional-delay interpolator. 32 taps is enough for a stopband below the
     * noise floor of anything a guitar pickup produces; 128 phases with a linear
     * blend between adjacent kernels puts the phase-quantisation error near
     * -85 dB, which is cheaper than the 512 phases it would take to reach the
     * same place without the blend. */
    static constexpr size_t sinc_taps   = 32;
    static constexpr size_t sinc_centre = (sinc_taps / 2) - 1;   // 15
    static constexpr size_t sinc_phases = 128;

    using SincKernel = std::array<float, sinc_taps>;

    /* Blackman-windowed sinc, cut at Nyquist. Cutting at exactly 0.5 is what
     * makes phase 0 a single unity tap -- sinc() is zero at every other integer
     * -- so a shift of zero semitones, where the read head never accumulates a
     * fraction, is bit-transparent rather than "nearly" transparent. */
    inline const std::array<SincKernel, sinc_phases + 1> sinc_table = []()
    {
        std::array<SincKernel, sinc_phases + 1> table{};
        for(size_t p = 0; p <= sinc_phases; p++)
        {
            const float frac = static_cast<float>(p) / static_cast<float>(sinc_phases);
            float sum = 0.f;
            for(size_t j = 0; j < sinc_taps; j++)
            {
                const float x = static_cast<float>(j) - static_cast<float>(sinc_centre) - frac;
                const float t = x / static_cast<float>(sinc_taps / 2);   // -1 .. 1
                const float w = 0.42f + 0.5f * std::cos(idsp::pi * t)
                                      + 0.08f * std::cos(idsp::twopi * t);
                const float s = (std::fabs(x) < 1e-7f)
                              ? 1.f
                              : std::sin(idsp::pi * x) / (idsp::pi * x);
                table[p][j] = w * s;
                sum += table[p][j];
            }
            /* Normalise so DC gain is exactly one on every phase; without this
             * the window's truncation shows up as a few hundredths of a dB of
             * ripple that the read head sweeps through as it drifts. */
            const float norm = 1.f / sum;
            for(size_t j = 0; j < sinc_taps; j++) table[p][j] *= norm;
        }
        return table;
    }();
}

class Shift_env_match2
{
    public:
        Shift_env_match2() { reset(); }
        ~Shift_env_match2() = default;

        struct Controls
        {
            int16_t shift_amount{0};   // semitones, -12 .. +12
        } controls;

        void reset();

        void process(const MonoDspBuffer& input, MonoDspBuffer& output);

    private:
        /* 4096 samples covers the worst case: the lowest period this tracks
         * (~60 Hz, 800 samples) as a grain, plus the head excursion, plus the
         * correlation window that reaches past the head, plus kernel margin. */
        static constexpr size_t   history_size = 4096;
        static constexpr uint32_t history_mask = history_size - 1u;

        /* The period tracker and the coarse splice search both run on a 4x
         * decimated copy. 12 kHz still resolves every partial that carries
         * period information for a guitar, and it makes the O(lags x window)
         * searches sixteen times cheaper. */
        static constexpr size_t   dec_factor = 4;
        static constexpr size_t   dec_size   = 1024;
        static constexpr uint32_t dec_mask   = dec_size - 1u;

        /* YIN, in decimated samples: lag 8 is 1500 Hz, lag 300 is 40 Hz.
         *
         * The bottom of that range is well below any note a guitar plays, and
         * deliberately so. What the splicer needs is not the pitch a listener
         * would name but the period the *waveform* actually repeats at, and for
         * two strings a fifth apart that is the common period -- 41 Hz for a
         * low E power chord, two octaves under either note. Stopping at 60 Hz
         * is what makes a tracker unable to lock any chord at all. */
        static constexpr size_t yin_window   = 384;
        static constexpr size_t yin_min_lag  = 8;
        static constexpr size_t yin_max_lag  = 300;
        static constexpr size_t yin_analysis = yin_window + yin_max_lag;
        static constexpr size_t yin_per_block = 2;      // lags retired per block
        static constexpr float  yin_threshold = 0.30f;  // d'(tau) below this is a pitch
        static constexpr uint32_t yin_hold_frames = 6;  // bad frames tolerated

        static constexpr float min_period = 32.f;   // E1: was 60, below which YIN (32 smp) still tracks
        static constexpr float max_period = static_cast<float>(yin_max_lag * 4);

        /* The similarity window is centred on the head, so half of it is also
         * the latency floor -- every sample of reach past the head is a sample
         * the output has to wait for. 256 is the shortest that still spans a
         * useful stretch of waveform at the bottom of the range. */
        static constexpr float max_corr_window = 256.f;

        /* The un-tracked path instead needs a window longer than a period of
         * the lowest thing it might see, and a grain long enough that a search
         * around it can reach the common period of a chord. */
        static constexpr float fallback_corr_window = 512.f;

        static constexpr float guard_samples = 40.f;    // kernel reach + slack
        static constexpr float min_grain     = 192.f;   // floor on splice spacing
        static constexpr float default_grain = 1536.f; /* E3b: was 768 */   // when no pitch is found
        static constexpr float    onset_grain      = 768.f;   // E5: blind grain for the first 200 ms after an onset
        static constexpr uint32_t onset_grain_hold = 9600u;

        /* Full-rate refinement half-width; must cover half the coarse grid step,
         * which is 4 input samples when tracking and 32 when searching blind. */
        static constexpr int32_t max_fine_reach = 16;

        /* The fine pass only resolves the half grid step the coarse pass could
         * not, so its window is set by how far a correlation peak has to be
         * localised, not by how much waveform tells one alignment from another --
         * that argument is the coarse pass's, and it has already been won by the
         * time this runs. 64 samples is 1.3 ms, ample to place a peak to a
         * fraction of a sample from within +-16 of it, and it is the difference
         * between a splice block that fits the audio budget and one that does
         * not. */
        static constexpr float fine_corr_window = 64.f;

        struct Head
        {
            uint32_t pos{0};
            float    frac{0.f};
        };

        float _read(const Head& head) const;
        void  _write(float x);
        void  _seek(Head& head, float lag);
        void  _update_interval();
        void  _update_grain();
        void  _pitch_tick();
        void  _yin_finalise();
        int32_t _splice_coarse(uint32_t ref, int32_t sign);
        float   _splice_fine(uint32_t ref, int32_t sign, int32_t best_dd) const;
        void    _start_fade(float target_lag, float length, bool match_level = false);

        /* ratio is at most 2, so the carry never runs more than twice. */
        inline void _advance(Head& h) const
        {
            h.frac += ratio;
            while(h.frac >= 1.f)
            {
                h.frac -= 1.f;
                h.pos++;
            }
        }

        // ---- line -------------------------------------------------------
        std::array<float, history_size> history{};
        std::array<float, dec_size>     dec_history{};
        uint32_t w_abs{0};              // total samples written
        uint32_t dec_abs{0};            // total decimated samples written
        uint32_t dec_phase{0};

        // ---- read heads --------------------------------------------------
        std::array<Head, 2> head{};
        std::array<float, 2> head_gain{1.f, 1.f};          // E6: per-head level match
        static constexpr float   gain_relax  = 1.f / 2400.f; // E6: ~50 ms back to unity
        static constexpr int32_t gain_window = 128;
        size_t   active{0};             // head that survives the current fade
        bool     fading{false};
        float    fade_pos{0.f};
        float    fade_step{0.f};

        /* The two halves of the splice search, run a block apart.
         *
         * Together they are by some margin the most expensive thing the shifter
         * does, and landing both in one block puts that block over its period --
         * which does not glitch quietly. A missed block is a block of input that
         * never reaches the line while the read head keeps advancing, so the
         * delay steps and the pitch with it; the symptom is warble, not a click.
         *
         * So the coarse pass is armed a block before the splice is due and its
         * answer held here. Holding a *distance* is what makes that safe: both
         * ends of it travel with the head, so it stays the distance the search
         * chose no matter where the head has advanced to by the time the splice
         * fires, and the fine pass re-hunts the peak around it either way. */
        bool     splice_armed{false};
        int32_t  splice_dd{0};

        // ---- interval ----------------------------------------------------
        int16_t  interval{0};
        float    ratio{1.f};
        float    grain{default_grain};  // splice distance, input samples
        float    xfade{256.f};          // crossfade length, output samples
        float    corr_window{384.f};    // correlation window, input samples
        /* The hard floor on the lag: below this the head is closer to the write
         * head than the similarity search reaches past it, and the search
         * correlates against samples the line has not written yet -- which on a
         * ring means a lap-old copy of the signal, so it scores as real and
         * picks a real-looking splice out of the wrong audio. lag_lo is what
         * the scheduler aims for and can carry extra on top; this is what
         * nothing may go below. */
        float    lag_floor{0.f};
        float    lag_lo{0.f};
        float    lag_hi{0.f};

        // ---- pitch -------------------------------------------------------
        std::array<float, yin_analysis>    yin_snap{};
        std::array<float, yin_max_lag + 1> yin_d{};
        size_t   yin_cursor{0};
        bool     yin_running{false};
        float    period{default_grain};
        bool     period_valid{false};
        uint32_t period_hold{0};

        // ---- onset -------------------------------------------------------
        float    onset_lp{0.f};
        float    onset_fast{0.f};
        float    onset_cur{0.f};        // peak so far in this period frame
        float    onset_prev{0.f};       // peak of the previous period frame
        uint32_t onset_frame{0};
        uint32_t onset_frame_len{512};
        uint32_t onset_hold{0};
        uint32_t onset_age{onset_grain_hold};   // E5: samples since the last onset

        // ---- filters -----------------------------------------------------
        idsp::BiquadFilter<idsp::BiquadType::Highpass>              dc_block;
        std::array<idsp::BiquadFilter<idsp::BiquadType::Lowpass>, 4> anti_alias;
        bool                                                        anti_alias_on{false};
        std::array<idsp::BiquadFilter<idsp::BiquadType::Lowpass>, 2> dec_filter;
};

#endif // SHIFT_ENV_MATCH2_HPP
