#include "env_match2.hpp"
#include <cmath>

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
    static constexpr float onset_ratio     = 2.5f;
    static constexpr float onset_floor     = 1.0e-4f;
    static constexpr uint32_t onset_refractory = 1440u;  // 30 ms
    static constexpr float onset_fade      = 64.f;       // output samples
    static constexpr float onset_runway    = 1200.f;     // 25 ms of splice-free attack

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

void Shift_env_match2::reset()
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

void Shift_env_match2::_seek(Head& h, float lag)
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

void Shift_env_match2::_write(float x)
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

float Shift_env_match2::_read(const Head& h) const
{
    const float    ph = h.frac * static_cast<float>(shift_env_match2::sinc_phases);
    uint32_t       p  = static_cast<uint32_t>(ph);
    if(p >= shift_env_match2::sinc_phases) p = shift_env_match2::sinc_phases - 1u;
    const float    pf = ph - static_cast<float>(p);

    const float* k0 = shift_env_match2::sinc_table[p].data();
    const float* k1 = shift_env_match2::sinc_table[p + 1u].data();

    const uint32_t base = h.pos - static_cast<uint32_t>(shift_env_match2::sinc_centre);

    float acc = 0.f;
    for(size_t j = 0; j < shift_env_match2::sinc_taps; j++)
    {
        const float k = k0[j] + pf * (k1[j] - k0[j]);
        acc += k * history[(base + static_cast<uint32_t>(j)) & history_mask];
    }
    return acc;
}

void Shift_env_match2::_update_interval()
{
    const int16_t semis = static_cast<int16_t>(idsp::clamp<int32_t>(controls.shift_amount, -12, 12));
    if(semis == interval) return;

    interval = semis;
    ratio    = tairm::fast_exp2(static_cast<float>(semis) * (1.f / 12.f));

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

void Shift_env_match2::_update_grain()
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
    xfade = tairm::clamp(0.25f * grain / drift, 32.f, 1024.f);

    /* Two very different jobs share this window. When the period is known the
     * splice distance is already predicted and the correlation only has to
     * refine it, so a short window is enough and its reach is pure latency.
     * When it is not -- a chord, a scrape, anything inharmonic -- the search
     * has no prediction to lean on and the window is the only thing telling it
     * one alignment from another; at that point it has to span more than a
     * period of the lowest content or it is matching noise. Paying for that in
     * latency only on material that actually needs it is the whole point of
     * having the tracker decide. */
    corr_window = period_valid ? tairm::clamp(p, 128.f, max_corr_window)
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
    lag_floor = guard_samples + (0.5f * corr_window);
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

void Shift_env_match2::_pitch_tick()
{
    if(!yin_running)
    {
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
    const size_t end = ((yin_cursor + yin_per_block) < (yin_max_lag + 1))
                     ? (yin_cursor + yin_per_block)
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
    }
}

void Shift_env_match2::_yin_finalise()
{
    /* Reject silence outright rather than letting the normalisation turn noise
     * into a confident wrong answer. */
    float energy = 0.f;
    for(size_t n = 0; n < yin_window; n++) energy += yin_snap[n] * yin_snap[n];
    if(energy < 1.0e-6f * static_cast<float>(yin_window))
    {
        period_hold  = 0u;
        period_valid = false;
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
        return;
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

    /* Track small drifts (bends, vibrato) smoothly so the grain length does not
     * jitter; jump outright on a real note change. */
    if(period_valid && std::fabs(found - period) < (0.06f * period))
    {
        period += 0.25f * (found - period);
    }
    else
    {
        period = found;
    }
    period_valid = true;
    period_hold  = yin_hold_frames;
}

int32_t Shift_env_match2::_splice_coarse(uint32_t ref, int32_t sign)
{
    /* Coarse pass on the 4x decimated line.
     *
     * Tracked, this is only a refinement of a prediction: a narrow sweep either
     * side of one grain, at full decimated resolution. Blind, it degenerates to
     * a plain WSOLA similarity search -- a third of a grain either side, wide
     * enough to reach the common period of a chord. */
    const float   span = period_valid ? tairm::max(period * 0.125f, 12.f) : tairm::min(0.35f * grain, 269.f);   /* E4b: cap blind reach at the original 0.35*768 */
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
    uint32_t rd = head_d - static_cast<uint32_t>(wd / 2);
    if(static_cast<int32_t>(head_d + static_cast<uint32_t>(wd / 2) - dec_abs) > 0)
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

float Shift_env_match2::_splice_fine(uint32_t ref, int32_t sign, int32_t best_dd) const
{
    /* Blind, the grid the coarse pass left behind is 8 decimated samples; that
     * is what sets the reach, and it has to be re-derived here rather than
     * carried, because the tracker can gain or lose lock between the block that
     * ran the coarse pass and the block that fires the splice. Re-deriving it
     * can only widen or narrow the window the peak is hunted in, never move it
     * off the distance the coarse pass chose. */
    const int32_t step  = period_valid ? 1 : 8;
    const int32_t reach = idsp::min<int32_t>(idsp::max<int32_t>(2 * step, 4), max_fine_reach);

    /* Fine pass at full rate, over the half-step the coarse grid could not
     * resolve, and then a parabolic step on the correlation peak that takes the
     * alignment below a sample -- which the read interpolator can actually use,
     * so the two heads end up phase-locked rather than merely close. Resolving
     * a grid step is all this stage does, so it does not need the long window
     * the blind coarse search does. */
    const int32_t w    = static_cast<int32_t>(tairm::min(corr_window, fine_corr_window));
    const int32_t d0   = best_dd * static_cast<int32_t>(dec_factor);
    const uint32_t rbase = ref - static_cast<uint32_t>(w / 2);

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
            const float a = history[(rbase + static_cast<uint32_t>(n)) & history_mask];
            const float b = history[(cp    + static_cast<uint32_t>(n)) & history_mask];
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

    float out = static_cast<float>(d0 + best_i - reach);
    if(best_i > 0 && best_i < (2 * reach))
    {
        const float a = fine[static_cast<size_t>(best_i) - 1u];
        const float b = fine[static_cast<size_t>(best_i)];
        const float c = fine[static_cast<size_t>(best_i) + 1u];
        const float denom = a - (2.f * b) + c;
        if(denom < -1.0e-9f) out += 0.5f * (a - c) / denom;
    }

    return tairm::max(out, 32.f);
}

void Shift_env_match2::_start_fade(float target_lag, float length, bool match_level)
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

void Shift_env_match2::process(const MonoDspBuffer& input, MonoDspBuffer& output)
{
    _update_interval();
    _update_grain();

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
            _start_fade((ratio > 1.f) ? lag_hi : lag_lo, onset_fade);
        }
        else if(onset && shifting)
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
            if(std::fabs(target - lag) > 8.f) _start_fade(target, onset_fade);
        }
        else if(shifting && !fading)
        {
            const bool due = (ratio > 1.f) ? (lag <= lag_lo) : (lag >= lag_hi);

            if(due && splice_armed && !searched)
            {
                const float d = _splice_fine(head[active].pos, sign, splice_dd);
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
            out = ((1.f - g) * head_gain[active ^ 1u] * _read(outgoing)) + (g * head_gain[active] * _read(head[active]));

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

#include "variant.hpp"
static RegisterShifter reg_env_match2{"env_match2", [] { return std::make_unique<ShiftAdapter<Shift_env_match2>>(); }};
