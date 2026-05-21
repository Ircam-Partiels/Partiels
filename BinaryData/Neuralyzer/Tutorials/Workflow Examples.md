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
  * Define the "maximumnumberofpartials" parameter to value 'N'
  * Define the "partialid" parameter by incrementing the value from '1' to 'N'
  * Define all name according to the "partialid" parameter
2. If no pitch track exists, create a Crepe - Pitch plugin track {"identifier": "ircamcrepe:crepe", "feature": "pitch"}, or ask the user to create a pitch track
3. Define the pitch track as the input of all the Harmonic Partial tracks

### Tips: 
* Create a gradient for the foreground and text colors of the Harmonic Partial tracks
* If you created the pitch track, hide it from the group, get the summary of results (wait a few seconds if necessary) and adjust the confidence threshold to exclude results with a low score

## Analyze the first N chord partials
1. Create N Chord Partial - Frequency plugin tracks {"identifier": "pm2:pm2chordpartialtracking", "feature": "frequency"}
  * Define the "maximumnumberofpartials" parameter to value 'N'
  * Define the "partialid" parameter by incrementing the value from '1' to 'N'
  * Define all name according to the "partialid" parameter
2. If no marker track exists, create a Transient Detection - Marker plugin track {"identifier": "supervp:supervptransientdetection", "feature": "transientinfo"}, or ask the user to create a marker track
3. Define the marker track as the input of all the Chord Partial tracks

### Tips: 
* Create a gradient for the foreground and text colors of the Harmonic Partial tracks

## Analyze the musical/MIDI notes
1. If no pitch track exists, create a Crepe - Pitch plugin track {"identifier": "ircamcrepe:crepe", "feature": "pitch"} (or ask the user to create a pitch track)
2. If no marker track exists, create a Transient Detection - Marker plugin track {"identifier": "supervp:supervptransientdetection", "feature": "transientinfo"} (or ask the user to create a marker track)
3. Create a Agregator - Notes track {"identifier": "ircammisc:noteagregator", "feature": "notes"} and sets the pitch and marker tracks for the frequencies and transients input tracks
4. Get the summary of results (wait a few seconds if necessary) of the pitch and marker tracks, and adjust the confidence and energy extra thresholds to exclude unwanted results
5. Get a the raw results of the Agregator - Notes track and give the equivalent musical notation and/or MIDI notes.

### Tips: 
* Get the energy results of the marker track to estimate the energy of the notes in dB and/or MIDI velocity
