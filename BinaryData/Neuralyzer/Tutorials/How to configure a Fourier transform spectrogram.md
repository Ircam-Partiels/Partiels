# How to configure a Fourier transform spectrogram?
Keywords: Spectrogram, FFT, Fast Fourier Transform, Short-Time Fourier Transform, STFT, Window, Frequency Resolution, Time Resolution, Color Map

The tutorial explains how to generate and configure an FFT spectrogram using the SuperVP plugin to analyze the frequency characteristics of an audio signal.

## Spectrogram Creation

The workflow consists of creating an FFT analysis track with the Fast Fourier Transform - Spectrogram plugin `{"identifier": "supervp:supervpspectrogramfft", "feature": "spectrogram"}`, and adjusting the analysis and graphical parameters.

The SuperVP plugin from the Ircam Vamp Plugins is required. The spectrogram is a vector track: place it at the bottom of the group (position 0), behind the other tracks.

## Analysis Parameters

### Window Size
The `"windowsize"` parameter defines the duration of the analyzed audio frames, in milliseconds.

* Large window:
  * better frequency precision,
  * lower temporal precision.
* Small window:
  * better temporal precision,
  * lower frequency precision.

This is the most important FFT parameter. To separate two partials with a Hanning window, the window must last at least about 4 periods of their frequency difference (e.g. 40 ms to separate partials 100 Hz apart, more with a Blackman window).

### Window Overlapping
The `"windowoverlapping"` parameter defines how much consecutive analysis frames overlap (the values `0` to `4` correspond to 3, 4, 8, 16, and 32 times).

* High overlap:
  * more frames and denser time sampling,
  * smoother display of the temporal variations,
  * heavier computation.
* Low overlap:
  * lighter computation,
  * fewer frames and less temporal detail.

The overlap doesn't change the temporal precision of each frame, which depends on the window size.

### FFT Oversampling
The `"fftoversampling"` parameter uses zero-padding to increase the number of frequency bins (the values `0` to `3` correspond to 1, 2, 4, and 8 times).

* interpolates the spectrum and improves its appearance,
* does not improve the frequency precision, which depends on the window size,
* increases the computation and the size of the results.

### Window Type
The `"windowtype"` parameter affects the frequency precision and the spectral leakage (the energy spreading around the peaks). The values are `0` (Blackman), `1` (Hanning), and `2` (Hamming).

* Blackman: the lowest spectral leakage but the widest peaks, best for clean spectral visualization and signals with a high dynamic range.
* Hanning: a balanced compromise, best for general audio analysis.
* Hamming: narrower peaks than Hanning, but more leakage far from the peaks, suitable to distinguish close frequencies of similar levels.

## Graphical Parameters

### Color Map
The `colorMap` property changes the visual representation of the spectrogram for accessibility, display preferences, and publication needs.

* Parula: a smooth blue-green-yellow perceptually uniform color map, the default of MATLAB.
* Heat: a rainbow color map from blue to cyan, green, yellow, and red.
* Jet: a rainbow color map from dark blue to dark red, visually striking but not perceptually uniform, often used in legacy plotting.
* Turbo: an improved rainbow replacement for Jet, smoother and more readable, but not strictly perceptually uniform.
* Hot: a black-to-red-to-yellow-white gradient used to represent increasing intensity, as in thermal images.
* Gray: a grayscale color map that represents the intensity without color bias, suitable for print.
* Magma: a perceptually uniform black-to-purple-to-light-yellow color map.
* Inferno: a perceptually uniform black-to-purple-to-orange-to-yellow color map with a high contrast.
* Plasma: a perceptually uniform blue-to-purple-to-orange-to-yellow color map, vibrant even for the low values.
* Viridis: a perceptually uniform purple-to-blue-to-green-to-yellow color map, designed for accessibility.
* Cividis: a perceptually uniform blue-to-yellow color map, optimized for color vision deficiencies.
* Github: a light gray to dark green color map, as in the GitHub contribution graphs, suitable for light backgrounds.

### Value Range
The value range (`set_track_value_range`) defines the energies mapped to the color map, between -120 dB and 0 dB.

* Increase the low threshold to hide low-energy components,
* Decrease the high threshold to highlight high-energy components.

## Additional Analysis Methods
The SuperVP plugin also supports:

* Cepstrum `{"identifier": "supervp:supervpspectrogramcepstrum", "feature": "spectrogram"}`: a smoothed spectral envelope obtained from the cepstrum (the inverse Fourier transform of the log-magnitude spectrum), which separates the source and filter characteristics.
* LPC `{"identifier": "supervp:supervpspectrogramlpc", "feature": "spectrogram"}`: a spectral envelope obtained by Linear Predictive Coding, which models the signal as a linear combination of past samples, commonly used for speech and formants.
* True Envelope `{"identifier": "supervp:supervpspectrogramtrueenv", "feature": "spectrogram"}`: a spectral envelope obtained by iterative cepstral smoothing, which passes through the peaks of the partials without over-smoothing.
* Reassigned Spectrum `{"identifier": "supervp:supervpspectrogramreaspectrum", "feature": "spectrogram"}`: a high-resolution time-frequency representation that sharpens the spectrogram by relocating the energy to the instantaneous frequency and time centers.

## Key Takeaways
* Window size controls the time/frequency precision trade-off.
* Overlap increases the time sampling density and the smoothness of the display.
* Oversampling improves display quality but not the actual frequency precision.
* Window type influences the spectral leakage and the width of the peaks.
* Graphical settings improve readability and interpretation.
