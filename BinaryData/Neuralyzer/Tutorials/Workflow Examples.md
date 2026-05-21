# Workflow Examples

## Analyze the text of the audio/speech
1. Create a Whisper - Token plugin track [identifier="ircamwhisper:whisper" feature="token"]
2. Define the "splitmode" parameter of the Whisper track to the value '0' (Sentence)
3. Hide the Whisper track from the group
4. Create a VAX - Aligner plugin track [identifier="ircamvax:ircamvaxaligner" feature="text"]
5. Define the "parsingmode" parameter of the VAX track to value '2' (Syllable)
6. Define the "modeltype" parameter of the VAX track to either '0' (Singing) or '1' (Speech) if the audio content is known
7. Define the Whisper track as the input of the VAX track

## Analyze the first N harmonic partials
1. Create N Harmonic Partial - Frequency plugin tracks [identifier="pm2:pm2harmonicpartialtracking" feature="frequency"]
2. Define the "maximumnumberofpartials" parameter of all the Harmonic Partial tracks to value 'N'
3. Define the "partialid" parameter of all the Harmonic Partial tracks by incrementing the value from '1' to 'N'
4. Rename all the Harmonic Partial tracks according to their partial identifier
5. If no pitch track exists, create a Crepe - Pitch plugin track [identifier="ircamcrepe:crepe" feature="pitch"]
6. Define the pitch track as the input of all the Harmonic Partial tracks
7. Create a gradient for the foreground and text colors of the Harmonic Partial tracks

## 