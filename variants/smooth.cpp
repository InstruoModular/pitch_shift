#include "smooth.hpp"
#include "variant.hpp"   // lat: set_param specialisation below needs ShiftAdapter
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace
{
    /* Onset detector. The trigger is high-frequency energy, not level: a pick
     * attack is a wideband click on top of whatever the string was already
     * doing, so an envelope of the signal above ~800 Hz separates it from a
     * swell or a bend, both of which must not re-seat the read head.
     *
     * The reference it is compared against is the peak of the *previous pitch
     * period*, and that framing is the whole detector. A plucked string is a
     * periodic click train, so any envelope shorter than a period sees an
     * attack every period, and any fixed window long enough to fix that for a
     * low E is far too sluggish on the top strings. A window one period long
     * contains exactly one click wherever its edges happen to fall, so
     * successive frames of a note that is merely continuing have the same peak,
     * and only a genuinely new event clears the threshold. */
    static constexpr float onset_lp_coef   = 0.0999f;    // ~800 Hz crossover
    static constexpr float onset_fast_coef = 0.0408f;    // ~0.5 ms, de-spike only
    float onset_ratio  = 2.5f;    // smooth: runtime-tunable (set_param onset_ratio)
    bool  onset_reseat = true;    // smooth: runtime-tunable (set_param onset_reseat)

    /* smooth: debug event log, only when SHIFT_EVENTS names a file. Harness-only; never in firmware. */
    FILE* events_file()
    {
        static FILE* f = nullptr;
        static bool tried = false;
        if(!tried)
        {
            tried = true;
            if(const char* path = std::getenv("SHIFT_EVENTS")) f = std::fopen(path, "w");
        }
        return f;
    }
    static constexpr float onset_floor     = 1.0e-4f;
    static constexpr uint32_t onset_refractory = 1440u;  // 30 ms
    static constexpr float onset_fade      = 64.f;       // output samples
    // onset_runway moved into the class (runtime-tunable)

    /* 8th-order Butterworth, as four biquads. Pole Qs are 1/(2 cos(theta)) for
     * theta = 11.25, 33.75, 56.25, 78.75 degrees. The cutoff sits well below
     * Nyquist/ratio: at an octave up it lands near 10 kHz, and since the output
     * of that resample is band-limited to 20 kHz either way, the octave of
     * headroom costs nothing audible and buys most of the fold rejection. */
    static constexpr std::array<float, 4> butterworth_8_q{0.50979f, 0.60134f, 0.89998f, 2.56292f};
    static constexpr float anti_alias_cutoff = 0.43f;
    /* 4th-order Butterworth for the decimator: theta = 22.5, 67.5 degrees. */
    static constexpr std::array<float, 2> butterworth_4_q{0.54120f, 1.30656f};

    static constexpr float dec_cutoff_hz = 4500.f;
    static constexpr float dc_cutoff_hz  = 20.f;

}

void Shift_smooth::reset()
{
    history.fill(0.f);
    dec_history.fill(0.f);
    yin_snap.fill(0.f);
    yin_d.fill(0.f);

    /* Start the write counter a whole line in so that a read head placed behind
     * it never has to be expressed as a negative absolute position. */
    w_abs     = static_cast<uint32_t>(history_size);
    dec_abs   = static_cast<uint32_t>(history_size / dec_factor);
    dec_phase = 0u;
    pre_valid = false;   // R3i

    active       = 0u;
    head_gain.fill(1.f);
    fading       = false;
    fade_pos     = 0.f;
    fade_step    = 0.f;
    splice_armed = false;
    splice_dd    = 0;

    interval      = 0;
    ratio         = 1.f;
    anti_alias_on = false;

    yin_cursor   = 0u;
    yin_running  = false;
    period       = default_grain;
    period_valid = false;
    period_hold  = 0u;
    weak_valid   = false;   // R3j
    yin_burst_on    = false;   // R3n
    burst_prev      = 0.f;     // R3o
    yin_burst_count = 0u;
    yin_wait        = 0u;
    weak_agree   = 0u;      // R3k
    found_count  = 0u;   // smooth
    for(auto& g : grains) g = OlaGrain{};   // R3

    onset_lp        = 0.f;
    onset_fast      = 0.f;
    onset_cur       = 0.f;
    onset_prev      = 0.f;
    onset_frame     = 0u;
    onset_frame_len = 512u;
    onset_hold      = 0u;
    onset_age       = onset_grain_hold;

    dc_block.set_parameters(dc_cutoff_hz * norm_sample_rate, 0.707f);
    dc_block.reset();

    for(size_t i = 0; i < dec_filter.size(); i++)
    {
        dec_filter[i].set_parameters(dec_cutoff_hz * norm_sample_rate, butterworth_4_q[i]);
        dec_filter[i].reset();
    }
    for(size_t i = 0; i < anti_alias.size(); i++)
    {
        anti_alias[i].set_parameters(anti_alias_cutoff, butterworth_8_q[i]);
        anti_alias[i].reset();
    }

    _update_grain();
    _seek(head[0], lag_lo);
    head[1] = head[0];
}

void Shift_smooth::_seek(Head& h, float lag)
{
    const float whole = std::floor(lag);
    const float f     = lag - whole;
    if(f <= 1.0e-4f)
    {
        h.pos  = w_abs - static_cast<uint32_t>(whole);
        h.frac = 0.f;
    }
    else
    {
        h.pos  = w_abs - static_cast<uint32_t>(whole) - 1u;
        h.frac = 1.f - f;
    }
}

void Shift_smooth::_write(float x)
{
    history[w_abs & history_mask] = x;

    const float lp = dec_filter[1].process(dec_filter[0].process(x));
    if(++dec_phase >= dec_factor)
    {
        dec_phase = 0u;
        dec_history[dec_abs & dec_mask] = lp;
        dec_abs++;
    }
    w_abs++;
}

float Shift_smooth::_read(const Head& h) const
{
    const float    ph = h.frac * static_cast<float>(shift_smooth::sinc_phases);
    uint32_t       p  = static_cast<uint32_t>(ph);
    if(p >= shift_smooth::sinc_phases) p = shift_smooth::sinc_phases - 1u;
    const float    pf = ph - static_cast<float>(p);

    const float* k0 = shift_smooth::sinc_table[p].data();
    const float* k1 = shift_smooth::sinc_table[p + 1u].data();

    const uint32_t base = h.pos - static_cast<uint32_t>(shift_smooth::sinc_centre);

    float acc = 0.f;
    for(size_t j = 0; j < shift_smooth::sinc_taps; j++)
    {
        const float k = k0[j] + pf * (k1[j] - k0[j]);
        acc += k * history[(base + static_cast<uint32_t>(j)) & history_mask];
    }
    return acc;
}

void Shift_smooth::_update_interval()
{
    const int16_t semis = static_cast<int16_t>(idsp::clamp<int32_t>(controls.shift_amount, -12, 12));
    if(semis == interval) return;

    interval = semis;
    ratio    = exact_ratio ? std::pow(2.f, static_cast<float>(semis) / 12.f)   // lat: exact; fast_exp2 is up to 0.88 c flat
                         : tairm::fast_exp2(static_cast<float>(semis) * (1.f / 12.f));

    /* A new interval can reverse which way the head travels, and a distance
     * searched for the old direction is meaningless in the new one. */
    splice_armed = false;

    /* Reading the line faster than it is written decimates it, so everything
     * above Nyquist/ratio in the *input* folds into the band the shifter is
     * supposed to be producing. Filtering here, on the write, is what lets the
     * read interpolator stay one full-band kernel instead of a bank per
     * interval -- and it is the correct place for it, because the fold happens
     * at the resample, not after it. */
    anti_alias_on = (ratio > 1.f);
    if(anti_alias_on)
    {
        const float fc = anti_alias_cutoff / ratio;
        for(size_t i = 0; i < anti_alias.size(); i++)
        {
            anti_alias[i].set_parameters(fc, butterworth_8_q[i]);
        }
    }

    _update_grain();
}

void Shift_smooth::_update_grain()
{
    const float p = period_valid ? tairm::clamp(period, min_period, max_period) : ((onset_age < onset_grain_hold) ? onset_grain : default_grain);   // E5

    /* The splice distance is a whole number of pitch periods. That is the whole
     * trick: two heads a period apart are in phase, so the crossfade between
     * them adds rather than combs. The min_grain floor is there because on the
     * top strings a period is a few dozen samples and splicing that often costs
     * more in smearing than the shorter excursion buys in latency. */
    const float k = std::ceil(min_grain / p);
    grain = k * p;

    /* Between splices the lag moves at |1 - ratio| per output sample, so
     * splices fall grain/|1 - ratio| apart. A quarter of that is long enough to
     * be smooth and short enough that the fade is always finished before the
     * next splice comes due. */
    const float drift = tairm::max(std::fabs(1.f - ratio), 1.0e-4f);
    xfade = tairm::min(tairm::clamp(xfade_frac * grain / drift, xfade_min, 1024.f),   // smooth: xfade_min
                       0.9f * grain / drift);

    /* Two very different jobs share this window. When the period is known the
     * splice distance is already predicted and the correlation only has to
     * refine it, so a short window is enough and its reach is pure latency.
     * When it is not -- a chord, a scrape, anything inharmonic -- the search
     * has no prediction to lean on and the window is the only thing telling it
     * one alignment from another; at that point it has to span more than a
     * period of the lowest content or it is matching noise. Paying for that in
     * latency only on material that actually needs it is the whole point of
     * having the tracker decide. */
    corr_window = period_valid ? tairm::clamp(p, tracked_corr_min, max_corr_window)
                               : fallback_corr_window;

    /* The onset frame must never be shorter than a period, or a frame can land
     * between two clicks, come up empty, and make the next one look like an
     * attack. The floor covers the top strings, where several periods fit in
     * the shortest frame worth running. */
    onset_frame_len = static_cast<uint32_t>(period_valid ? tairm::clamp(p, 128.f, 1024.f)
                                                         : 1024.f);

    /* Two things set the floor on the lag, and the larger wins. The similarity
     * search is centred on the head, so it reaches half a window past it into
     * samples that have to be written already; and on an upshift the outgoing
     * head keeps falling for the whole crossfade after the splice has moved on. */
    lag_floor = guard_samples + (causal_corr ? 0.f : 0.5f * corr_window);   // E13: causal windows need no look-ahead
    lag_lo    = tairm::max(lag_floor,
                           guard_samples + (xfade * tairm::max(ratio - 1.f, 0.f)));
    /* E2: a downshift splice lands at lag - d, and the search can return d longer than grain by its
     * whole reach. With lag_lo sitting on lag_floor that target gets clamped to the floor, the jump
     * stops being a period multiple, and every splice slips phase flat. Give it the reach as headroom. */
    if(ratio < 1.f)
    {
        lag_lo += (period_valid ? tairm::max(p * 0.125f, 12.f) : tairm::min(0.35f * grain, 64.f))
                + static_cast<float>(max_fine_reach);
    }
    lag_hi    = lag_lo + grain;

    const float ceiling = static_cast<float>(history_size)
                        - corr_window - guard_samples - (0.30f * grain) - 64.f;
    if(lag_hi > ceiling)
    {
        lag_hi = ceiling;
        if(lag_lo > lag_hi - 64.f) lag_lo = tairm::max(lag_hi - 64.f, lag_floor);
        grain = lag_hi - lag_lo;
    }
}

void Shift_smooth::_pitch_tick()
{
    if(restart_age < 100000u) restart_age++;   // R3q
    if(!yin_running)
    {
        if(yin_wait > 0u) { yin_wait--; return; }   // R3n: let post-onset audio fill the snapshot
        if(dec_abs < static_cast<uint32_t>(yin_analysis)) return;

        const uint32_t start = dec_abs - static_cast<uint32_t>(yin_analysis);
        for(size_t i = 0; i < yin_analysis; i++)
        {
            yin_snap[i] = dec_history[(start + static_cast<uint32_t>(i)) & dec_mask];
        }
        yin_cursor  = yin_min_lag;
        yin_running = true;
    }

    /* The difference function is retired a few lags per block. A whole pass is
     * ~50k multiply-accumulates, which would be a spike big enough to matter in
     * a 16-sample block; spread over 25 blocks it is a flat few percent, and an
     * 8 ms update rate is still far faster than a guitar changes note. */
    const size_t per_block = yin_burst_on ? std::max(yin_per_block, static_cast<size_t>(std::max(yin_burst, 1)))
                                          : yin_per_block;   // R3n
    const size_t end = ((yin_cursor + per_block) < (yin_max_lag + 1))
                     ? (yin_cursor + per_block)
                     : (yin_max_lag + 1);

    for(size_t tau = yin_cursor; tau < end; tau++)
    {
        float acc = 0.f;
        for(size_t n = 0; n < yin_window; n++)
        {
            const float d = yin_snap[n] - yin_snap[n + tau];
            acc += d * d;
        }
        yin_d[tau] = acc;
    }
    yin_cursor = end;

    if(yin_cursor > yin_max_lag)
    {
        _yin_finalise();
        yin_running = false;
        if(yin_burst_on)   // R3n: stop on a fresh lock (finalise re-arms period_hold) or after the frame cap
        {
            yin_burst_count++;
            const bool fresh = period_valid && (period_hold == yin_hold_frames);
            bool done = fresh;
            if(fresh && (yin_burst_snap > 0))   // R3q: keep refining at burst speed until two locks agree within 1 %
            {
                done = (burst_last > 0.f) && (std::fabs(period - burst_last) < (0.01f * burst_last));
                burst_last = period;
            }
            if(done || yin_burst_count >= static_cast<uint32_t>(std::max(yin_burst_frames, 1))) yin_burst_on = false;
        }
    }
}

void Shift_smooth::_yin_finalise()
{
    /* Reject silence outright rather than letting the normalisation turn noise
     * into a confident wrong answer. */
    float energy = 0.f;
    for(size_t n = 0; n < yin_window; n++) energy += yin_snap[n] * yin_snap[n];
    if(energy < 1.0e-6f * static_cast<float>(yin_window))
    {
        period_hold  = 0u;
        period_valid = false;
        weak_valid   = false;   // R3j
        weak_agree   = 0u;      // R3k
        return;
    }

    /* YIN's cumulative mean normalisation, in place over the raw difference
     * function. Dividing each d(tau) by the running mean of everything below it
     * is what keeps the search off the trivial zero at tau = 0. */
    float running = 0.f;
    for(size_t tau = yin_min_lag; tau <= yin_max_lag; tau++)
    {
        running += yin_d[tau];
        const float count = static_cast<float>(tau - yin_min_lag + 1);
        yin_d[tau] = (running > 1.0e-20f) ? (yin_d[tau] * count / running) : 1.f;
    }

    /* Take the *first* dip under the threshold, walked down to its local
     * minimum, not the deepest dip anywhere. The deepest is usually an octave
     * below the note, which on a shifter is the one error you always hear. */
    size_t chosen = 0;
    for(size_t tau = yin_min_lag; tau <= yin_max_lag; tau++)
    {
        if(yin_d[tau] < yin_threshold)
        {
            while((tau + 1) <= yin_max_lag && yin_d[tau + 1] < yin_d[tau]) tau++;
            chosen = tau;
            break;
        }
    }

    if(chosen == 0)
    {
        /* One unconvincing frame is usually a pick attack passing through the
         * analysis window, not the end of the note. Let the last good period
         * stand for a few frames rather than dropping to the blind path and
         * jolting the grain length -- and with it the latency -- mid-note. */
        if(period_hold > 0u) period_hold--;
        period_valid = (period_hold > 0u);
        /* R3j: chords with no short common period never pass the threshold; the deepest dip still names the
         * lag at which most partials line up, which is a far better OLA jump than a fixed fallback. */
        const bool was_weak = weak_valid;   // R3l
        weak_valid = false;
        if(yin_weak_threshold <= 0.f) weak_agree = 0u;
        if(yin_weak_threshold > 0.f)
        {
            /* R3r: the strong tracker wants a long range (a fourth's common period is 3 note periods), but the
             * deepest dip out there can be a partial-chord multiple that aligns some notes and not others. */
            const size_t weak_hi = (yin_weak_max_lag > 0 && static_cast<size_t>(yin_weak_max_lag) < yin_max_lag)
                                 ? static_cast<size_t>(yin_weak_max_lag) : yin_max_lag;
            size_t deep = yin_min_lag;
            for(size_t tau = yin_min_lag + 1; tau <= weak_hi; tau++)
                if(yin_d[tau] < yin_d[deep]) deep = tau;
            if(!(yin_d[deep] < yin_weak_threshold && deep > yin_min_lag)) weak_agree = 0u;
            if(yin_d[deep] < yin_weak_threshold && deep > yin_min_lag)
            {
                float tw = static_cast<float>(deep);
                if(deep < weak_hi)
                {
                    const float a = yin_d[deep - 1];
                    const float b = yin_d[deep];
                    const float c = yin_d[deep + 1];
                    const float denom = a - (2.f * b) + c;
                    if(denom > 1.0e-9f) tw += 0.5f * (a - c) / denom;
                }
                const float wp = tw * static_cast<float>(dec_factor);
                /* R3k: require N consecutive agreeing weak frames before trusting the dip. */
                if(weak_agree > 0u && std::fabs(wp - weak_cand) < (yin_weak_tol * weak_cand)) weak_agree++;
                else weak_agree = 1u;
                weak_cand = wp;
                if(weak_agree >= static_cast<uint32_t>(tairm::max(yin_weak_stable, 1.f)))
                {
                    weak_period = wp;
                    weak_valid  = true;
                }
            }
        }
        /* R3l: an established weak period survives a few missing/disagreeing frames (strum, decay) with its old
         * value; a one-off dip is never adopted, it only spends the hold. */
        if(weak_valid) weak_hold_left = static_cast<uint32_t>(tairm::max(yin_weak_hold, 0.f));
        else if(was_weak && weak_hold_left > 0u && yin_weak_threshold > 0.f)
        {
            weak_hold_left--;
            weak_valid = true;
        }
        return;
    }

    /* G2: subharmonic (octave-down) check -- see yin_octave_tol. */
    if(yin_octave_tol > 0.f && chosen > 0)
    {
        for(int rep = 0; rep < 2; rep++)
        {
            const size_t t2 = chosen * 2u;
            if(t2 + 1u > yin_max_lag) break;
            /* the dip near 2*tau can sit a sample or two either side */
            size_t best = t2;
            const size_t w = (chosen / 16u) + 2u;
            const size_t lo = (t2 > w) ? (t2 - w) : yin_min_lag;
            const size_t hi = ((t2 + w) < yin_max_lag) ? (t2 + w) : yin_max_lag;
            for(size_t tau = lo; tau <= hi; tau++) if(yin_d[tau] < yin_d[best]) best = tau;
            if(yin_d[best] >= yin_octave_tol * yin_d[chosen]) break;
            chosen = best;
        }
    }

    float tau_f = static_cast<float>(chosen);
    if(chosen > yin_min_lag && chosen < yin_max_lag)
    {
        const float a = yin_d[chosen - 1];
        const float b = yin_d[chosen];
        const float c = yin_d[chosen + 1];
        const float denom = a - (2.f * b) + c;
        if(denom > 1.0e-9f) tau_f += 0.5f * (a - c) / denom;
    }

    const float found = tau_f * static_cast<float>(dec_factor);

    /* R3o: the first post-onset frame still holds the attack transition and can name a period 10-15 % off; adopted
     * outright (it exceeds period_jump) it misaligns every grain until the next frame. During a burst, only adopt an
     * estimate the following burst frame confirms. */
    if(yin_burst_on && (yin_burst_confirm > 0))
    {
        const bool agree = (burst_prev > 0.f) && (std::fabs(found - burst_prev) < (period_jump * burst_prev));
        burst_prev = found;
        if(!agree) return;
    }

    /* Track small drifts (bends, vibrato) smoothly so the grain length does not
     * jitter; jump outright on a real note change. */
    /* smooth: a single bad frame (a partial or the attack locking briefly) shouldn't move the grain length, and
     * a real drift should glide rather than step, since every period step is a splice-length step. */
    float est = found;
    if(period_median3)
    {
        found_hist[found_count % 3u] = found;
        found_count++;
        if(found_count >= 3u)
        {
            const float a = found_hist[0], b = found_hist[1], c = found_hist[2];
            est = std::max(std::min(a, b), std::min(std::max(a, b), c));
        }
    }
    if(period_valid && std::fabs(est - period) < (period_jump * period) && !(yin_burst_on && (yin_burst_snap > 0)))   // R3q: bursts snap
    {
        float step = period_alpha * (est - period);
        if(period_slew > 0.f)
        {
            const float lim = period_slew * period;
            step = tairm::clamp(step, -lim, lim);
        }
        period += step;
    }
    else
    {
        period = est;
    }
    period_valid = true;
    period_hold  = yin_hold_frames;
    weak_valid   = false;   // R3j
    weak_agree   = 0u;      // R3k
}

int32_t Shift_smooth::_splice_coarse(uint32_t ref, int32_t sign)
{
    /* Coarse pass on the 4x decimated line.
     *
     * Tracked, this is only a refinement of a prediction: a narrow sweep either
     * side of one grain, at full decimated resolution. Blind, it degenerates to
     * a plain WSOLA similarity search -- a third of a grain either side, wide
     * enough to reach the common period of a chord. */
    const float   span = period_valid ? tairm::max(period * 0.125f, 12.f) : tairm::min(0.35f * grain, blind_span_cap);   /* E4b: cap blind reach at the original 0.35*768 */
    /* Blind, the span is nearly seven times wider than the tracked one, so the
     * grid is stepped out to match rather than paying for the extra reach
     * candidate by candidate. Eight decimated samples is 32 input samples, and
     * the fine pass resolves that -- moving the work to the stage that costs a
     * sixth as much per candidate. */
    const int32_t step = period_valid ? 1 : 8;

    const int32_t dd_centre = static_cast<int32_t>((grain * 0.25f) + 0.5f);
    const int32_t dd_span   = static_cast<int32_t>((span  * 0.25f) + 0.5f);
    const int32_t dd_lo = idsp::max<int32_t>(dd_centre - dd_span, 2);
    int32_t       dd_hi = dd_centre + dd_span;

    const int32_t  wd = static_cast<int32_t>(corr_window) / static_cast<int32_t>(dec_factor);
    /* Centred on the head rather than starting at it. Straddling the splice
     * point keeps WSOLA's look-ahead -- the transition itself is inside the
     * window being matched -- for half the reach into unwritten samples, and
     * that reach is what sets the latency floor.
     *
     * Only as far past it as the line has actually been written, though. The
     * tracker can widen corr_window under a head that was placed when it was
     * narrow -- losing lock doubles it -- and the far half of the window then
     * reaches past the write head into the previous lap of the ring. That is
     * not silence, it is a lap-old copy of the same instrument, so it
     * correlates like real audio and the splice lands confidently in the wrong
     * place. Slide the window back rather than let it read what is not there
     * yet; the look-ahead is worth having but it is not worth inventing. */
    const uint32_t head_d = ref / static_cast<uint32_t>(dec_factor);
    uint32_t rd = head_d - static_cast<uint32_t>(causal_corr ? wd : wd / 2);   // E13
    if(!causal_corr && static_cast<int32_t>(head_d + static_cast<uint32_t>(wd / 2) - dec_abs) > 0)   // E13: a causal window never reaches past the head
    {
        rd = dec_abs - static_cast<uint32_t>(wd);
    }

    /* On a downshift the candidates march *towards* the write head, so it is
     * the far end of the search -- not the reference -- that runs out of line
     * first. The furthest candidate the search may legally reach is the one
     * whose window ends exactly on the last sample written; past that it is
     * scoring the previous lap again. */
    if(sign > 0)
    {
        const int32_t room = static_cast<int32_t>(dec_abs - rd) - wd;
        dd_hi = idsp::max<int32_t>(idsp::min<int32_t>(dd_hi, room), dd_lo);
    }

    float   best_score = -1.0e30f;
    int32_t best_dd    = dd_centre;

    for(int32_t dd = dd_lo; dd <= dd_hi; dd += step)
    {
        const uint32_t cd = rd + static_cast<uint32_t>(sign * dd);
        float num = 0.f;
        float den = 1.0e-12f;
        for(int32_t n = 0; n < wd; n++)
        {
            const float a = dec_history[(rd + static_cast<uint32_t>(n)) & dec_mask];
            const float b = dec_history[(cd + static_cast<uint32_t>(n)) & dec_mask];
            num += a * b;
            den += b * b;
        }
        /* Normalise by the candidate's energy only -- the reference is the same
         * for every lag, so its norm cannot change the ranking, and leaving it
         * out keeps the inner loop to two multiply-accumulates. */
        const float score = num / std::sqrt(den);
        if(score > best_score)
        {
            best_score = score;
            best_dd    = dd;
        }
    }

    return best_dd;
}

float Shift_smooth::_splice_fine(uint32_t ref, int32_t sign, int32_t best_dd) const
{
    /* Blind, the grid the coarse pass left behind is 8 decimated samples; that
     * is what sets the reach, and it has to be re-derived here rather than
     * carried, because the tracker can gain or lose lock between the block that
     * ran the coarse pass and the block that fires the splice. Re-deriving it
     * can only widen or narrow the window the peak is hunted in, never move it
     * off the distance the coarse pass chose. */
    const int32_t step  = period_valid ? 1 : 8;
    const int32_t reach = idsp::min<int32_t>(idsp::max<int32_t>(idsp::max<int32_t>(2 * step, 4), fine_reach_min),
                                             max_fine_reach);   // smooth: fine_reach_min

    /* Fine pass at full rate, over the half-step the coarse grid could not
     * resolve, and then a parabolic step on the correlation peak that takes the
     * alignment below a sample -- which the read interpolator can actually use,
     * so the two heads end up phase-locked rather than merely close. Resolving
     * a grid step is all this stage does, so it does not need the long window
     * the blind coarse search does. */
    const int32_t w    = static_cast<int32_t>(tairm::min(corr_window, fine_corr_window));
    const int32_t d0   = best_dd * static_cast<int32_t>(dec_factor);
    const uint32_t rbase = ref - static_cast<uint32_t>(causal_corr ? w : w / 2);   // E13

    /* Every offset, not a strided sweep. The peak being resolved here is only
     * as wide as the signal's top octave leaves it -- a few samples on
     * anything bright, nothing like a fraction of a period -- so a stride of
     * even 2 steps over it and lands on the wrong sample several percent of
     * the time. Splitting the search across two blocks is what buys the budget
     * to keep this exhaustive. */
    std::array<float, (2 * max_fine_reach) + 1> fine{};
    int32_t best_i = reach;
    float   best_f = -1.0e30f;

    for(int32_t i = 0; i < ((2 * reach) + 1); i++)
    {
        const int32_t  d  = d0 + i - reach;
        const uint32_t cp = rbase + static_cast<uint32_t>(sign * d);
        float num = 0.f;
        float den = 1.0e-12f;
        for(int32_t n = 0; n < w; n++)
        {
            float a = history[(rbase + static_cast<uint32_t>(n)) & history_mask];
            float b = history[(cp    + static_cast<uint32_t>(n)) & history_mask];
            if(corr_preemph > 0.f)   // smooth: HF-weighted alignment
            {
                a -= corr_preemph * history[(rbase + static_cast<uint32_t>(n) - 1u) & history_mask];
                b -= corr_preemph * history[(cp    + static_cast<uint32_t>(n) - 1u) & history_mask];
            }
            num += a * b;
            den += b * b;
        }
        fine[static_cast<size_t>(i)] = num / std::sqrt(den);
        if(fine[static_cast<size_t>(i)] > best_f)
        {
            best_f = fine[static_cast<size_t>(i)];
            best_i = i;
        }
    }

    /* smooth: fully normalised correlation at the chosen alignment, for the crossfade law. Real guitar partials
     * are stretched, so a period-aligned splice keeps the low partials in phase but not the upper ones; the
     * broadband figure tells the fade how much of the signal will actually add coherently. */
    {
        float ref_e = 1.0e-12f;
        for(int32_t n = 0; n < w; n++)
        {
            float a = history[(rbase + static_cast<uint32_t>(n)) & history_mask];
            if(corr_preemph > 0.f) a -= corr_preemph * history[(rbase + static_cast<uint32_t>(n) - 1u) & history_mask];
            ref_e += a * a;
        }
        splice_rho = tairm::clamp(best_f / std::sqrt(ref_e), 0.f, 1.f);
    }

    float out = static_cast<float>(d0 + best_i - reach);
    if(best_i > 0 && best_i < (2 * reach))
    {
        const float a = fine[static_cast<size_t>(best_i) - 1u];
        const float b = fine[static_cast<size_t>(best_i)];
        const float c = fine[static_cast<size_t>(best_i) + 1u];
        const float denom = a - (2.f * b) + c;
        if(denom < -1.0e-9f) out += 0.5f * (a - c) / denom;
    }

    /* smooth: the integer-grid parabola is only as good as the NCC's curvature over one sample. Re-evaluate the
     * correlation at +-0.25 smp around it through the same sinc interpolator the read heads use, and take that
     * parabola's vertex, so the two heads are aligned to a fraction of a sample on the waveform itself. */
    if(subsample_refine)
    {
        constexpr double h = 0.25;
        const float c_m = _ncc_frac(rbase, static_cast<double>(out) - h, sign, w);
        const float c_0 = _ncc_frac(rbase, static_cast<double>(out), sign, w);
        const float c_p = _ncc_frac(rbase, static_cast<double>(out) + h, sign, w);
        const float denom = c_m - (2.f * c_0) + c_p;
        if(denom < -1.0e-9f)
        {
            out += static_cast<float>(h) * tairm::clamp(0.5f * (c_m - c_p) / denom, -1.f, 1.f);
        }
    }

    return tairm::max(out, 32.f);
}

void Shift_smooth::_start_fade(float target_lag, float length, bool match_level)
{
    /* Anything already fading is abandoned rather than queued. The only two
     * callers are a splice (which cannot fire mid-fade) and an onset (which is
     * loud enough to cover dropping the outgoing head on the spot). */
    /* The similarity search is free to return a distance that puts the incoming
     * head somewhere useless, so the ring's own bounds get the last word: a bad
     * splice should cost one audible seam, never a read off the end of the
     * line. */
    const float ceiling = static_cast<float>(history_size)
                        - corr_window - guard_samples - (0.30f * grain) - 64.f;

    Head& next = head[active ^ 1u];
    _seek(next, tairm::clamp(target_lag, lag_floor, ceiling));

    if(!match_level) splice_rho = 0.f;   // smooth: onset re-seats and range recovery are not aligned: treat as uncorrelated

    /* E6: the heads are period-aligned, so a short window compares like with like. Match the incoming
     * head's level to the outgoing one so a decaying note doesn't step at every splice; the gain then
     * relaxes to unity slowly enough to be an inaudible glide. Onsets keep their level (no match). */
    /* E7: whole periods (>= 256 smp), looking back from each head so only written samples are read; only
     * when tracked, since a chord has no period to make the two windows comparable. */
    const int32_t w = period_valid
                    ? static_cast<int32_t>(tairm::clamp(std::ceil(256.f / period) * period, 128.f, 1024.f))
                    : 0;
    if(match_level && period_valid)
    {
        auto energy = [this, w](const Head& h)
        {
            float e = 1.0e-9f;
            for(int32_t j = -w; j < 0; j++)
            {
                const float v = history[(h.pos + static_cast<uint32_t>(j)) & history_mask];
                e += v * v;
            }
            return e;
        };
        float gm = std::sqrt(energy(head[active]) / energy(next));
        if(std::fabs(gm - 1.f) < 0.002f) gm = 1.f;   // E7: deadband, a steady tone gets no gain step at all
        head_gain[active ^ 1u] = head_gain[active] * tairm::clamp(gm, 0.7f, 1.4f);
    }
    else
    {
        head_gain[active ^ 1u] = head_gain[active];
    }
    active   ^= 1u;
    fading    = true;
    fade_pos  = 0.f;
    fade_step = 1.f / tairm::max(length, 1.f);

    /* Every fade lands the head somewhere the armed coarse result was not
     * chosen for -- including the splice that armed it, which has just spent
     * it. Nothing pending survives a seek. */
    splice_armed = false;
}

void Shift_smooth::process(const MonoDspBuffer& input, MonoDspBuffer& output)
{
    _update_interval();
    _update_grain();

    if(ola_mode != 0)   // R3: continuous overlap-add time repair
    {
        _process_ola(input, output);
        return;
    }

    /* The period search and the splice search are the expensive things that
     * happen at block rate, and no two of them may land in the same block. The
     * period search is elastic and simply takes a block longer to finish. The
     * splice cannot wait -- but its coarse half can be run ahead of time, so
     * what is staggered is coarse, then fine-and-fire, then YIN resumes. */
    bool searched = false;

    const bool  shifting  = (interval != 0);
    const int32_t sign    = (ratio > 1.f) ? -1 : 1;
    const float drift     = tairm::max(std::fabs(1.f - ratio), 1.0e-4f);
    const float safe_high = static_cast<float>(history_size)
                          - corr_window - guard_samples - (0.30f * grain) - 64.f;

    for(size_t i = 0; i < audio_block_size; i++)
    {
        // ---- write side ----------------------------------------------------
        float x = dc_block.process(input[i]);
        if(anti_alias_on)
        {
            x = anti_alias[0].process(x);
            x = anti_alias[1].process(x);
            x = anti_alias[2].process(x);
            x = anti_alias[3].process(x);
        }
        _write(x);

        // ---- onset ---------------------------------------------------------
        onset_lp   += (x - onset_lp) * onset_lp_coef;
        const float hf = std::fabs(x - onset_lp);
        onset_fast += (hf - onset_fast) * onset_fast_coef;

        bool onset = false;
        if(onset_hold > 0u)
        {
            onset_hold--;
        }
        else if(onset_fast > ((onset_ratio * onset_prev) + onset_floor))
        {
            onset      = true;
            onset_hold = onset_refractory; onset_age = 0u;   // E5
        }

        onset_cur = tairm::max(onset_cur, onset_fast);
        if(onset_frame == 0u)
        {
            onset_prev  = onset_cur;
            onset_cur   = 0.f;
            onset_frame = onset_frame_len;
        }
        onset_frame--;
        if(onset_age < onset_grain_hold) onset_age++;   // E5

        // ---- head scheduling -----------------------------------------------
        const float lag = static_cast<float>(w_abs - head[active].pos) - head[active].frac;

        if(lag < guard_samples || lag > safe_high)
        {
            /* Only reachable in the block after an interval change, where the
             * bounds move under a head that was legal a moment ago. */
            if(FILE* ev = events_file()) std::fprintf(ev, "%u R %.1f\n", w_abs - static_cast<uint32_t>(history_size), lag);
            _start_fade((ratio > 1.f) ? lag_hi : lag_lo, onset_fade);
        }
        else if(onset && shifting && onset_reseat)
        {
            /* Re-seat so the pick gets splice-free runway across the attack.
             * Without it the attack is the one thing in the signal that gets
             * repeated, and a repeated attack is a flam -- the single most
             * recognisable pitch-shifter artefact. The seek itself lands inside
             * the attack, which is also the only place in the signal that hides
             * it.
             *
             * Upshifts have to buy that runway with latency, because the lag is
             * what the head spends catching up; so buy exactly onset_runway
             * worth and no more. At an octave that is the whole grain, but at a
             * semitone the lag barely moves and a few dozen samples cover the
             * same 25 ms. Downshifts get the runway for free by sitting at the
             * bottom of the excursion, which is also the lowest latency. */
            const float target = (ratio > 1.f)
                               ? tairm::min(lag_hi, lag_lo + (onset_runway * (ratio - 1.f)))
                               : lag_lo;
            if(std::fabs(target - lag) > 8.f)
            {
                if(FILE* ev = events_file()) std::fprintf(ev, "%u O %.1f\n", w_abs - static_cast<uint32_t>(history_size), lag);
                _start_fade(target, onset_fade);
            }
        }
        else if(shifting && !fading)
        {
            const bool due = (ratio > 1.f) ? (lag <= lag_lo) : (lag >= lag_hi);

            if(due && splice_armed && !searched)
            {
                const float d = _splice_fine(head[active].pos, sign, splice_dd);
                if(FILE* ev = events_file()) std::fprintf(ev, "%u S %.1f %.3f %.2f %.1f %d %.3f\n", w_abs - static_cast<uint32_t>(history_size), lag, splice_rho,
                                          period, grain, period_valid ? 1 : 0, d);
                _start_fade(lag - (static_cast<float>(sign) * d), xfade, true);
                searched = true;
            }
            else if(!splice_armed && !searched)
            {
                /* Two blocks of run-up, not one. Arming is per-sample, so a
                 * single block's worth would let the arm and the splice fall on
                 * either side of the same block boundary -- which is the one
                 * arrangement the split exists to prevent. Two guarantees a
                 * whole block between them, and the fine pass then fires on the
                 * exact sample the splice comes due rather than overshooting to
                 * the next boundary. `due` itself still arms, for the first
                 * splice after a reset or an interval change, which has had no
                 * run-up at all. */
                const float lookahead = drift * static_cast<float>(2u * audio_block_size);
                const bool  soon = (ratio > 1.f) ? (lag <= (lag_lo + lookahead))
                                                 : (lag >= (lag_hi - lookahead));
                if(soon)
                {
                    splice_dd    = _splice_coarse(head[active].pos, sign);
                    splice_armed = true;
                    searched     = true;
                }
            }
        }

        // ---- read side -----------------------------------------------------
        float out;
        if(!shifting)
        {
            /* At zero semitones there is nothing to splice and nothing to
             * resample, so there is no reason to charge the player the delay
             * line for it: hand back the input. The machine keeps running
             * underneath -- line fed, period tracked, heads advancing at unity
             * so the lag holds -- so the first interval away from zero starts
             * from a settled state rather than from a cold buffer. */
            out = x;
        }
        else if(fading)
        {
            Head& outgoing = head[active ^ 1u];
            /* Smoothstep, and a plain (1-g, g) pair rather than an equal-power
             * one: the heads were aligned to a correlation peak, so they are
             * correlated, and correlated signals crossfade at constant
             * amplitude, not constant power. */
            const float t = fade_pos;
            const float g = t * t * (3.f - (2.f * t));
            /* smooth: amplitude-complementary gains only keep constant level for fully correlated heads; for the
             * uncorrelated part (stretched upper partials) they dip up to 3 dB mid-fade, once per splice = buzz. */
            float w_out = 1.f - g;
            float w_in  = g;
            if(xfade_law == 2)
            {
                w_out = std::cos(g * 1.5707964f);
                w_in  = std::sin(g * 1.5707964f);
            }
            else if(xfade_law == 1)
            {
                const float nrm = std::sqrt(tairm::max((w_out * w_out) + (w_in * w_in) + (2.f * splice_rho * w_out * w_in), 1.0e-6f));
                w_out /= nrm;
                w_in  /= nrm;
            }
            out = (w_out * head_gain[active ^ 1u] * _read(outgoing)) + (w_in * head_gain[active] * _read(head[active]));

            _advance(outgoing);
            fade_pos += fade_step;
            if(fade_pos >= 1.f) fading = false;
        }
        else
        {
            out = head_gain[active] * _read(head[active]);
        }
        _advance(head[active]);
        head_gain[active] += (1.f - head_gain[active]) * gain_relax;   // E6

        output[i] = out;
    }

    if(!searched) _pitch_tick();
}


float Shift_smooth::_ncc_frac(uint32_t rbase, double dist, int32_t sign, int32_t w) const
{
    /* smooth: NCC between the reference window at rbase and a candidate window `dist` samples away (fractional),
     * read through the sinc interpolator exactly as a read head would hear it. */
    float num = 0.f;
    float den = 1.0e-12f;
    const double start = static_cast<double>(sign) * dist;
    for(int32_t n = 0; n < w; n++)
    {
        float        a  = history[(rbase + static_cast<uint32_t>(n)) & history_mask];
        if(corr_preemph > 0.f) a -= corr_preemph * history[(rbase + static_cast<uint32_t>(n) - 1u) & history_mask];
        const double p  = static_cast<double>(n) + start;
        const double fl = std::floor(p);
        Head h;
        h.pos  = rbase + static_cast<uint32_t>(static_cast<int64_t>(fl));
        h.frac = static_cast<float>(p - fl);
        float b = _read(h);
        if(corr_preemph > 0.f)
        {
            Head hp = h;
            hp.pos -= 1u;
            b -= corr_preemph * _read(hp);
        }
        num += a * b;
        den += b * b;
    }
    return num / std::sqrt(den);
}


/* R3: continuous period-synchronous overlap-add. */
void Shift_smooth::_ola_spawn(int youngest, bool at_min, bool presearch)
{
    int slot = -1;
    for(int k = 0; k < static_cast<int>(grains.size()); k++)
    {
        if(!grains[static_cast<size_t>(k)].active) { slot = k; break; }
    }
    if(slot < 0)   // all busy: replace the oldest
    {
        slot = 0;
        for(int k = 1; k < static_cast<int>(grains.size()); k++)
            if(grains[static_cast<size_t>(k)].age > grains[static_cast<size_t>(slot)].age) slot = k;
    }

    const bool  p_weak = !period_valid && weak_valid && (yin_weak_threshold > 0.f);   // R3j
    const bool  p_ok   = period_valid || p_weak;
    const float p_in = period_valid ? tairm::clamp(period, min_period, max_period)
                     : (p_weak ? tairm::clamp(weak_period, min_period, max_period) : ola_fallback_period);
    const float len_f = tairm::clamp(ola_periods * p_in / ratio, ola_min_len, ola_max_len);
    uint32_t len = static_cast<uint32_t>(len_f * 0.5f) * 2u;
    if(len < 16u) len = 16u;

    /* An upshift grain eats len*(ratio-1) samples of lag over its life, so it must start that far back, plus the
     * interpolator's reach. Half a period on top leaves room to land on a whole-period jump. */
    const float need   = guard_samples + 16.f + tairm::max(0.f, static_cast<float>(len) * (ratio - 1.f));
    const float target = need + ola_lag_extra + (0.5f * p_in);
    const float ceiling = static_cast<float>(history_size) - static_cast<float>(ola_window) - 96.f;

    float lag_new = at_min ? need : target;   // R3c: onset restarts at the minimum lag
    float ev_score = -2.f;                    // debug log: best full-rate NCC of the chosen alignment (-2 = none)
    if(youngest >= 0 && !at_min)
    {
        const Head& prev = grains[static_cast<size_t>(youngest)].h;
        const float lag_nat = static_cast<float>(w_abs - prev.pos) - prev.frac;
        int32_t reach = ola_reach;
        if(p_ok)   // R3j: a weak period still quantises the jump
        {
            const float m = std::round((lag_nat - target) / p_in);
            lag_new = lag_nat - (m * p_in);
        }
        else
        {
            lag_new = target;
            reach = tairm::max(static_cast<float>(reach), 0.5f * p_in) > 64.f ? 64 : static_cast<int32_t>(tairm::max(static_cast<float>(reach), 0.5f * p_in));
        }
        while(lag_new < need) lag_new += p_in;
        if(lag_new > ceiling) lag_new = ceiling;

        /* Causal NCC: the W samples just before the previous grain's read point vs the W samples before each
         * candidate start. */
        int32_t W = ola_window;
        const uint32_t ref_end = prev.pos;
        std::array<float, 129> sc{};
        float best = -1.0e30f;
        int32_t best_i = -1;
        int32_t r = idsp::min<int32_t>(reach, 64);

        /* R3e: two-stage search. The coarse pass runs on the 4x decimated line (every candidate k is a multiple of
         * the decimation factor there), then the full-rate pass below only has to resolve +-ola_fine_reach around it
         * with a short window: ~12x less work per spawn than scoring every offset at full rate.
         * R3f: the coarse pass keeps its best ola_coarse_cands local maxima; each is refined at full rate and the
         * highest full-rate score wins (a single coarse pick lands on the wrong peak on bright material). */
        std::array<int32_t, 4> centers{};
        int32_t n_centers = 1;
        centers[0] = static_cast<int32_t>(std::lround(lag_nat - lag_new));
        const int32_t k_snap = centers[0];   // R3i: presearch centres are stored relative to the period snap
        if(ola_coarse != 0)
        {
            if(!presearch && pre_valid && pre_grain == youngest)   // R3i: centres found in an earlier block
            {
                /* Upshift lag shrinks between presearch and spawn, so the snap may land whole periods away: move the
                 * stored centres by the snap change quantised to whole periods (the NCC peaks repeat per period; an
                 * untracked snap just drifts with the lag and quantises to no move). */
                const int32_t d = static_cast<int32_t>(std::lround(std::round(static_cast<float>(k_snap - pre_snap) / p_in) * p_in));
                n_centers = pre_n;
                for(int32_t ci = 0; ci < n_centers; ci++)
                    centers[static_cast<size_t>(ci)] = pre_centers[static_cast<size_t>(ci)] + d;
            }
            else
            {
                const int32_t df  = static_cast<int32_t>(dec_factor);
                const int32_t Wd  = idsp::max<int32_t>(W / df, 16);
                const int32_t rd  = idsp::min<int32_t>(idsp::max<int32_t>((r + df - 1) / df, 1), 16);
                const int32_t kd0 = static_cast<int32_t>(std::lround(static_cast<float>(centers[0]) / static_cast<float>(df)));
                const uint32_t ref_d = ref_end / static_cast<uint32_t>(df);
                std::array<float, 33> scd{};
                for(int32_t jd = -rd; jd <= rd; jd++)
                {
                    const int32_t kd = kd0 + jd;
                    const size_t id = static_cast<size_t>(jd + rd);
                    const float lag_c = lag_nat - static_cast<float>(kd * df);
                    if(lag_c < need || lag_c > ceiling) { scd[id] = -1.0e30f; continue; }
                    const uint32_t cand_d = ref_d + static_cast<uint32_t>(kd);
                    float num = 0.f;
                    float den = 1.0e-12f;
                    for(int32_t n = 1; n <= Wd; n++)
                    {
                        const float a = dec_history[(ref_d  - static_cast<uint32_t>(n)) & dec_mask];
                        const float b = dec_history[(cand_d - static_cast<uint32_t>(n)) & dec_mask];
                        num += a * b;
                        den += b * b;
                    }
                    scd[id] = num / std::sqrt(den);
                }
                /* Top-N local maxima (ends count as maxima against their single neighbour). */
                const int32_t want = idsp::min<int32_t>(idsp::max<int32_t>(ola_coarse_cands, 1), 4);
                std::array<float, 4> top_s{};
                top_s.fill(-1.0e29f);
                n_centers = 0;
                for(int32_t id = 0; id <= 2 * rd; id++)
                {
                    const float s = scd[static_cast<size_t>(id)];
                    if(s <= -1.0e29f) continue;
                    if(id > 0 && scd[static_cast<size_t>(id - 1)] > s) continue;
                    if(id < 2 * rd && scd[static_cast<size_t>(id + 1)] >= s) continue;
                    int32_t slot_i = n_centers < want ? n_centers : want - 1;
                    if(n_centers >= want && s <= top_s[static_cast<size_t>(slot_i)]) continue;
                    while(slot_i > 0 && top_s[static_cast<size_t>(slot_i - 1)] < s)
                    {
                        top_s[static_cast<size_t>(slot_i)]   = top_s[static_cast<size_t>(slot_i - 1)];
                        centers[static_cast<size_t>(slot_i)] = centers[static_cast<size_t>(slot_i - 1)];
                        slot_i--;
                    }
                    top_s[static_cast<size_t>(slot_i)]   = s;
                    centers[static_cast<size_t>(slot_i)] = (kd0 + id - rd) * df;
                    if(n_centers < want) n_centers++;
                }
                if(n_centers == 0) { centers[0] = kd0 * df; n_centers = 1; }
            }
            if(presearch)
            {
                pre_centers = centers;
                pre_snap    = k_snap;
                pre_n       = n_centers;
                pre_grain   = youngest;
                pre_valid   = true;
                return;
            }
            r = idsp::min<int32_t>(idsp::max<int32_t>(ola_fine_reach, 1), 64);
            W = idsp::max<int32_t>(ola_fine_window, 16);
        }
        /* R3d: candidates are INTEGER offsets k from the previous head's own sample grid (prev.pos), so reference and
         * candidates share one rounding and the chosen lag keeps prev.frac exactly. (Truncating w_abs - lag_c here
         * biased every grain ~0.5 smp the same way: a constant ~7 c pitch offset.) */
        bool found = false;
        float best_off = 0.f;
        const int32_t stride = ola_coarse != 0 ? idsp::min<int32_t>(idsp::max<int32_t>(ola_fine_stride, 1), 4) : 1;
        for(int32_t ci = 0; ci < n_centers; ci++)
        {
            const int32_t k0 = centers[static_cast<size_t>(ci)];   // forward distance of the snapped/coarse candidate
            best_i = -1;
            float best_c = -1.0e30f;
            for(int32_t j = -r; j <= r; j++)
            {
                const int32_t k = k0 + j;
                const float lag_c = lag_nat - static_cast<float>(k);
                const size_t idx = static_cast<size_t>(j + r);
                if(lag_c < need || lag_c > ceiling) { sc[idx] = -1.0e30f; continue; }
                const uint32_t cand_end = prev.pos + static_cast<uint32_t>(k);
                float num = 0.f;
                float den = 1.0e-12f;
                for(int32_t n = 1; n <= W; n += stride)   // R3h: stride > 1 keeps the window span at a fraction of the work
                {
                    const float a = history[(ref_end  - static_cast<uint32_t>(n)) & history_mask];
                    const float b = history[(cand_end - static_cast<uint32_t>(n)) & history_mask];
                    num += a * b;
                    den += b * b;
                }
                sc[idx] = num / std::sqrt(den);
                if(sc[idx] > best_c) { best_c = sc[idx]; best_i = static_cast<int32_t>(idx); }
            }
            if(best_i < 0 || best_c <= best) continue;
            best = best_c;
            found = true;
            best_off = static_cast<float>(k0 + best_i - r);   // R3d: absolute forward distance from prev
            if(best_i > 0 && best_i < 2 * r && sc[static_cast<size_t>(best_i - 1)] > -1.0e29f && sc[static_cast<size_t>(best_i + 1)] > -1.0e29f)
            {
                const float a = sc[static_cast<size_t>(best_i - 1)];
                const float b = sc[static_cast<size_t>(best_i)];
                const float c = sc[static_cast<size_t>(best_i + 1)];
                const float den = a - (2.f * b) + c;
                if(den < -1.0e-9f) best_off += tairm::clamp(0.5f * (a - c) / den, -0.5f, 0.5f);
            }
        }
        if(found)
        {
            lag_new = lag_nat - best_off;   // R3d: exact, preserves prev.frac
            ev_score = best;
        }
    }
    lag_new = tairm::clamp(lag_new, need, ceiling);

    OlaGrain& g = grains[static_cast<size_t>(slot)];
    _seek(g.h, lag_new);
    g.age = 0u;
    g.len = len;
    g.active = true;
    g.kill = false;
    g.fade = 1.f;
    /* G1: crossfade half-length. At >= 0.5 this is the original Hann grain (len/2); below it, a flat middle with
     * short fades, floored at 16 samples so the fade stays click-free. */
    {
        const float xf_f = tairm::clamp(ola_xfade_frac, 0.02f, 0.5f) * static_cast<float>(len);
        uint32_t xf = static_cast<uint32_t>(tairm::max(xf_f, 16.f));
        if(xf > len / 2u) xf = len / 2u;
        g.xf = xf;
    }
    pre_valid = false;   // R3i
    /* debug: "<t> G <lag_new> <p_in> <valid> <len> <at_min> <score>" */
    if(FILE* ev = events_file()) std::fprintf(ev, "%u G %.2f %.2f %d %u %d %.3f\n", w_abs - static_cast<uint32_t>(history_size), lag_new, p_in,
                                              period_valid ? 1 : 0, len, at_min ? 1 : 0, ev_score);
}

void Shift_smooth::_process_ola(const MonoDspBuffer& input, MonoDspBuffer& output)
{
    const bool shifting = (interval != 0);

    for(size_t i = 0; i < audio_block_size; i++)
    {
        float x = dc_block.process(input[i]);
        if(anti_alias_on)
        {
            x = anti_alias[0].process(x);
            x = anti_alias[1].process(x);
            x = anti_alias[2].process(x);
            x = anti_alias[3].process(x);
        }
        _write(x);

        // R3c: same onset detector as the splice path
        onset_lp   += (x - onset_lp) * onset_lp_coef;
        const float hf = std::fabs(x - onset_lp);
        onset_fast += (hf - onset_fast) * onset_fast_coef;
        bool onset = false;
        if(onset_hold > 0u)
        {
            onset_hold--;
        }
        else if(onset_fast > ((onset_ratio * onset_prev) + onset_floor))
        {
            onset      = true;
            onset_hold = onset_refractory;
            /* R3n: a new note needs a period before the grains can align; the in-flight YIN frame is mostly
             * pre-onset and a frame takes ~100 ms at 2 lags/block, so run faster until the first fresh lock. */
            if(yin_burst > 0)
            {
                yin_burst_on    = true;
                yin_burst_count = 0u;
                burst_prev      = 0.f;   // R3o
                burst_last      = 0.f;   // R3q
                /* R3t: a pick after silence has no tracked period, so restarting the frame is free; an articulation
                 * inside a sustained phrase keeps its period, and restarting there locks on the transition. */
                if(yin_onset_restart > 0 && restart_age >= static_cast<uint32_t>(std::max(yin_restart_holdoff, 0))
                   && !(yin_restart_untracked_only > 0 && period_valid))
                {
                    yin_running = false;
                    yin_wait    = static_cast<uint32_t>(yin_onset_restart);
                    restart_age = 0u;
                }
            }
        }
        onset_cur = tairm::max(onset_cur, onset_fast);
        if(onset_frame == 0u)
        {
            onset_prev  = onset_cur;
            onset_cur   = 0.f;
            onset_frame = onset_frame_len;
        }
        onset_frame--;

        if(!shifting)
        {
            output[i] = x;
            for(auto& g : grains) g.active = false;
            pre_valid = false;   // R3i
            continue;
        }

        if(onset && ola_onsets != 0)
        {
            for(auto& g : grains) if(g.active) g.kill = true;
            _ola_spawn(-1, true);
        }

        int youngest = -1;
        for(int k = 0; k < static_cast<int>(grains.size()); k++)
        {
            const OlaGrain& g = grains[static_cast<size_t>(k)];
            if(g.active && !g.kill && (youngest < 0 || g.age < grains[static_cast<size_t>(youngest)].age)) youngest = k;
        }
        if(ola_presearch > 0 && ola_coarse != 0 && youngest >= 0 && !pre_valid)   // R3i
        {
            const OlaGrain& gy = grains[static_cast<size_t>(youngest)];
            const uint32_t half = gy.len / 2u;
            if(gy.age < half && gy.age + static_cast<uint32_t>(ola_presearch) >= half) _ola_spawn(youngest, false, true);
        }
        const uint32_t hop_div = static_cast<uint32_t>(idsp::min<int>(idsp::max<int>(ola_hop_div, 2), 4));   // R4
        uint32_t hop_at = 0u;
        if(youngest >= 0)
        {
            const OlaGrain& gy = grains[static_cast<size_t>(youngest)];
            /* G1: the hop is len - xf (so the fades of consecutive grains abut); the R4 hop_div still applies to
             * the Hann case, where xf = len/2. */
            hop_at = (ola_xfade_frac >= 0.5f) ? (gy.len / hop_div) : (gy.len - gy.xf);
        }
        if(youngest < 0 || grains[static_cast<size_t>(youngest)].age >= hop_at)
        {
            _ola_spawn(youngest);
        }

        float acc  = 0.f;
        float wsum = 0.f;
        for(auto& g : grains)
        {
            if(!g.active) continue;
            /* G1: equal-power trapezoid -- sin^2 rise over xf, flat, sin^2 fall over xf. xf = len/2 gives the
             * original Hann grain exactly. */
            const float a_f = static_cast<float>(g.age) + 0.5f;
            const float xf_f = static_cast<float>(g.xf);
            float s;
            if(a_f < xf_f)                                   s = std::sin(0.5f * idsp::pi * (a_f / xf_f));
            else if(a_f > static_cast<float>(g.len) - xf_f)  s = std::sin(0.5f * idsp::pi * ((static_cast<float>(g.len) - a_f) / xf_f));
            else                                             s = 1.f;
            if(g.kill)   // R3c
            {
                g.fade -= 1.f / tairm::max(ola_kill_len, 1.f);
                if(g.fade <= 0.f) { g.active = false; continue; }
            }
            const float w  = s * s * g.fade;
            acc  += w * _read(g.h);
            wsum += w;
            _advance(g.h);
            if(++g.age >= g.len) g.active = false;
        }
        output[i] = (wsum > 1.0e-3f) ? (acc / wsum) : acc;
    }

    _pitch_tick();
}

template<> bool ShiftAdapter<Shift_smooth>::set_param(const std::string& name, double value)
{
    const float v = static_cast<float>(value);
    if(name == "default_grain")        { Shift_smooth::default_grain = v;        return true; }
    if(name == "fallback_corr_window") { Shift_smooth::fallback_corr_window = v; return true; }
    if(name == "blind_span_cap")       { Shift_smooth::blind_span_cap = v;       return true; }
    if(name == "onset_runway")         { Shift_smooth::onset_runway = v;         return true; }
    if(name == "onset_grain")          { Shift_smooth::onset_grain = v;          return true; }
    if(name == "guard_samples")        { Shift_smooth::guard_samples = std::max(v, 17.f); return true; }
    if(name == "min_grain")            { Shift_smooth::min_grain = v;            return true; }
    if(name == "yin_per_block")        { Shift_smooth::yin_per_block = static_cast<size_t>(std::max(v, 1.f)); return true; }
    if(name == "max_corr_window")      { Shift_smooth::max_corr_window = v;      return true; }
    if(name == "tracked_corr_min")     { Shift_smooth::tracked_corr_min = v;     return true; }
    if(name == "exact_ratio")          { Shift_smooth::exact_ratio = v > 0.5f;   return true; }
    if(name == "causal_corr")          { Shift_smooth::causal_corr = v > 0.5f;   return true; }
    if(name == "period_median3")       { Shift_smooth::period_median3 = v > 0.5f;  return true; }
    if(name == "period_jump")          { Shift_smooth::period_jump = v;            return true; }
    if(name == "period_alpha")         { Shift_smooth::period_alpha = v;           return true; }
    if(name == "period_slew")          { Shift_smooth::period_slew = v;            return true; }
    if(name == "subsample_refine")     { Shift_smooth::subsample_refine = v > 0.5f; return true; }
    if(name == "xfade_min")            { Shift_smooth::xfade_min = v;              return true; }
    if(name == "onset_reseat")         { onset_reseat = v > 0.5f;                  return true; }
    if(name == "onset_ratio")          { onset_ratio = v;                          return true; }
    if(name == "ola_coarse_cands")     { Shift_smooth::ola_coarse_cands = static_cast<int>(v + 0.5f); return true; }
    if(name == "ola_fine_stride")      { Shift_smooth::ola_fine_stride = static_cast<int>(v + 0.5f); return true; }
    if(name == "ola_presearch")        { Shift_smooth::ola_presearch = static_cast<int>(v + 0.5f); return true; }
    if(name == "yin_max_lag")
    {
        const size_t lag = std::clamp(static_cast<size_t>(std::max(v, 0.f) + 0.5f), size_t{64}, Shift_smooth::yin_max_lag_cap);
        Shift_smooth::yin_max_lag  = lag;
        Shift_smooth::yin_analysis = Shift_smooth::yin_window + lag;
        Shift_smooth::max_period   = static_cast<float>(lag * Shift_smooth::dec_factor);
        return true;
    }
    if(name == "yin_restart_untracked_only") { Shift_smooth::yin_restart_untracked_only = static_cast<int>(v + 0.5f); return true; }
    if(name == "yin_burst_snap")       { Shift_smooth::yin_burst_snap = static_cast<int>(v + 0.5f); return true; }
    if(name == "yin_restart_holdoff")  { Shift_smooth::yin_restart_holdoff = static_cast<int>(v + 0.5f); return true; }
    if(name == "yin_burst_confirm")    { Shift_smooth::yin_burst_confirm = static_cast<int>(v + 0.5f); return true; }
    if(name == "yin_burst")            { Shift_smooth::yin_burst = static_cast<int>(v + 0.5f); return true; }
    if(name == "yin_burst_frames")     { Shift_smooth::yin_burst_frames = static_cast<int>(v + 0.5f); return true; }
    if(name == "yin_onset_restart")    { Shift_smooth::yin_onset_restart = static_cast<int>(v + 0.5f); return true; }
    if(name == "yin_hold_frames")      { Shift_smooth::yin_hold_frames = static_cast<uint32_t>(std::max(v, 0.f) + 0.5f); return true; }
    if(name == "yin_weak_hold")        { Shift_smooth::yin_weak_hold = v; return true; }
    if(name == "yin_weak_tol")         { Shift_smooth::yin_weak_tol = v; return true; }
    if(name == "yin_weak_max_lag")     { Shift_smooth::yin_weak_max_lag = static_cast<int>(v + 0.5f); return true; }
    if(name == "yin_weak_stable")      { Shift_smooth::yin_weak_stable = v; return true; }
    if(name == "yin_weak_threshold")   { Shift_smooth::yin_weak_threshold = v; return true; }
    if(name == "ola_coarse")           { Shift_smooth::ola_coarse = static_cast<int>(v + 0.5f); return true; }
    if(name == "ola_fine_reach")       { Shift_smooth::ola_fine_reach = static_cast<int>(v + 0.5f); return true; }
    if(name == "ola_fine_window")      { Shift_smooth::ola_fine_window = static_cast<int>(v + 0.5f); return true; }
    if(name == "ola_onsets")           { Shift_smooth::ola_onsets = static_cast<int>(v + 0.5f); return true; }
    if(name == "ola_kill_len")         { Shift_smooth::ola_kill_len = v;           return true; }
    if(name == "ola_mode")             { Shift_smooth::ola_mode = static_cast<int>(v + 0.5f); return true; }
    if(name == "yin_octave_tol")       { Shift_smooth::yin_octave_tol = v;        return true; }
    if(name == "ola_xfade_frac")       { Shift_smooth::ola_xfade_frac = v;        return true; }
    if(name == "ola_hop_div")          { Shift_smooth::ola_hop_div = static_cast<int>(v + 0.5f); return true; }
    if(name == "ola_periods")          { Shift_smooth::ola_periods = v;            return true; }
    if(name == "ola_min_len")          { Shift_smooth::ola_min_len = v;            return true; }
    if(name == "ola_max_len")          { Shift_smooth::ola_max_len = v;            return true; }
    if(name == "ola_lag_extra")        { Shift_smooth::ola_lag_extra = v;          return true; }
    if(name == "ola_reach")            { Shift_smooth::ola_reach = static_cast<int>(v + 0.5f); return true; }
    if(name == "ola_window")           { Shift_smooth::ola_window = static_cast<int>(v + 0.5f); return true; }
    if(name == "ola_fallback_period")  { Shift_smooth::ola_fallback_period = v;    return true; }
    if(name == "corr_preemph")         { Shift_smooth::corr_preemph = v;           return true; }
    if(name == "fine_reach_min")       { Shift_smooth::fine_reach_min = static_cast<int>(v + 0.5f); return true; }
    if(name == "xfade_law")            { Shift_smooth::xfade_law = static_cast<int>(v + 0.5f); return true; }
    if(name == "fine_corr_window")     { Shift_smooth::fine_corr_window = v;       return true; }
    if(name == "xfade_frac")           { Shift_smooth::xfade_frac = v;          return true; }
    return false;
}

#include "variant.hpp"
static RegisterShifter reg_smooth{"smooth", [] { return std::make_unique<ShiftAdapter<Shift_smooth>>(); }};
