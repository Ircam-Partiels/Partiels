# Analyze the text of the speech  
Keywords: Speech-to-Text, Transcription, Transcribe Spoken Words, Speech Recognition, Whisper, Text Alignment, Syllable Analysis, Lyrics

1. Create a Whisper - Token plugin track `{"identifier": "ircamwhisper:whisper", "feature": "token"}` with the `"splitmode"` parameter set to `0` (Sentences).
2. Hide the Whisper track from the group.
3. Create a VAX - Aligner plugin track `{"identifier": "ircamvax:ircamvaxaligner", "feature": "text"}` in the same group, with:  
    * the `"parsingmode"` parameter set to `2` (Syllable),  
    * the `"modeltype"` parameter set to `0` (Speech) or `1` (Singing), depending on the audio content (ask the user if necessary).
4. Set the Whisper track as the input of the VAX track (`"receiverInputIdentifier": "text"`).
5. Tips:  
    * The values of the Whisper `"model"` parameter are not sorted by size: `1` (tiny), `0` (base), `2` (small), `3` (medium), `4` (large). Use a larger model when processing time is not critical.  
    * Get the Whisper results to verify that the generated text is valid, and use a larger model if necessary.  
    * Set the Whisper `"language"` parameter when the language is known, instead of the automatic detection.

# Analyze the pitch  
Keywords: Pitch Analysis, Pitch Detection, Fundamental Frequency, F0, Pitch Tracking, Audio Analysis, Pitch Confidence

1. Get the information of the audio file (duration) with `get_audio_file_info`, and the nature of its content (ask the user if necessary).
2. Create a pitch analysis track:  
    * If the audio contains a pitched percussive instrument (piano, guitar, marimba...), use the Pitched Percussion - Pitch plugin `{"identifier": "supervp:supervpf0pitchedpercussion", "feature": "fundamental"}`.  
    * If the audio contains a monophonic instrument or voice and is short (< 30 seconds), use the Crepe - Pitch plugin `{"identifier": "ircamcrepe:crepe", "feature": "pitch"}`.  
    * If the audio contains a monophonic instrument or voice and is long (>= 30 seconds), use the Feature Scoring - Pitch plugin `{"identifier": "supervp:supervpf0featurescoring", "feature": "fundamental"}`.  
    * If the audio is polyphonic, tell the user that these plugins estimate a single fundamental frequency, and propose the analysis of the chord partials instead (see **Analyze the first N chord partials**).
3. Give the pitch track a name that describes the analysis for the user (e.g. "Voice Pitch").
4. Get the summary of results (with `waitSeconds` if the analysis is running), and set the extra threshold of the score to exclude the results with a low score: the `"confidence"` extra output of Crepe or the `"score"` extra output of Feature Scoring (the Pitched Percussion plugin has no extra output).
5. Tips:  
    * The Feature Scoring - Pitch plugin is the fastest and most versatile, but may require complex configuration (`"minpitch"`, `"maxpitch"`, `"relnoisethreshold"`...).  
    * The Crepe - Pitch plugin is designed for vocals but works well with other monophonic instruments. It is the easiest to use but also the slowest. Larger models (`"modelcapacity"` from `0` Tiny to `4` Full) generally produce better results, but processing time increases rapidly.

# Analyze the first N harmonic partials  
Keywords: Harmonic Partial Analysis, Harmonic Partial Tracking, Frequency Analysis, Partial Frequencies, Harmonics

1. If no pitch track exists, create a Crepe - Pitch plugin track `{"identifier": "ircamcrepe:crepe", "feature": "pitch"}` (or ask the user to create a pitch track), and hide it from the group.
2. Create N Harmonic Partial - Frequency plugin tracks `{"identifier": "pm2:pm2harmonicpartialtracking", "feature": "frequency"}` in the group of the pitch track. **Define the plugin and the group globally.**  
    * Set `"maximumnumberofpartials"` to value `N` as a **global parameter**.  
    * Set `"partialid"` by incrementing its value from `1` to `N` as a **per-track parameter**.  
    * Set each track name according to its `"partialid"`.  
    * **Do not repeat global plugin, group, or parameter values in individual tracks.**  
3. Set the pitch track as the input of all the Harmonic Partial tracks (`"receiverInputIdentifier": "frequency"`, defined globally with the sender track).
4. Tips:  
    * Create a gradient for the foreground and text colors of the Harmonic Partial tracks.  
    * If you created the pitch track, get the summary of its results (with `waitSeconds` if the analysis is running) and set its `"confidence"` extra threshold to exclude the results with a low score.  
    * Get the summary of results (with `waitSeconds` if the analysis is running) of the first and last partials and zoom vertically in the group to contain all the frequencies.

# Analyze the first N chord partials  
Keywords: Chord Partial Analysis, Chord Partial Tracking, Polyphonic Partial Tracking, Frequency Analysis, Partial Frequencies

The Chord Partial plugin tracks the partials of the chords in time regions; it doesn't recognize the chords (names or degrees).

1. If no marker track exists, create a Transient Detection - Marker plugin track `{"identifier": "supervp:supervptransientdetection", "feature": "transientinfo"}` (or ask the user to create a marker track), and hide it from the group.
2. Create N Chord Partial - Frequency plugin tracks `{"identifier": "pm2:pm2chordpartialtracking", "feature": "frequency"}` in the group of the marker track. **Define the plugin and the group globally.**  
    * Set `"maximumnumberofpartials"` to value `N` as a **global parameter**.  
    * Set `"partialid"` by incrementing its value from `1` to `N` as a **per-track parameter**.  
    * Set each track name according to its `"partialid"`.  
    * **Do not repeat global plugin, group, or parameter values in individual tracks.**  
3. Set the marker track as the input of all the Chord Partial tracks (`"receiverInputIdentifier": "frequency"`, the input of the chord regions, defined globally with the sender track).
4. Tips:  
    * Create a gradient for the foreground and text colors of the Chord Partial tracks.  
    * Get the summary of results (with `waitSeconds` if the analysis is running) of the first and last partials and zoom vertically in the group to contain all the frequencies.  
    * Set `"useregionsduration"` to `1` to use the durations of the markers as the regions of the chords.

# Analyze the transients  
Keywords: Transient Analysis, Transient Detection, Onset Detection, Audio Transients, Attacks

1. If no spectrogram exists, create a Reassigned Spectrum - Spectrogram plugin track `{"identifier": "supervp:supervpspectrogramreaspectrum", "feature": "spectrogram"}` at the bottom of the group (position 0), to visualize the transients over the spectral content.
2. Create a Transient Detection - Marker plugin track `{"identifier": "supervp:supervptransientdetection", "feature": "transientinfo"}` in the **same group**.
3. Get the summary of results (with `waitSeconds` if the analysis is running) of the transient track, and increase the `"energy"` extra threshold to exclude the unwanted results.
4. Tips:  
    * Reduce the `"minonsetduration"` parameter (Minimum Onset Time Interval) to detect transients that are closer in time.  
    * Increase the `"sensitivitythreshold"` parameter to detect fewer transients.

# Analyze the musical/MIDI notes  
Keywords: Musical Note Analysis, MIDI Note Extraction, Pitch Detection, Onset Detection, Note Aggregation, Musical Notation

1. If no pitch track exists, create a Crepe - Pitch plugin track `{"identifier": "ircamcrepe:crepe", "feature": "pitch"}` (or ask the user to create a pitch track), and hide it from the group.
2. If no marker track exists, create a Transient Detection - Marker plugin track `{"identifier": "supervp:supervptransientdetection", "feature": "transientinfo"}` (or ask the user to create a marker track).
3. Get the summaries of the pitch and marker tracks (with `waitSeconds` if the analysis is running), and set the `"confidence"` and `"energy"` extra thresholds to exclude the unwanted results.
4. Create an Aggregator - Notes track `{"identifier": "ircammisc:noteaggregator", "feature": "notes"}`, get its compatible inputs, and set the pitch and marker tracks as its **frequency and transients inputs**.
5. Get the summary, then the raw results of the Aggregator - Notes track, and provide the equivalent **musical notation and/or MIDI notes**: `midi = 69 + 12 × log2(frequency / 440)`, rounded to the nearest integer (60 is C4, 69 is A4).
6. Tips:  
    * Get the energy results of the marker track to estimate the dynamics of the notes in **dB and/or MIDI velocity**.
