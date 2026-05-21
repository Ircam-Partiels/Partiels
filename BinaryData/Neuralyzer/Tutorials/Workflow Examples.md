# Workflow Examples
A series of workflow examples.

## Analyze the text of the audio/speech
1. Create a Whisper - Token plugin track {"identifier": "ircamwhisper:whisper", "feature": "token"}
2. Define the "splitmode" parameter of the Whisper track to the value '0' (Sentence)
3. Hide the Whisper track from the group
4. Create a VAX - Aligner plugin track {"identifier": "ircamvax:ircamvaxaligner", "feature": "text"}
5. Define the "parsingmode" parameter of the VAX track to value '2' (Syllable)
6. Define the "modeltype" parameter of the VAX track to either '0' (Singing) or '1' (Speech) if the audio content is known
7. Define the Whisper track as the input of the VAX track

### Tips: 
* Increase the size of the Whisper model based if the audio duration is not too important 
* Get the Whisper results to ensure the generated text is valid, and increase the model size if necessary

## Analyze the pitch
1. Get information about the file and its contents (ask the user if necessary)
2. Create a pitch analysis track:
   * If the audio contains a percussive instrument, use the Pitched Percussion - Pitch plugin {"identifier": "supervp:supervpf0pitchedpercussion", "feature": "fundamental"}
   * If the audio contains a monophonic instrument and is short (< 30 seconds), use the Crepe - Pitch plugin {"identifier": "ircamcrepe:crepe", "feature": "pitch"}
   * If the audio contains a monophonic instrument and is long (>= 30 seconds), use the Feature Scoring - Pitch plugin {"identifier": "supervp:supervpf0featurescoring", "feature": "fundamental"}
3. Rename the pitch track for the user and based on the analysis
4. Get the summary of results (wait a few seconds if necessary), and adjust the confidence threshold to exclude results a with low score

### Tips: 
* The Feature Scoring - Pitch plugin is the fastest and most versatile, but it may require complex configuration
* The Crepe - Pitch plugin is designed for vocals but works well with other monophonic instruments. It is the easiest to use but also the slowest. Results are better with larger models, but processing time increases rapidly

## Analyze the first N harmonic partials
1. Create N Harmonic Partial - Frequency plugin tracks {"identifier": "pm2:pm2harmonicpartialtracking", "feature": "frequency"}
2. Define the "maximumnumberofpartials" parameter of all the Harmonic Partial tracks to value 'N'
3. Define the "partialid" parameter of all the Harmonic Partial tracks by incrementing the value from '1' to 'N'
4. Rename all the Harmonic Partial tracks according to their partial identifier
5. If no pitch track exists, create a Crepe - Pitch plugin track {"identifier": "ircamcrepe:crepe", "feature": "pitch"}, or ask the user to create a pitch track
6. Define the pitch track as the input of all the Harmonic Partial tracks

### Tips: 
* Create a gradient for the foreground and text colors of the Harmonic Partial tracks
* If you created the pitch track, hide it from the group, get the summary of results (wait a few seconds if necessary) and adjust the confidence threshold to exclude results with a low score

## Analyze the first N chord partials
1. Create N Chord Partial - Frequency plugin tracks {"identifier": "pm2:pm2chordpartialtracking", "feature": "frequency"}
2. Define the "maximumnumberofpartials" parameter of all the Chord Partial tracks to value 'N'
3. Define the "partialid" parameter of all the Chord Partial tracks by incrementing the value from '1' to 'N'
4. Rename all the Chord Partial tracks according to their partial identifier
5. If no marker track exists, create a Transient Detection - Marker plugin track {"identifier": "supervp:supervptransientdetection", "feature": "transientinfo"}, or ask the user to create a marker track
6. Define the marker track as the input of all the Chord Partial tracks

## Analyze the musical/MIDI notes
1. If no pitch track exists, create a Crepe - Pitch plugin track {"identifier": "ircamcrepe:crepe", "feature": "pitch"} (or ask the user to create a pitch track), get the summary of results (wait a few seconds if necessary) and adjust the confidence extra threshold (around 0.5) of the pitch track to exclude unwanted results
2. If no marker track exists, create a Transient Detection - Marker plugin track {"identifier": "supervp:supervptransientdetection", "feature": "transientinfo"} (or ask the user to create a marker track), get the summary of results (wait a few seconds if necessary) and adjust the energy extra threshold of the marker track to exclude unwanted results if necessary
3. Create a Combine - Statictics {"identifier": "partiels-vamp-plugins:transformermarker", "feature": "result"} and sets the pitch and marker tracks for the points and markers input tracks.
4. Get a the raw track results and estimate the musical or MIDI notes.
