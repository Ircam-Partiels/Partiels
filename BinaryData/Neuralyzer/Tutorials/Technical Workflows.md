# Export the results
Keywords: Export, Save Results, Export Format, Max, Pure Data, Reaper, CSV, JSON, LAB, CUE, SDIF, Image, PNG, JPEG

1. Get the document state and the selected items.
2. If the user hasn't specified the format, ask the user (with the elicit tool) **what the results will be used for** and deduce the format (see **Choose the export format**).
3. If the user hasn't specified the items, ask the user which items to export: the whole document, the selected groups/tracks (propose them by name when a selection exists), or specific groups or tracks (then ask which ones, listed by name).
4. Resolve the items to export (see **Resolve the items to export**).
5. Ask the user for the options of the format that are relevant to the use and to the exported tracks (see **Format options**): first propose to use the default values in one question, and ask for the options one at a time only if the user wants to change them; skip the options already specified by the user.
6. Get the summary of results of the tracks to ensure that the analyses are complete (with `waitSeconds` if the analysis is running).
7. Save the results with the `save_track_results` tool, check the errors, and give the user the paths of the exported files (see **Export tips**).

## Export tips
Keywords: Export Files, Temporary Folder, Time Range, Verify Export

* The files are saved in a temporary folder: tell the user to copy them elsewhere to keep them.
* For image formats, read the exported images to verify the result.
* The whole duration of the document is exported. To export a time range, the user must use the export window of Partiels (`File → Export...`).

## Choose the export format
Keywords: Export Format, Use Case, Max, Pure Data, Reaper, Spreadsheet, Audacity, Reload

Suggest the formats according to the use:
* Max: `max` format (text file for the `coll` object).
* Pure Data: `puredata` format (text file for the `text` object).
* Reaper: `reaper` format (markers or regions to import in the Region/Marker Manager), only for marker (label) tracks.
* Table, spreadsheet, or scripts (Excel, LibreOffice, Python, MATLAB, R...): `csv` format, the most portable.
* Reload in Partiels later: `json` format, less readable but it preserves all the results and, with the extra description, the analysis and graphical properties of the tracks.
* Annotation and speech tools (Audacity labels, HTK...): `lab` format (start and end times instead of durations).
* Cue sheet for audio players, CD burning, or DJ software: `cue` format, only for marker (label) tracks.
* IRCAM tools and SDIF libraries: `sdif` format.
* Figure, report, or visual check: `png` (transparent background) or `jpeg` format.

## Resolve the items to export
Keywords: Export Items, Groups, Tracks, Compatibility, Track Types, Matrix Tracks

* For image formats, a group identifier exports one image of the group with all its visible tracks overlaid, and a track identifier exports one image of the track. Ask the user whether they want one image per group or one image per track.
* For text and binary formats, the results are exported per track: a group identifier exports all the tracks of the group.
* Get the types of the tracks: the `cue` and `reaper` formats only support marker (label) tracks, the other tracks are reported as errors. Inform the user of the excluded tracks.
* Vector tracks (spectrograms, matrices) generate very large text files: ask the user whether to include them. To exclude them, provide the identifiers of the other tracks instead of the group.
* The hidden tracks of a group are exported in text formats but not in group images: ask the user if necessary.
* One file is created per item, named after the group and the track.

## Format options
Keywords: Export Options, Format Options, Export Properties

Ask only for the options of the chosen format, using the default values when the user has no preference.

### Image options (png, jpeg)
Keywords: Image Export, PNG, JPEG, Image Size, Resolution, Grid

* Size: the current size of the plots (`useAutoSize` true) or a specific size (`imageWidth`, `imageHeight`, default 1920 x 1200 pixels).
* Resolution: `imagePpi` (default 144), increase it for print publications.
* Graduations outside the plot: `outsideGridJustification` with `left`, `right`, `top`, and/or `bottom` (default `none`). Propose `left` and `bottom` for figures.
* Background: `png` preserves the transparency, `jpeg` uses the background color of the theme.

### Text options (csv, lab, puredata, max, json, cue, reaper)
Keywords: CSV Export, LAB Export, JSON Export, Reaper Export, Pure Data Export, Max Export, Extra Thresholds, Header, Separator

* All text formats: `applyExtraThresholds` (default false) only exports the results that meet the extra thresholds of the tracks (e.g. confidence). Propose it when the tracks have extra thresholds.
* CSV: `csvHeaderType` (`none`, `generic` with generic column names, `specific` with the names of the plugin outputs, default `generic`), `columnSeparator` (`comma` by default, `tab` or `space` for some software, `colon`, `pipe`, `slash`), and `disableLabelEscaping` (default false, keep the quotes around labels unless the software doesn't support them).
* LAB: `labSeparator` (`space` by default, `tab` for Audacity labels) and `disableLabelEscaping`.
* JSON: `includeDescription` (default true), keep it to restore the analysis and graphical properties when reloading the file in Partiels.
* Reaper: `reaperType` `marker` (time points, default) or `region` (time segments using the durations).
* Pure Data, Max, and CUE: no specific option.

### SDIF options
Keywords: SDIF Export, Frame Signature, Matrix Signature, Column Name

* `sdifFrameSignature` and `sdifMatrixSignature` are required: 4-character signatures without `?` (e.g. `1FQ0` for a fundamental frequency). Ask the user for the signatures expected by the target application, never invent them.
* `sdifColumnName` is optional.
* The extra thresholds are not applied to SDIF exports.
