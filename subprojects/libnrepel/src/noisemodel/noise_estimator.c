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

#include "noise_estimator.h"
#include "../shared/spectral_features.h"
#include "louizou_estimator.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

struct NoiseEstimator {
  uint32_t fft_size;
  uint32_t half_fft_size;
  bool noise_spectrum_available;
  float noise_blocks_count;

  SpectralFeatures *spectral_features;
  ProcessorParameters *parameters;
  NoiseProfile *noise_profile;
  LouizouEstimator *adaptive_estimator;
};

NoiseEstimatorHandle
noise_estimation_initialize(const uint32_t fft_size, const uint32_t sample_rate,
                            NoiseProfile *noise_profile,
                            ProcessorParameters *parameters) {
  NoiseEstimator *self = (NoiseEstimator *)calloc(1U, sizeof(NoiseEstimator));

  self->fft_size = fft_size;
  self->half_fft_size = self->fft_size / 2U;
  self->noise_blocks_count = 0U;
  self->noise_spectrum_available = false;

  self->noise_profile = noise_profile;
  self->parameters = parameters;

  self->spectral_features =
      spectral_features_initialize(self->half_fft_size + 1U);
  self->adaptive_estimator = louizou_estimator_initialize(
      self->half_fft_size + 1U, sample_rate, fft_size);

  return self;
}

void noise_estimation_free(NoiseEstimatorHandle instance) {
  NoiseEstimator *self = (NoiseEstimator *)instance;

  spectral_features_free(self->spectral_features);
  louizou_estimator_free(self->adaptive_estimator);
  free(self);
}

bool is_noise_estimation_available(NoiseEstimatorHandle instance) {
  NoiseEstimator *self = (NoiseEstimator *)instance;

  return self->noise_spectrum_available;
}

static void get_rolling_mean_noise_spectrum(NoiseEstimator *self,
                                            const float *spectrum,
                                            float *noise_spectrum) {
  for (uint32_t k = 1U; k <= self->half_fft_size; k++) {
    if (self->noise_blocks_count <= 1U) {
      noise_spectrum[k] = spectrum[k];
    } else {
      noise_spectrum[k] +=
          ((spectrum[k] - noise_spectrum[k]) / self->noise_blocks_count);
    }
  }
}

bool noise_estimation_run(NoiseEstimatorHandle instance, float *fft_spectrum) {
  if (!instance || !fft_spectrum) {
    return false;
  }

  NoiseEstimator *self = (NoiseEstimator *)instance;

  self->noise_blocks_count++;

  compute_power_spectrum(self->spectral_features, fft_spectrum, self->fft_size);

  float *noise_profile = get_noise_profile(self->noise_profile);
  const float *reference_spectrum = get_power_spectrum(self->spectral_features);

  if (self->parameters->auto_learn_noise) {
    louizou_estimator_run(self->adaptive_estimator, reference_spectrum,
                          noise_profile);
  } else {
    get_rolling_mean_noise_spectrum(self, reference_spectrum, noise_profile);
  }

  self->noise_spectrum_available = true;

  return true;
}
