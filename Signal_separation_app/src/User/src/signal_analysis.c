/******************************************************************************
 * signal_analysis.c
 *
 * The FFT supplies candidate frequencies. A joint time-domain residual then
 * selects the independent A/B waveform types from the four valid combinations.
 ******************************************************************************/

#include "../include/app_config.h"
#include "../include/signal_processing.h"

#include <math.h>

typedef struct {
    int bin;
    float32_t magnitude;
} signal_peak_t;

static float32_t signal_triangle_sample(float32_t phase)
{
    float32_t value = 0.0f;
    int harmonic;
    int harmonic_index = 0;

    for (harmonic = 1; harmonic <= APP_TRI_MAX_HARMONIC; harmonic += 2) {
        float32_t sign = (harmonic_index & 1) ? -1.0f : 1.0f;

        value += sign * sinf((float32_t)harmonic * phase) /
                 ((float32_t)harmonic * (float32_t)harmonic);
        harmonic_index++;
    }
    return value;
}

static float32_t signal_component_sample(const signal_component_t *component,
                                         int sample_index)
{
    float32_t phase = component->measured_phase_rad + 2.0f * APP_PI *
        component->frequency_hz * (float32_t)sample_index /
        APP_SAMPLE_RATE_HZ;

    if (component->waveform == SIGNAL_WAVE_TRIANGLE) {
        return component->fundamental_amplitude * signal_triangle_sample(phase);
    }
    return component->fundamental_amplitude * sinf(phase);
}

static void signal_prepare_adc_frame(const u16 *raw_samples,
                                     float32_t *time_domain,
                                     float32_t *fft_input)
{
    float32_t sum = 0.0f;
    float32_t mean;
    int index;

    for (index = 0; index < APP_FFT_LEN; index++) {
        time_domain[index] = (float32_t)(raw_samples[index] >> 4);
        sum += time_domain[index];
    }

    mean = sum / (float32_t)APP_FFT_LEN;
    for (index = 0; index < APP_FFT_LEN; index++) {
        float32_t hann = 0.5f - 0.5f * cosf(2.0f * APP_PI *
            (float32_t)index / (float32_t)(APP_FFT_LEN - 1));

        time_domain[index] -= mean;
        fft_input[index] = time_domain[index] * hann;
    }
}

static void signal_compute_magnitudes(const float32_t *spectrum,
                                      float32_t *magnitude)
{
    int bin;

    magnitude[0] = fabsf(spectrum[0]);
    for (bin = 1; bin < APP_SPEC_LEN; bin++) {
        float32_t real = spectrum[2 * bin];
        float32_t imaginary = spectrum[2 * bin + 1];

        magnitude[bin] = sqrtf(real * real + imaginary * imaginary);
    }
}

static void signal_insert_peak(signal_peak_t *peaks, int *peak_count,
                               int bin, float32_t magnitude)
{
    int index;
    int limit = *peak_count;

    if (limit < APP_PEAK_CANDIDATE_COUNT) {
        peaks[limit].bin = bin;
        peaks[limit].magnitude = magnitude;
        (*peak_count)++;
    } else if (magnitude <= peaks[limit - 1].magnitude) {
        return;
    } else {
        peaks[limit - 1].bin = bin;
        peaks[limit - 1].magnitude = magnitude;
    }

    for (index = *peak_count - 1; index > 0; index--) {
        signal_peak_t temporary;

        if (peaks[index].magnitude <= peaks[index - 1].magnitude) {
            break;
        }
        temporary = peaks[index];
        peaks[index] = peaks[index - 1];
        peaks[index - 1] = temporary;
    }
}

static int signal_find_candidate_peaks(const float32_t *magnitude,
                                       signal_peak_t *peaks)
{
    int bin;
    int peak_count = 0;
    int first_bin = (int)(APP_SIGNAL_MIN_HZ / APP_BIN_WIDTH_HZ);
    int last_bin = (int)(APP_SIGNAL_MAX_HZ / APP_BIN_WIDTH_HZ) + 1;

    if (first_bin < 1) {
        first_bin = 1;
    }
    if (last_bin >= APP_SPEC_LEN - 1) {
        last_bin = APP_SPEC_LEN - 2;
    }

    for (bin = first_bin; bin <= last_bin; bin++) {
        if (magnitude[bin] > magnitude[bin - 1] &&
            magnitude[bin] >= magnitude[bin + 1]) {
            signal_insert_peak(peaks, &peak_count, bin, magnitude[bin]);
        }
    }
    return peak_count;
}

static float32_t signal_refine_frequency(const float32_t *magnitude, int bin)
{
    float32_t left = magnitude[bin - 1];
    float32_t center = magnitude[bin];
    float32_t right = magnitude[bin + 1];
    float32_t denominator = left - 2.0f * center + right;
    float32_t offset = 0.0f;

    if (fabsf(denominator) > 1.0e-12f) {
        offset = 0.5f * (left - right) / denominator;
        if (offset > 0.5f) {
            offset = 0.5f;
        } else if (offset < -0.5f) {
            offset = -0.5f;
        }
    }
    return ((float32_t)bin + offset) * APP_BIN_WIDTH_HZ;
}

static void signal_estimate_fundamental(const float32_t *time_domain,
                                        float32_t frequency_hz,
                                        signal_waveform_t waveform,
                                        signal_component_t *component)
{
    float32_t sin_sum = 0.0f;
    float32_t cos_sum = 0.0f;
    float32_t phase_step = 2.0f * APP_PI * frequency_hz /
                               APP_SAMPLE_RATE_HZ;
    int index;

    for (index = 0; index < APP_FFT_LEN; index++) {
        float32_t phase = phase_step * (float32_t)index;

        sin_sum += time_domain[index] * sinf(phase);
        cos_sum += time_domain[index] * cosf(phase);
    }

    component->frequency_hz = frequency_hz;
    component->fundamental_amplitude = 2.0f * sqrtf(
        sin_sum * sin_sum + cos_sum * cos_sum) / (float32_t)APP_FFT_LEN;
    component->measured_phase_rad = atan2f(cos_sum, sin_sum);
    component->waveform = waveform;
}

static float32_t signal_joint_residual(const float32_t *time_domain,
                                       const signal_component_t *channel_a,
                                       const signal_component_t *channel_b,
                                       float32_t *model_workspace)
{
    float32_t signal_energy = 0.0f;
    float32_t error_energy = 0.0f;
    int index;

    for (index = 0; index < APP_FFT_LEN; index++) {
        float32_t error;

        model_workspace[index] = signal_component_sample(channel_a, index) +
                                 signal_component_sample(channel_b, index);
        error = time_domain[index] - model_workspace[index];
        signal_energy += time_domain[index] * time_domain[index];
        error_energy += error * error;
    }

    if (signal_energy <= 1.0e-12f) {
        return 1.0e9f;
    }
    return sqrtf(error_energy / signal_energy);
}

static void signal_refine_pair(const float32_t *time_domain,
                               signal_component_t *channel_a,
                               signal_component_t *channel_b,
                               float32_t *workspace)
{
    int iteration;
    int index;

    /*
     * Alternate two residual projections. This separates an A triangle third
     * harmonic from a B fundamental when fB = 3*fA before final scoring.
     */
    for (iteration = 0; iteration < 2; iteration++) {
        for (index = 0; index < APP_FFT_LEN; index++) {
            workspace[index] = time_domain[index] -
                signal_component_sample(channel_b, index);
        }
        signal_estimate_fundamental(workspace, channel_a->frequency_hz,
                                    channel_a->waveform, channel_a);

        for (index = 0; index < APP_FFT_LEN; index++) {
            workspace[index] = time_domain[index] -
                signal_component_sample(channel_a, index);
        }
        signal_estimate_fundamental(workspace, channel_b->frequency_hz,
                                    channel_b->waveform, channel_b);
    }
}

int signal_analyze_frame(const u16 *raw_samples,
                         arm_rfft_fast_instance_f32 *fft_instance,
                         float32_t *time_domain,
                         float32_t *fft_input,
                         float32_t *fft_spectrum,
                         float32_t *fft_magnitude,
                         float32_t *model_workspace,
                         signal_analysis_result_t *result)
{
    signal_peak_t peaks[APP_PEAK_CANDIDATE_COUNT];
    float32_t best_residual = 1.0e9f;
    int peak_count;
    int first;
    int second;
    int waveform_a;
    int waveform_b;

    if (raw_samples == 0 || fft_instance == 0 || time_domain == 0 ||
        fft_input == 0 || fft_spectrum == 0 || fft_magnitude == 0 ||
        model_workspace == 0 || result == 0) {
        return XST_FAILURE;
    }

    signal_prepare_adc_frame(raw_samples, time_domain, fft_input);
    arm_rfft_fast_f32(fft_instance, fft_input, fft_spectrum, 0);
    signal_compute_magnitudes(fft_spectrum, fft_magnitude);
    peak_count = signal_find_candidate_peaks(fft_magnitude, peaks);
    if (peak_count < 2) {
        return XST_FAILURE;
    }

    for (first = 0; first < peak_count - 1; first++) {
        for (second = first + 1; second < peak_count; second++) {
            float32_t frequency_one = signal_refine_frequency(
                fft_magnitude, peaks[first].bin);
            float32_t frequency_two = signal_refine_frequency(
                fft_magnitude, peaks[second].bin);
            float32_t frequency_a = frequency_one;
            float32_t frequency_b = frequency_two;

            if (frequency_a > frequency_b) {
                float32_t temporary = frequency_a;
                frequency_a = frequency_b;
                frequency_b = temporary;
            }
            if (frequency_b - frequency_a < APP_MIN_COMPONENT_GAP_HZ) {
                continue;
            }

            for (waveform_a = SIGNAL_WAVE_SINE;
                 waveform_a <= SIGNAL_WAVE_TRIANGLE; waveform_a++) {
                for (waveform_b = SIGNAL_WAVE_SINE;
                     waveform_b <= SIGNAL_WAVE_TRIANGLE; waveform_b++) {
                    signal_component_t candidate_a;
                    signal_component_t candidate_b;
                    float32_t residual;

                    signal_estimate_fundamental(time_domain, frequency_a,
                                                (signal_waveform_t)waveform_a,
                                                &candidate_a);
                    signal_estimate_fundamental(time_domain, frequency_b,
                                                (signal_waveform_t)waveform_b,
                                                &candidate_b);
                    signal_refine_pair(time_domain, &candidate_a, &candidate_b,
                                       model_workspace);
                    residual = signal_joint_residual(time_domain,
                                                      &candidate_a,
                                                      &candidate_b,
                                                      model_workspace);
                    if (residual < best_residual) {
                        best_residual = residual;
                        result->channel_a = candidate_a;
                        result->channel_b = candidate_b;
                    }
                }
            }
        }
    }

    if (best_residual >= 1.0e8f) {
        return XST_FAILURE;
    }
    result->normalized_residual = best_residual;
    return XST_SUCCESS;
}

void signal_track_result(signal_analysis_result_t *tracked,
                         const signal_analysis_result_t *measurement,
                         float32_t frequency_alpha)
{
    signal_component_t *tracked_channels[2];
    const signal_component_t *measured_channels[2];
    int index;

    if (tracked == 0 || measurement == 0) {
        return;
    }
    if (frequency_alpha < 0.0f) {
        frequency_alpha = 0.0f;
    } else if (frequency_alpha > 1.0f) {
        frequency_alpha = 1.0f;
    }

    tracked_channels[0] = &tracked->channel_a;
    tracked_channels[1] = &tracked->channel_b;
    measured_channels[0] = &measurement->channel_a;
    measured_channels[1] = &measurement->channel_b;
    for (index = 0; index < 2; index++) {
        tracked_channels[index]->frequency_hz += frequency_alpha *
            (measured_channels[index]->frequency_hz -
             tracked_channels[index]->frequency_hz);
        tracked_channels[index]->fundamental_amplitude =
            measured_channels[index]->fundamental_amplitude;
        tracked_channels[index]->measured_phase_rad =
            measured_channels[index]->measured_phase_rad;
        tracked_channels[index]->waveform = measured_channels[index]->waveform;
    }
    tracked->normalized_residual = measurement->normalized_residual;
}

const char *signal_waveform_name(signal_waveform_t waveform)
{
    return (waveform == SIGNAL_WAVE_TRIANGLE) ? "triangle" : "sine";
}

static void signal_generate_test_frame(u16 *raw_samples,
                                       const signal_component_t *channel_a,
                                       const signal_component_t *channel_b)
{
    int index;

    for (index = 0; index < APP_FFT_LEN; index++) {
        int code = (int)(2048.0f + signal_component_sample(channel_a, index) +
                         signal_component_sample(channel_b, index) + 0.5f);

        if (code < 0) {
            code = 0;
        } else if (code > 4095) {
            code = 4095;
        }
        raw_samples[index] = (u16)(code << 4);
    }
}

static int signal_run_test_case(signal_waveform_t waveform_a,
                                signal_waveform_t waveform_b,
                                float32_t frequency_a,
                                float32_t frequency_b)
{
    static u16 raw_samples[APP_FFT_LEN];
    static float32_t time_domain[APP_FFT_LEN];
    static float32_t fft_input[APP_FFT_LEN];
    static float32_t fft_spectrum[APP_FFT_LEN];
    static float32_t fft_magnitude[APP_SPEC_LEN];
    static float32_t model_workspace[APP_FFT_LEN];
    arm_rfft_fast_instance_f32 fft_instance;
    signal_component_t channel_a;
    signal_component_t channel_b;
    signal_analysis_result_t result;

    if (arm_rfft_fast_init_f32(&fft_instance, APP_FFT_LEN) !=
        ARM_MATH_SUCCESS) {
        return XST_FAILURE;
    }

    channel_a.frequency_hz = frequency_a;
    channel_a.fundamental_amplitude = 220.0f;
    channel_a.measured_phase_rad = 0.20f;
    channel_a.waveform = waveform_a;
    channel_b.frequency_hz = frequency_b;
    channel_b.fundamental_amplitude = 190.0f;
    channel_b.measured_phase_rad = -0.35f;
    channel_b.waveform = waveform_b;
    signal_generate_test_frame(raw_samples, &channel_a, &channel_b);

    if (signal_analyze_frame(raw_samples, &fft_instance, time_domain,
                             fft_input, fft_spectrum, fft_magnitude,
                             model_workspace, &result) != XST_SUCCESS) {
        return XST_FAILURE;
    }
    if (result.channel_a.waveform != waveform_a ||
        result.channel_b.waveform != waveform_b ||
        fabsf(result.channel_a.frequency_hz - frequency_a) > 750.0f ||
        fabsf(result.channel_b.frequency_hz - frequency_b) > 750.0f ||
        result.normalized_residual > 0.30f) {
        return XST_FAILURE;
    }
    return XST_SUCCESS;
}

int signal_run_self_tests(void)
{
    if (signal_run_test_case(SIGNAL_WAVE_SINE, SIGNAL_WAVE_SINE,
                             50000.0f, 55000.0f) != XST_SUCCESS ||
        signal_run_test_case(SIGNAL_WAVE_SINE, SIGNAL_WAVE_TRIANGLE,
                             50000.0f, 55000.0f) != XST_SUCCESS ||
        signal_run_test_case(SIGNAL_WAVE_TRIANGLE, SIGNAL_WAVE_SINE,
                             50000.0f, 55000.0f) != XST_SUCCESS ||
        signal_run_test_case(SIGNAL_WAVE_TRIANGLE, SIGNAL_WAVE_TRIANGLE,
                             50000.0f, 55000.0f) != XST_SUCCESS ||
        signal_run_test_case(SIGNAL_WAVE_TRIANGLE, SIGNAL_WAVE_SINE,
                             25000.0f, 75000.0f) != XST_SUCCESS) {
        return XST_FAILURE;
    }
    return XST_SUCCESS;
}
