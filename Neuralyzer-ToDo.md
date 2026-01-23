# Neuralyzer TODO list

## Beta 2
- [] Clear the cache of loop prompts
- [x] Reset must simply clear all the cache
- [] Save/restore the history
- [x] Control temperature and other inference parameters
- [] Add access to the manual, tutorials, etc. via MCP
- [] Add acces to internet website via MCP 
- [] Add access to internet Vamp plugins
- [] Add prompt for specific approach in MCP 
- [] Ask to undo/redo/fix the previous prompt
- [x] Add support for Graphics properties via MCP
- [x] Add support for groups via MCP
- [x] Provide information about the audio file via MCP
- [] Improve batch manahement
- [x] Use Qwen3.3 instead of 2.5
- [x] Add show track in group
- [] Add methods for the zooms
- [] Add methods for the extra values
- [] Check the available memory size of the system
- [x] Add Markdwon parser
- [x] Add support for Copilot
- [x] Remove unnecessary context management
- [ ] Add max model size

### Sequence 1

Create a track with a text/audio alignment plugin.
Use the Whisper track as the input of the VAX track.
Modify the VAX track so that the text is split by syllables.
Create a track with a text/audio alignment plugin, use the Whisper track as its input, then use syllables for the parsing.
Improve the names of the tracks.
I want a track to synchronize the syllables of the Whisper track on the audio
Create 10 harmonic partials tracks (with partial id from 1 to 11) using the pitch track as input
