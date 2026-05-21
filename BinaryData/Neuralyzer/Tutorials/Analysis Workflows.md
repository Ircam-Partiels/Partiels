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
    * If the audio contains a pitched percussive instrument (piano, guitar, marimba...), use the Pitched Percussion - Pitch plugin `{"identifier": "ircamsvp:supervpf0pitchedpercussion", "feature": "fundamental"}`.  
    * If the audio contains a monophonic instrument or voice and is short (< 30 seconds), use the Crepe - Pitch plugin `{"identifier": "ircamcrepe:crepe", "feature": "pitch"}`.  
    * If the audio contains a monophonic instrument or voice and is long (>= 30 seconds), use the Feature Scoring - Pitch plugin `{"identifier": "ircamsvp:supervpf0featurescoring", "feature": "fundamental"}`.  
    * If the audio is polyphonic, tell the user that these plugins estimate a single fundamental frequency, and propose the analysis of the pitches of the polyphonic notes (see **Analyze the pitches of the polyphonic notes**) or the analysis of the chord partials (see **Analyze the first N chord partials**) instead.
3. Give the pitch track a name that describes the analysis for the user (e.g. "Voice Pitch").
4. Get the summary of results (with `waitSeconds` if the analysis is running), and set the extra threshold of the score to exclude the results with a low score: the `"confidence"` extra output of Crepe or the `"score"` extra output of Feature Scoring (the Pitched Percussion plugin has no extra output).
5. Tips:  
    * The Feature Scoring - Pitch plugin is the fastest and most versatile, but may require complex configuration (`"minpitch"`, `"maxpitch"`, `"relnoisethreshold"`...).  
    * The Crepe - Pitch plugin is designed for vocals but works well with other monophonic instruments. It is the easiest to use but also the slowest. Larger models (`"modelcapacity"` from `0` Tiny to `4` Full) generally produce better results, but processing time increases rapidly.  
    * If the instrument alternates between several playing techniques (e.g. ordinario, percussive, multiphonic), see **Analyze the pitch based on playing techniques**.

# Analyze the pitch based on playing techniques  
Keywords: Pitch Analysis, Playing Techniques, Instrumental Playing Techniques, IPT, Extended Techniques, Ordinario, Multiphonics, Percussive Techniques, Fundamental Frequency, F0, Pitch Tracking

No pitch plugin suits all the playing techniques of an instrument. The IPT plugin segments the audio by playing technique, then each technique is analyzed with the most appropriate pitch plugin, and each pitch track keeps only the results of the time ranges of its techniques.

1. Identify the instrument from the metadata of the audio file with `get_audio_file_info`, or with an instrument classification plugin if one is installed, or ask the user.
2. Get the description of the IPT - Category plugin `{"identifier": "ircamipt:ipt", "feature": "marker"}` and check if a value of its `"model"` parameter corresponds to the instrument (e.g. `0` eguitar, `1` flute, `2` trumpet, `3` trumpet_harmon). If no model corresponds, tell the user and use **Analyze the pitch** instead.
3. Create an IPT - Category plugin track with the `"model"` parameter set to the instrument, and name it after the analysis (e.g. "Flute Techniques").
4. Get the summary of results (with `waitSeconds` if the analysis is running), set the `"score"` extra threshold (and the `"occurrences"` extra threshold if necessary) to exclude the unreliable categories, then get the raw results to list the playing techniques and their time ranges: a category lasts from its marker to the next marker. If necessary, correct the categories: erase the isolated or wrong markers with `erase_track_results` (the time range `[time, time]` erases the marker at this time), or modify them with `set_track_results_raw`.
5. Select a pitch plugin for each playing technique (ask the user if a technique is ambiguous):  
    * Ordinary and sustained techniques on a monophonic sound (e.g. ordinario, vibrato): the Crepe - Pitch plugin `{"identifier": "ircamcrepe:crepe", "feature": "pitch"}`, or the Feature Scoring - Pitch plugin `{"identifier": "ircamsvp:supervpf0featurescoring", "feature": "fundamental"}` for long audio (>= 30 seconds).  
    * Percussive techniques (e.g. slap, pizzicato, key clicks): the Pitched Percussion - Pitch plugin `{"identifier": "ircamsvp:supervpf0pitchedpercussion", "feature": "fundamental"}`.  
    * Polyphonic techniques (e.g. multiphonics, chords): the Basic Pitch - Pitch plugin `{"identifier": "ircambasicpitch:basicpitch", "feature": "pitch"}` with `linkedTrackCount` set to the maximum number of simultaneous notes (see **Analyze the pitches of the polyphonic notes**).  
    * Unpitched techniques (e.g. breath, noise): no pitch track.
6. Create one pitch track for each selected plugin in the group of the IPT track, and name it after its techniques (e.g. "Pitch - Ordinario", "Pitch - Multiphonics").
7. Get the summary of results of each pitch track (with `waitSeconds` if the analysis is running), and set the extra thresholds to exclude the results with a low score: the `"confidence"` extra output of Crepe, the `"score"` extra output of Feature Scoring, or the `"amplitude"` extra output of Basic Pitch (the Pitched Percussion plugin has no extra output).
8. For each pitch track (and each linked track of Basic Pitch), keep only the results in the time ranges of its techniques:  
    * If the track has no results in the time ranges of its techniques (count them with `get_track_results_summary` and the `"timeRanges"` of its techniques), remove it.  
    * Otherwise, erase the results outside the time ranges of its techniques with `erase_track_results` and the complementary `"timeRanges"`: from `0` to the start of the first range, between the consecutive ranges, and from the end of the last range to the duration of the audio file.
9. Tips:  
    * Edit the results of the pitch tracks only when their parameters are final: modifying the parameters runs the analysis again.  
    * Use `erase_track_results` rather than `set_track_results_raw` to remove results: it doesn't require getting and sending the results, which are numerous with the pitch tracks with a high time resolution (e.g. Crepe).

# Analyze the first N harmonic partials  
Keywords: Harmonic Partial Analysis, Harmonic Partial Tracking, Sinusoidal Analysis, Frequency Analysis, Partial Frequencies, Harmonics, Multitrack Output, Linked Tracks, PM2

The Harmonic Partial plugin tracks the harmonic partials of a monophonic sound from its pitch and returns each partial in its own track; the Partial output is a multitrack output.

1. Choose the group of the analysis, so that the partials are displayed over the spectral content and share its frequency zoom:  
    * If a group contains a frequency spectrogram (a vector track whose bins are frequencies, e.g. a Fast Fourier Transform or Reassigned Spectrum spectrogram), use this group.  
    * Otherwise, create a new group, create a Fast Fourier Transform - Spectrogram plugin track `{"identifier": "ircamsvp:supervpspectrogramfft", "feature": "spectrogram"}` in it (see **How to configure a Fourier transform spectrogram?**), and use this new group.  
    * Create all the following tracks in this group (`groupIdentifier`).
2. If no pitch track exists, create a Crepe - Pitch plugin track `{"identifier": "ircamcrepe:crepe", "feature": "pitch"}` in the group of the analysis (or ask the user to create a pitch track), and hide it from the group. An existing pitch track of another group can be used as the input of the partials without being moved.
3. Create a single Harmonic Partial - Partial plugin track `{"identifier": "ircampm2:pm2harmonicpartialtracking", "feature": "frequency"}` in the group of the analysis with `linkedTrackCount` set to `N`. The linked tracks are created together at the top of the group, from the bottom to the top (the first partial is the lowest track): **do not create the other linked tracks**, they share the same analysis and the same parameters.
4. Give all the linked tracks (returned by the creation) names that describe the partials, or set the `name` of the track at creation to suffix it automatically with the number of each partial (e.g. "Partial 1", "Partial 2"...).
5. Set the pitch track as the input of only one of the linked tracks (`"receiverInputIdentifier": "frequency"`), the other linked tracks are updated automatically.
6. Tips:  
    * Create a gradient for the foreground and text colors of the linked tracks with the `colorGradient` of `set_track_graphics` (the tracks in the order of the `linkedTracks` property returned by `get_track_properties`, from the first to the last partial).  
    * If you created the pitch track, get the summary of its results (with `waitSeconds` if the analysis is running) and set its `"confidence"` extra threshold to exclude the results with a low score.  
    * Get the summary of results (with `waitSeconds` if the analysis is running) of the first and last partials and zoom vertically in the group to contain all the frequencies.  
    * Use `set_linked_track_count` to change the number of partials (the tracks are added or removed and returned by the tool).  
    * The window of the analysis is defined by the `"windowduration"` parameter in milliseconds, or by the `"windowsize"` alias parameter in samples that modifies the window duration: set only one of them. Longer windows improve the frequency resolution of low pitches but reduce the time resolution.  
    * Decrease the `"amplitudethreshold"` parameter (the maximum amplitude attenuation in dB) to ignore the partials with a low amplitude.

# Analyze the first N chord partials  
Keywords: Chord Partial Analysis, Chord Partial Tracking, Polyphonic Partial Tracking, Sinusoidal Analysis, Frequency Analysis, Partial Frequencies, Multitrack Output, Linked Tracks, PM2

The Chord Partial plugin tracks the partials of the chords in time regions and returns each partial in its own track; it doesn't recognize the chords (names or degrees). The Partial output is a multitrack output.

1. Choose the group of the analysis, so that the partials are displayed over the spectral content and share its frequency zoom:  
    * If a group contains a frequency spectrogram (a vector track whose bins are frequencies, e.g. a Fast Fourier Transform or Reassigned Spectrum spectrogram), use this group.  
    * Otherwise, create a new group, create a Fast Fourier Transform - Spectrogram plugin track `{"identifier": "ircamsvp:supervpspectrogramfft", "feature": "spectrogram"}` in it (see **How to configure a Fourier transform spectrogram?**), and use this new group.  
    * Create all the following tracks in this group (`groupIdentifier`).
2. If no marker track exists, create a Transient Detection - Marker plugin track `{"identifier": "ircamsvp:supervptransientdetection", "feature": "transientinfo"}` in the group of the analysis (or ask the user to create a marker track), and hide it from the group. An existing marker track of another group can be used as the input of the partials without being moved.
3. Create a single Chord Partial - Partial plugin track `{"identifier": "ircampm2:pm2chordpartialtracking", "feature": "frequency"}` in the group of the analysis with `linkedTrackCount` set to `N`. The linked tracks are created together at the top of the group, from the bottom to the top (the first partial is the lowest track): **do not create the other linked tracks**, they share the same analysis and the same parameters.
4. Give all the linked tracks (returned by the creation) names that describe the partials, or set the `name` of the track at creation to suffix it automatically with the number of each partial (e.g. "Partial 1", "Partial 2"...).
5. Set the marker track as the input of only one of the linked tracks (`"receiverInputIdentifier": "frequency"`, the input of the chord regions), the other linked tracks are updated automatically.
6. Tips:  
    * Create a gradient for the foreground and text colors of the linked tracks with the `colorGradient` of `set_track_graphics` (the tracks in the order of the `linkedTracks` property returned by `get_track_properties`, from the first to the last partial).  
    * Get the summary of results (with `waitSeconds` if the analysis is running) of the first and last partials and zoom vertically in the group to contain all the frequencies.  
    * Set `"useregionsduration"` to `1` to use the durations of the markers as the regions of the chords.  
    * Use `set_linked_track_count` to change the number of partials (the tracks are added or removed and returned by the tool).  
    * The window of the analysis is defined by the `"windowduration"` parameter in milliseconds, or by the `"windowsize"` alias parameter in samples that modifies the window duration: set only one of them.

# Analyze the pitches of the polyphonic notes  
Keywords: Polyphonic Pitch Analysis, Multiple Pitch Estimation, Automatic Music Transcription, Polyphonic Notes, Multitrack Output, Linked Tracks, Basic Pitch

The Basic Pitch plugin estimates the notes of a polyphonic audio (several instruments or a polyphonic instrument) and splits them into monophonic tracks ordered from the lowest to the highest notes; the Pitch output is a multitrack output.

1. Create a single Basic Pitch - Pitch plugin track `{"identifier": "ircambasicpitch:basicpitch", "feature": "pitch"}` with `linkedTrackCount` set to the maximum number of simultaneous notes expected (ask the user if necessary). The linked tracks are created together at the top of the group, from the bottom to the top (the first track is the lowest one): **do not create the other linked tracks**, they share the same analysis and the same parameters.
2. Give all the linked tracks (returned by the creation) names that describe the analysis for the user with the number of the track (e.g. "Piano Notes 1", "Piano Notes 2"...), or set the `name` of the track at creation to suffix it automatically with the number of each linked track.
3. Get the summary of results (with `waitSeconds` if the analysis is running) of the first and last linked tracks, and zoom vertically in the group to contain all the frequencies.
4. Tips:  
    * Create a gradient for the foreground and text colors of the linked tracks with the `colorGradient` of `set_track_graphics` (the tracks in the order of the `linkedTracks` property returned by `get_track_properties`).  
    * Set the `"amplitude"` extra threshold of the linked tracks to exclude the notes with a low amplitude.  
    * Increase the `"framethreshold"` and `"onsetthreshold"` parameters to detect fewer notes, and the `"minnoteduration"` parameter to ignore the short notes. Modify the parameters of only one of the tracks, the other linked tracks are updated automatically.  
    * Use the `linkedTracks` property of `get_track_properties` to get the number of linked tracks, its range, and all the linked tracks, and `set_linked_track_count` to change the number of linked tracks (the tracks are added or removed and returned by the tool); the parameters don't change the number of linked tracks.

# Analyze the transients  
Keywords: Transient Analysis, Transient Detection, Onset Detection, Audio Transients, Attacks

1. If no spectrogram exists, create a Reassigned Spectrum - Spectrogram plugin track `{"identifier": "ircamsvp:supervpspectrogramreaspectrum", "feature": "spectrogram"}` (as a vector track, it is created at the bottom of the group), to visualize the transients over the spectral content.
2. Create a Transient Detection - Marker plugin track `{"identifier": "ircamsvp:supervptransientdetection", "feature": "transientinfo"}` in the **same group**.
3. Get the summary of results (with `waitSeconds` if the analysis is running) of the transient track, and increase the `"energy"` extra threshold to exclude the unwanted results.
4. Tips:  
    * Reduce the `"minonsetduration"` parameter (Minimum Onset Time Interval) to detect transients that are closer in time.  
    * Increase the `"sensitivitythreshold"` parameter to detect fewer transients.

# Analyze the musical/MIDI notes  
Keywords: Musical Note Analysis, MIDI Note Extraction, Pitch Detection, Onset Detection, Note Aggregation, Musical Notation

1. If no pitch track exists, create a Crepe - Pitch plugin track `{"identifier": "ircamcrepe:crepe", "feature": "pitch"}` (or ask the user to create a pitch track), and hide it from the group.
2. If no marker track exists, create a Transient Detection - Marker plugin track `{"identifier": "ircamsvp:supervptransientdetection", "feature": "transientinfo"}` (or ask the user to create a marker track).
3. Get the summaries of the pitch and marker tracks (with `waitSeconds` if the analysis is running), and set the `"confidence"` and `"energy"` extra thresholds to exclude the unwanted results.
4. Create an Aggregator - Notes track `{"identifier": "partiels-vamp-plugins:noteaggregator", "feature": "notes"}`, get its compatible inputs, and set the pitch and marker tracks as its **frequency and transient inputs** (`"receiverInputIdentifier": "frequencies"` and `"transients"`).
5. Get the summary, then the raw results of the Aggregator - Notes track, and provide the equivalent **musical notation and/or MIDI notes**: `midi = 69 + 12 × log2(frequency / 440)`, rounded to the nearest integer (60 is C4, 69 is A4).
6. Tips:  
    * Get the energy results of the marker track to estimate the dynamics of the notes in **dB and/or MIDI velocity**.
