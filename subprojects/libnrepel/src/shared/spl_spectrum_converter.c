/*
noise-repellent -- Noise Reduction LV2

Copyright 2021 Luciano Dato <lucianodato@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/
*/

#include "spl_spectrum_converter.h"
#include "../shared/fft_transform.h"
#include "../shared/modules_configurations.h"
#include "../shared/spectral_features.h"
#include "../shared/spectral_utils.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define AT_SINE_WAVE_FREQ 1000.f
#define REFERENCE_LEVEL 90.f
#define S_AMP 1.f

static void generate_sinewave(SplSpectrumConverter *self);
static void compute_spl_reference_spectrum(SplSpectrumConverter *self);

struct SplSpectrumConverter {
  float *sinewave;
  float *window;
  float *spl_reference_values;

  SpectralFeatures *spectral_features;
  FftTransform *fft_transform;

  uint32_t fft_size;
  uint32_t half_fft_size;
  uint32_t sample_rate;
};

SplSpectrumConverter *reference_spectrum_initialize(uint32_t sample_rate) {
  SplSpectrumConverter *self =
      (SplSpectrumConverter *)calloc(1U, sizeof(SplSpectrumConverter));

  self->fft_transform = fft_transform_initialize();

  self->fft_size = get_fft_size(self->fft_transform);
  self->half_fft_size = self->fft_size / 2U;
  self->sample_rate = self->sample_rate;

  self->spl_reference_values =
      (float *)calloc((self->half_fft_size + 1U), sizeof(float));

  self->sinewave = (float *)calloc(self->fft_size, sizeof(float));
  self->window = (float *)calloc(self->fft_size, sizeof(float));

  self->spectral_features =
      spectral_features_initialize(self->half_fft_size + 1U);

  generate_sinewave(self);
  get_fft_window(self->window, self->fft_size, HANN_WINDOW);
  compute_spl_reference_spectrum(self);

  return self;
}

void reference_spectrum_free(SplSpectrumConverter *self) {
  free(self->sinewave);
  free(self->window);
  free(self->spl_reference_values);
  spectral_features_free(self->spectral_features);
}

static void generate_sinewave(SplSpectrumConverter *self) {
  for (uint32_t k = 0; k < self->fft_size; k++) {
    self->sinewave[k] = S_AMP * sinf((2.f * M_PI * k * AT_SINE_WAVE_FREQ) /
                                     (float)self->sample_rate);
  }
}

static void compute_spl_reference_spectrum(SplSpectrumConverter *self) {
  for (uint32_t k = 0; k < self->fft_size; k++) {
    get_fft_input_buffer(self->fft_transform)[k] =
        self->sinewave[k] * self->window[k];
  }

  compute_forward_fft(self->fft_transform);

  compute_power_spectrum(self->spectral_features,
                         get_fft_output_buffer(self->fft_transform),
                         self->fft_size);
  float *reference_spectrum = get_power_spectrum(self->spectral_features);

  for (uint32_t k = 1; k <= self->half_fft_size; k++) {
    self->spl_reference_values[k] =
        REFERENCE_LEVEL - 10.f * log10f(reference_spectrum[k]);
  }
}

bool convert_spectrum_to_dbspl(SplSpectrumConverter *self, float *spectrum) {
  if (!self || !spectrum) {
    return false;
  }
  for (uint32_t k = 1; k <= self->half_fft_size; k++) {
    spectrum[k] += self->spl_reference_values[k];
  }

  return true;
}