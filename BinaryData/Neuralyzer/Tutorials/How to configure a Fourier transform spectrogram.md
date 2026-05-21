# How to configure a Fourier transform spectrogram?

The tutorial explains how to generate and configure an FFT spectrogram using the SuperVP plugin to analyze the frequency characteristics of an audio signal.

## Spectrogram Creation

The workflow consists of creating an FFT analysis track with the Fast Fourier Transform Spectrogram plugin, and adjusting analysis and visualization parameters.

The SuperVP plugin from Ircam Vamp Plugins is required.

## Analysis Parameters

### Window Size
Defines the duration of the analyzed audio frames.

* Large window:
  * better frequency precision,
  * lower temporal precision.
* Small window:
  * better temporal precision,
  * lower frequency precision.

This is the most important FFT parameter.

### Window Overlapping
Defines how much consecutive analysis frames overlap.

* High overlap:
  * smoother temporal continuity,
  * more precise temporal variations.
* Low overlap:
  * lighter computation,
  * less temporal detail.

### FFT Oversampling
Uses zero-padding to increase visual frequency resolution.

* improves spectral appearance,
* does not add real information,
* may introduce artifacts.

### Window Type
Affects the frequency precision, spectral leakage, and analysis artifacts.

* Hanning: Balanced performance, moderate resolution, best for general audio analysis
* Hamming: Better frequency precision, more leakage, best for harmonic and pitch analysis
* Blackman: Lowest spectral leakage, lower frequency resolution, best for clean spectral visualization

## Graphical Parameters

### Color Map
Changes the visual representation of the spectrogram for accessibility, display preferences, and publication needs.

* Parula: A smooth blue–green–yellow perceptually uniform colormap commonly used in MATLAB for scientific data visualization.
* Heat: A black-to-red-to-yellow-white intensity scale used to emphasize increasing magnitude in heatmaps and thermal-like data.
* Jet: A rainbow-style colormap spanning blue to red that is visually striking but not perceptually uniform, often used in legacy plotting.
* Turbo: A modern perceptually uniform rainbow replacement for jet that preserves color variation while improving readability of gradients.
* Hot: A black-to-red-to-yellow-white gradient used to represent increasing intensity, commonly in thermal or signal strength plots.
* Gray: A grayscale colormap used to represent intensity without color bias, ideal for structural or luminance-focused data.
* Magma: A perceptually uniform dark purple-to-yellow colormap designed for clear contrast in low-light and high-density scientific plots.
* Inferno: A high-contrast black-to-purple-to-orange-to-yellow colormap optimized for perceptual clarity in complex visualizations.
* Plasma: A vibrant purple-to-yellow perceptually uniform colormap used to highlight continuous variation with strong visual impact.
* Viridis: A blue-to-green-to-yellow perceptually uniform default colormap designed for accessibility and consistent data interpretation.
* Cividis: A blue-to-yellow perceptually uniform colormap optimized for colorblind accessibility and reliable grayscale reproduction.
* GitHub: A blue-centered neutral palette often used in GitHub-style charts and UI heatmaps to convey data intensity with a clean, minimal aesthetic.

### Value Range
Defines visible energy thresholds (range -120 dB to 0 dB) in the spectrogram.

* Increase low threshold to hide low-energy components,
* Decrease high threshold to highlight high-energy components.

## Additional Analysis Methods
The SuperVP plugin also supports:

* Cepstrum: A signal processing representation obtained by taking the inverse Fourier transform of the log spectrum, used to separate source and filter characteristics (e.g., pitch detection).
* LPC (Linear Predictive Coding): A parametric speech model that approximates a signal as a linear combination of past samples to efficiently represent spectral envelope and compression.
* True Envelope: A spectral smoothing method that estimates the perceptual amplitude contour of a signal while preserving harmonic structure without over-smoothing peaks.
* Reassigned Spectrum: A high-resolution time–frequency representation that sharpens spectrograms by relocating energy to instantaneous frequency and time centers for improved localization.

## Key Takeaways
* Window size controls the time/frequency precision trade-off.
* Overlap improves temporal continuity.
* Oversampling improves display quality but not actual information quality.
* Window type influences spectral artifacts.
* Graphical settings improve readability and interpretation.
