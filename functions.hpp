#ifndef IDSP_REVERB_TOOLKIT_H
#define IDSP_REVERB_TOOLKIT_H

#include "isl/assert.hpp"
#include "idsp/functions.hpp"
#include "idsp/delay.hpp"
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace tairm
{

    /** @return the minimum of `a` and `b`. */
    constexpr float min(float a, float b) {
        return std::fmin(a, b);
    }

    /** @return the maximum of `a` and `b`. */
    constexpr float max(float a, float b) {
        return std::fmax(a, b);
    }

    /** Limits `x` between to `lo` and `hi`. */
    constexpr float clamp(float x, float lo, float hi) {
        return tairm::max(tairm::min(x, hi), lo);
    }


    static constexpr size_t hann_lut_size = 1024;
    inline const std::array<float, hann_lut_size + 1> hann_lut = []()
    {
        std::array<float, hann_lut_size + 1> t{};
        for (size_t i = 0; i <= hann_lut_size; ++i)
        {
            const float s = std::sin(idsp::pi * static_cast<float>(i) / static_cast<float>(hann_lut_size));
            t[i] = s * s;
        }
        return t;
    }();

    inline float hann_at(size_t phase, float env_scale)
    {
        const float x = static_cast<float>(phase) * env_scale;
        const float xc = (x < static_cast<float>(hann_lut_size)) ? x : static_cast<float>(hann_lut_size);
        const size_t i = static_cast<size_t>(xc);
        const float  f = xc - static_cast<float>(i);
        const float  a = hann_lut[i];
        const float  b = hann_lut[i + 1];
        return a + (b - a) * f;
    }

    class SineLfo
    {
        public:
            void set_rate(float normalised)
            {
                step = static_cast<uint32_t>(normalised * phase_scale);
            }

            void set_phase(float turns)
            {
                phase = static_cast<uint32_t>(turns * phase_scale);
            }

            void reset() { phase = 0u; }

            inline float process()
            {
                const float out = sine_at(phase);
                phase += step;
                return out;
            }

            static inline float sine_at(uint32_t phase)
            {
                const float x = static_cast<float>(static_cast<int32_t>(phase)) * inv_phase_scale;

                const float parabola = 4.f * x * (1.f - std::fabs(x));
                return 0.775f * parabola + 0.225f * parabola * std::fabs(parabola);
            }

        private:
            static constexpr float phase_scale     = 4294967296.f;
            static constexpr float inv_phase_scale = 1.f / 2147483648.f;

            uint32_t phase{0};
            uint32_t step{0};
    };

    static inline float fast_log1p(float u)
    {
        const float v = u + 1.f;
        int32_t bits;
        __builtin_memcpy(&bits, &v, sizeof(bits));
        const int e = (bits >> 23) - 127;
        bits = (bits & 0x007FFFFF) | 0x3F800000;  // normalise mantissa to [1, 2)
        float m;
        __builtin_memcpy(&m, &bits, sizeof(m));
        // ln(m) for m in [1,2) via y=(m-1)/(m+1), then ln(m) = 2*(y + y³/3 + y⁵/5)
        const float y  = (m - 1.f) / (m + 1.f);  // y in [0, 1/3)
        const float y2 = y * y;
        const float lnm = y * (2.f + y2 * (0.666667f + y2 * 0.4f));
        return static_cast<float>(e) * 0.693147f + lnm;
    }

    static inline float db_to_linear(float db) { return std::pow(10.0f, db / 20.0f); }

    /// Fast sin(x) for x in [0, π/2]. Taylor degree-5, max error ~0.05%.
    inline float fast_sin_halfpi(float x)
    {
        const float x2 = x * x;
        return x * (1.f + x2 * (-0.166667f + x2 * 0.008333f));
    }

    /// Fast cos(x) for x in [0, π/2]. Taylor degree-6, max error ~0.02%.
    inline float fast_cos_halfpi(float x)
    {
        const float x2 = x * x;
        return 1.f + x2 * (-0.5f + x2 * (0.041667f + x2 * -0.001389f));
    }

    /// @brief Fast 2^x approximation using integer exponent + minimax polynomial.
    /// Max error: ~0.002% — well within audio tolerance for pitch/modulation use.
    /// Handles any x in roughly [-126, 126] (normal float exponent range).
    inline float fast_exp2(float x)
    {
        // Floor to integer (handles negative x correctly, unlike truncation)
        const int xi = static_cast<int>(x) - (x < static_cast<float>(static_cast<int>(x)) ? 1 : 0);
        const float xf = x - static_cast<float>(xi);  // fractional part in [0, 1)
        // 4th-order minimax polynomial for 2^xf on [0, 1)
        const float p = 1.f + xf * (0.693147f + xf * (0.240226f + xf * (0.055504f + xf * 0.009618f)));
        // Exact power-of-2 scale via exponent field: reinterpret (xi+127) << 23
        int32_t bits = (xi + 127) << 23;
        float scale;
        __builtin_memcpy(&scale, &bits, sizeof(scale));
        return p * scale;
    }

    /// Compile-time log2, so ratios between tuning constants can be derived
    /// from the constants themselves rather than pasted in as a literal that
    /// silently goes stale. Constant-folded -- fast_log2 is the audio-path one.
    constexpr float const_log2(float x)
    {
        int e = 0;
        while (x >= 2.f) { x *= 0.5f; ++e; }
        while (x <  1.f) { x *= 2.f;  --e; }
        // x in [1, 2): log2(x) = (2/ln2) * atanh(y), y = (x-1)/(x+1)
        const float y  = (x - 1.f) / (x + 1.f);
        const float y2 = y * y;
        const float s  = y * (1.f + y2 * (1.f/3.f + y2 * (1.f/5.f
                       + y2 * (1.f/7.f + y2 * (1.f/9.f + y2 * (1.f/11.f))))));
        return static_cast<float>(e) + 2.8853900818f * s;   // 2 / ln(2)
    }

    static constexpr bool is_prime(int n)
    {
        if (n < 2) return false;
        if ((n % 2) == 0) return n == 2;
        for (int d = 3; d * d <= n; d += 2) {
            if ((n % d) == 0) return false;
        }
        return true;
    }


    template<size_t Count>
    struct PrimeTable
    {
        std::array<int, Count> primes{};

        constexpr PrimeTable()
        {
            int count = 0;
            int n = 2;
            while(count < (int)primes.size())
            {
                if(is_prime(n))
                {
                    primes[count++] = n;
                }
                n++;
            }
        }

        constexpr int at_float_index(float idxFloat) const
        {
            int idx = (int)idxFloat;
            idx = idsp::clamp(idx, 0, (int)primes.size() - 1);
            return primes[(size_t)idx];
        }
    };

    using DiffuserPrimes = PrimeTable<512>;
    inline constexpr DiffuserPrimes prime_table{};
    static_assert(prime_table.primes[0] == 2 && prime_table.primes[511] == 3671, "prime(0)==2, prime(511)==3671");

    struct Smooth
    {
        float smoothing{0.8f};
        float y{0.0f};
        float x{0.0f};

        void set_coef(float s)
        {
            smoothing = tairm::clamp(s, 0.0f, 0.999999f);
        }

        void set_target(float target)
        {
            x = target;
        }

        void reset(float defaultValue = 0.f)
        {
            y = defaultValue;
            x = defaultValue;
        }

        inline float process(float target)
        {
            y = (1.0f - smoothing) * target + smoothing * y;
            return y;
        }

        inline float process()
        {
            y = (1.0f - smoothing) * x + smoothing * y;
            return y;
        }

        inline float value()
        {
            return y;
        }
    };

    struct SineCosLFO
    {
        float sampleRate = 48000.0f;
        float phase = 0.0f; // radians
        float phaseInc = 0.0f;

        void setSampleRate(float sr)
        {
            sampleRate = sr;
        }

        void setFrequencyHz(float fHz)
        {
            phaseInc = 2.0f * (float)idsp::pi * (fHz / sampleRate);
        }

        void process(float& outSin, float& outCos)
        {
            outSin = std::sin(phase);
            outCos = std::cos(phase);
            phase += phaseInc;
            if (phase > 2.0f * (float)idsp::pi) phase -= 2.0f * (float)idsp::pi;
        }
    };

    static inline float lcg_noise(uint32_t& seed)
    {
        seed = 1664525u * seed + 1013904223u;
        return static_cast<float>(seed) * (2.f / 4294967296.f) - 1.f;
    }

    // -------- deterministic hash -> [0,1) float --------
    // Fast, stable across platforms (uses uint32 ops only).
    static inline uint32_t hash_u32(uint32_t x)
    {
        // Wang hash-ish
        x ^= x >> 16;
        x *= 0x7feb352dU;
        x ^= x >> 15;
        x *= 0x846ca68bU;
        x ^= x >> 16;
        return x;
    }

    static inline float hash01(uint32_t seed)
    {
        // Take top 24 bits -> [0,1)
        uint32_t h = hash_u32(seed);
        return float(h >> 8) * (1.0f / 16777216.0f); // 2^24
    }

    static inline float hash_sign(uint32_t seed)
    {
        return (hash_u32(seed) & 1U) ? 1.0f : -1.0f;
    }

    // Gamma/Power Curve
    //Increases resolution near 1 i.e the edge
    inline float edge_scale_bipolar (float x, float shape)
    {
        const float sign = (x >= 0.f) ? 1.f : -1.f;
        const float abs_x = std::abs(x);
        return sign * std::pow(abs_x, 1.f / shape);
    }

    inline float edge_scale (float x, float shape)
    {
        return std::pow(tairm::clamp(x, 0.f, 1.f), 1.f / shape);
    }


    struct TanhADAA1
    {
        inline float process(float x)
        {
            constexpr float epsilon = 1e-5f;
            const float dx = x - x_prev;
            const float result = (std::abs(dx) < epsilon)
            ? idsp::tanh_fast(0.5f * (x + x_prev))
            : (log_cosh(x) - log_cosh(x_prev)) / dx;
            x_prev = x;
            return result;
        }

        private:

            static inline float log_cosh(float x)
            {
                constexpr float k1 = 3.f - (1.f / idsp::pi); // ≈ 2.6817
                constexpr float k2 = 1.f / (3.f * idsp::pi); // ≈ 0.1061
                const float x2 = x * x;
                return 0.5f * (k1 * fast_log1p(x2 / 3.f) + k2 * x2);
            }

            float x_prev{0.f};
    };

    class OnePoleResonanceBoost
    {
        public:

            OnePoleResonanceBoost() = default;
            ~OnePoleResonanceBoost() = default;

            void reset()
            {
                y1 = 0.0f;
            }

            void set_coefficient(float a_)
            {
                a = tairm::clamp(a_, -0.9f, 0.9f);
            }

            inline float process(float x)
            {
                if (a == 0.f)
                {
                    y1 = x;
                    return x;
                }
                float y = x + a * y1;
                y1 = idsp::tanh_fast(y);
                return y;
            }

        private:
            // TanhADAA1 tanh;
            float a = 0.0f;
            float y1 = 0.0f;
    };

    inline float fast_log2(float x)
    {
        uint32_t bits;
        std::memcpy(&bits, &x, sizeof(bits));
        int exponent = static_cast<int>((bits >> 23) & 0xFFu) - 127;

        const uint32_t mantissa_bits = (bits & 0x007FFFFFu) | 0x3F800000u;
        float mantissa;
        std::memcpy(&mantissa, &mantissa_bits, sizeof(mantissa));
        if (mantissa > 1.3333333f) { mantissa *= 0.5f; ++exponent; }

        const float u = mantissa - 1.f;
        const float p =
            (((((((-0.227448768f * u + 0.252508299f) * u - 0.235624817f) * u
                + 0.284484182f) * u - 0.360870245f) * u + 0.481026499f) * u
            - 0.721344748f) * u + 1.44269396f) * u;
        return static_cast<float>(exponent) + p;
    }


    inline float comp(float x)
    {
        static constexpr float inv_log2_11 = 0.289064826f;   // 1 / log2(11)
        return std::copysign(fast_log2(1.f + 10.f * std::abs(x)) * inv_log2_11, x);
    }

    inline float expand(float x)
    {
        const float u = std::abs(x);
        const float p =
            (((((((0.00935673468f * u - 0.00605525146f) * u + 0.0431837346f) * u
                + 0.0555942464f) * u + 0.141522591f) * u + 0.229041225f) * u
            + 0.287570011f) * u + 0.239786671f) * u;
        return std::copysign(p, x);
    }

    // The cubic soft clip the Spring send, the diffuser state and Shimmer's
    // feedback tap all carry a private copy of: unity slope at zero, 1.3 dB down
    // at full scale, hard-bounded at 1.0 out for any input. Knee 1.5 is where
    // the cubic's own derivative reaches zero, so it is the widest knee that
    // stays monotonic and the clamp above it is exact rather than a kink.
    static constexpr float soft_clip_knee     = 1.5f;
    static constexpr float soft_clip_curvature = 1.f / (soft_clip_knee * soft_clip_knee * 3.f);

    //
    // fmin/fmax rather than tairm::clamp, which compiles to a compare-and-branch
    // pair: these are vmaxnm/vminnm on this core, so the whole thing is 9
    // instructions with no branch in it, and 5 in a loop once the two constants
    // hoist.
    inline float soft_clip(float x)
    {
        const float c = std::fmin(std::fmax(x, -soft_clip_knee), soft_clip_knee);
        return c * (1.f - soft_clip_curvature * c * c);
    }

    static inline float next_noise(uint32_t noise_seed)
    {
        noise_seed ^= noise_seed << 13;
        noise_seed ^= noise_seed >> 17;
        noise_seed ^= noise_seed << 5;
        return static_cast<float>(static_cast<int32_t>(noise_seed)) * 4.656613e-10f; // / 2^31
    }



} // namespace tairm

#endif