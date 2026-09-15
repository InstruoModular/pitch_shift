#ifndef ENGINE_CONSTANT_HPP
#define ENGINE_CONSTANT_HPP

#include "idsp/buffer_types.hpp"
#include <cstdint>
#include <string>


static constexpr size_t audio_block_size = 32;
static constexpr size_t audio_channels = 4;
static constexpr float hardware_sample_rate = 48000.f;
using AudioBuffer = idsp::PolySampleBufferStatic<audio_block_size, audio_channels>;

/* Per-channel rate of the external CV ADC (ADS8638) on SPI1.
 *
 * This is the oversampling ratio into hardware/control_decimator.hpp: the CV
 * decimator takes spi_block_size samples per channel per block and returns one.
 * Higher is better -- it pushes the one band no digital filter can rescue, the
 * fold around the ADC's own sample rate, further from anything a Eurorack source
 * puts out -- but the pacer can only ask for what the SPI can retire. A PL022
 * frame is 16 SCLK plus a gap, so 48 kHz/ch costs 384 kwords/s and about
 * 6.6 MHz of the 10 MHz baudrate in pin_map.hpp. Going back to 96 kHz/ch needs
 * that baudrate raised to ~14 MHz (the ADS8638 is specified to 17), which is a
 * bench call, not a desk one. */
static constexpr float spi_sample_rate = 48000.f;
static constexpr float adc_sample_rate = 12000.f;
static constexpr size_t spi_block_size =static_cast<size_t>(audio_block_size*(spi_sample_rate/hardware_sample_rate));
static constexpr size_t adc_block_size =static_cast<size_t>(audio_block_size*(adc_sample_rate/hardware_sample_rate));
static constexpr size_t adc_channels = 8;
static constexpr size_t spi_adc_channels = 8;
using SpiBuffer = std::array<std::array<uint16_t, spi_block_size>, spi_adc_channels>;// idsp::PolySampleBufferStatic<spi_block_size, adc_channels>;
using AdcBuffer = std::array<std::array<int16_t, adc_block_size>, spi_adc_channels>;// idsp::PolySampleBufferStatic<spi_block_size, adc_channels>;
using CvBuffer = std::array<float, spi_adc_channels>;

/* One control channel's worth of CV at the *audio* rate.
 *
 * The external converter runs at exactly the codec rate and the pacer puts
 * exactly one half-buffer in front of each audio block, so spi_block_size and
 * audio_block_size are the same number and there is one CV sample per audio
 * sample. No decimation, no resampling, no halfband, nothing to align -- the
 * whole audio-rate CV path is a scale and a pole. The assert is what keeps that
 * true if either rate is ever moved; a mismatch needs a real resampler and this
 * is not it. */
static_assert(spi_block_size == audio_block_size,
              "the audio-rate CV path assumes one CV sample per audio sample");

using CvBlock       = std::array<float, audio_block_size>;
using CvBlockBuffer = std::array<CvBlock, spi_adc_channels>;

/** What an unpatched, or simply unmoving, CV contributes. Every consumer points
 *  at this by default, so nothing has to test a pointer per sample -- and
 *  re-pointing a parameter's tap back at it is how that parameter is rolled
 *  back to block rate. */
inline constexpr CvBlock cv_block_zero{};

/** Audio block size. */
static constexpr size_t dsp_block_size = audio_block_size;

/** Number of DSP channels. */
static constexpr size_t dsp_num_channels = 2;

/** Sample data type. */
#define Sample float

/* Audio sample rate */
static constexpr float sample_rate = hardware_sample_rate;
/* Audio sample rate */
static constexpr float sample_rate_ms =  static_cast<float>(sample_rate)/1000.f;

/** DSP IO buffer type. */
using DspBuffer = idsp::PolySampleBufferStatic<dsp_block_size, dsp_num_channels>;
/** DSP IO buffer type. */
using MonoDspBuffer = idsp::SampleBufferStatic<dsp_block_size>;

static constexpr float norm_sample_rate = 1.f / sample_rate;

static constexpr uint32_t NUM_WS_LEDS = 4;

#endif