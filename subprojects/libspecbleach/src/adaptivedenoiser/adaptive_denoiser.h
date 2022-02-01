/*
libspecbleach - A spectral processing library

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

#ifndef SPECTRAL_ADAPTIVE_DENOISER_H
#define SPECTRAL_ADAPTIVE_DENOISER_H

#include "../shared/spectral_processor.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct AdaptiveDenoiserParameters {
  float reduction_amount;
  float noise_rescale;
  bool residual_listen;
} AdaptiveDenoiserParameters;

SpectralProcessorHandle
spectral_adaptive_denoiser_initialize(uint32_t sample_rate, uint32_t fft_size,
                                      uint32_t overlap_factor);
void spectral_adaptive_denoiser_free(SpectralProcessorHandle instance);
bool load_adaptive_reduction_parameters(SpectralProcessorHandle instance,
                                        AdaptiveDenoiserParameters parameters);
bool spectral_adaptive_denoiser_run(SpectralProcessorHandle instance,
                                    float *fft_spectrum);

#endif