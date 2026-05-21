# Analyze the text of the speech  
Keywords: Speech-to-Text, Speech Analysis, Whisper, Text Alignment, Syllable Analysis, Voice Transcription

1. Create a Whisper - Token plugin track `{"identifier": "ircamwhisper:whisper", "feature": "token"}`.
2. Set the Whisper track's `"splitmode"` parameter to `0` (Sentence).
3. Hide the Whisper track from the group.
4. Create a VAX - Aligner plugin track `{"identifier": "ircamvax:ircamvaxaligner", "feature": "text"}`.
5. Set the VAX track's `"parsingmode"` parameter to `2` (Syllable).
6. Set the VAX track's `"modeltype"` parameter to `0` (Singing) or `1` (Speech), depending on the audio content.
7. Define the Whisper track as the `{identifier: "text"}` input of the VAX track.
8. Tips:  
    * Increase the Whisper model size when processing time is not critical.  
    * Get the Whisper results to verify that the generated text is valid, and increase the model size if necessary.

# Analyze the pitch  
Keywords: Pitch Analysis, Pitch Detection, Fundamental Frequency, Pitch Tracking, Audio Analysis, Pitch Confidence

1. Get information about the file and its contents (ask the user if necessary).
2. Create a pitch analysis track:  
    * If the audio contains a percussive instrument, use the Pitched Percussion - Pitch plugin `{"identifier": "supervp:supervpf0pitchedpercussion", "feature": "fundamental"}`.  
    * If the audio contains a monophonic instrument and is short (< 30 seconds), use the Crepe - Pitch plugin `{"identifier": "ircamcrepe:crepe", "feature": "pitch"}`.  
    * If the audio contains a monophonic instrument and is long (>= 30 seconds), use the Feature Scoring - Pitch plugin `{"identifier": "supervp:supervpf0featurescoring", "feature": "fundamental"}`.
3. Rename the pitch track based on the analysis and for the user.
4. Get the summary of results (wait a few seconds if necessary), and adjust the **confidence threshold** to exclude results with a low score.
5. Tips:  
    * The Feature Scoring - Pitch plugin is the fastest and most versatile, but may require complex configuration.  
    * The Crepe - Pitch plugin is designed for vocals but works well with other monophonic instruments. It is the easiest to use but also the slowest. Larger models generally produce better results, but processing time increases rapidly.

# Analyze the first N harmonic partials  
Keywords: Harmonic Partial Analysis, Harmonic Partial Tracking, Frequency Analysis, Partial Frequencies

1. Create N Harmonic Partial - Frequency plugin tracks `{"identifier": "pm2:pm2harmonicpartialtracking", "feature": "frequency"}`. **Use the plugin globally and define the group globally.**  
    * Set `"maximumnumberofpartials"` to value `N` as a **global parameter**.  
    * Set `"partialid"` by incrementing its value from `1` to `N` as a **per-track parameter**.  
    * Set each track name according to its `"partialid"`.  
    * **Do not repeat global plugin, group, or parameter values in individual tracks.**  
2. If no pitch track exists, create a Crepe - Pitch plugin track `{"identifier": "ircamcrepe:crepe", "feature": "pitch"}`, or ask the user to create a pitch track.
3. Define the pitch track as the input `{identifier: "frequency"}` of all Harmonic Partial tracks.
4. Tips:  
    * Create a gradient for the foreground and text colors of the Harmonic Partial tracks.  
    * If you created the pitch track, hide it from the group, get the summary of results (wait a few seconds if necessary), and adjust the confidence threshold to exclude results with a low score.  
    * Get the summary of results (wait a few seconds if necessary) of the first and last partials and zoom vertically in the group to contain all the frequencies.

# Analyze the first N chord partials  
Keywords: Chord Partial Analysis, Chord Partial Tracking, Frequency Analysis, Chord Detection, Partial Frequencies

1. Create N Chord Partial - Frequency plugin tracks `{"identifier": "pm2:pm2chordpartialtracking", "feature": "frequency"}`. **Use the plugin and group globally.**  
    * Set `"maximumnumberofpartials"` to value `N` as a **global parameter**.  
    * Set `"partialid"` by incrementing its value from `1` to `N` as a **per-track parameter**.  
    * Set each track name according to its `"partialid"`.  
    * **Do not repeat global plugin, group, or parameter values in individual tracks.**  
2. If no marker track exists, create a Transient Detection - Marker plugin track `{"identifier": "supervp:supervptransientdetection", "feature": "transientinfo"}`, or ask the user to create a marker track.
3. Define the marker track as the `{identifier: "frequency"}` input of all Chord Partial tracks.
4. Tips:  
    * Create a gradient for the foreground and text colors of the Chord Partial tracks.  
    * Get the summary of results (wait a few seconds if necessary) of the first and last partials and zoom vertically in the group to contain all the frequencies.

# Analyze the transients  
Keywords: Transient Analysis, Transient Detection, Onset Detection, Audio Transients

1. If no spectrogram exists, create a Reassigned Spectrum - Spectrogram plugin track `{"identifier":"supervp:supervpspectrogramreaspectrum", "feature":"spectrogram"}`.
2. Create a Transient Detection - Marker plugin track `{"identifier": "supervp:supervptransientdetection", "feature": "transientinfo"}` in the **same group**.
3. Get the summary of results (wait a few seconds if necessary) of the transient track, and adjust the **energy extra threshold** to exclude unwanted results.
4. Tips:  
    * Reduce the **minimum onset time interval** to increase the time accuracy of the analysis.  
    * Increase the **energy extra threshold** to exclude unwanted results.

# Analyze the musical/MIDI notes  
Keywords: Musical Note Analysis, MIDI Note Extraction, Pitch Detection, Onset Detection, Note Aggregation, Musical Notation

1. If no pitch track exists, create a Crepe - Pitch plugin track `{"identifier": "ircamcrepe:crepe", "feature": "pitch"}` (or ask the user to create a pitch track).
2. If no marker track exists, create a Transient Detection - Marker plugin track `{"identifier": "supervp:supervptransientdetection", "feature": "transientinfo"}` (or ask the user to create a marker track).
3. Create an Aggregator - Notes track `{"identifier": "ircammisc:noteagregator", "feature": "notes"}` and set the pitch and marker tracks as the **frequency and transients input tracks**.
4. Get the summaries of the pitch and marker tracks (wait a few seconds if necessary), and adjust the **confidence** and **energy extra thresholds** to exclude unwanted results.
5. Get the raw results of the Aggregator - Notes track and provide the equivalent **musical notation and/or MIDI notes**.
6. Tips:  
    * Get the energy results of the marker track to estimate the energy of the notes in **dB and/or MIDI velocity**.  
    * If you created the pitch track, hide it from the group.
