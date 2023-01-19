<h1 align="center">Partiels Manual</h1>

<p align="center">
<i>Version 1.0.6 for Windows, Mac & Linux</i><br>
<i>Manual by Pierre Guillot</i><br>
<a href="www.ircam.fr">www.ircam.fr</a><br><br>

<img src="Images/overview.png" alt="Example" width="1840"/>
</p>

## Table of contents

* [1. Introduction](#1-introduction)
  * [1.1. System requirements](#11-system-requirements)
  * [1.2. Vamp plug-ins](#12-vamp-plug-ins)
* [2. Overview](#2-overview)
* [3. Create and open documents](#3-create-and-open-documents)
* [4. Create new tracks](#4-create-new-tracks)
  * [4.1. Analysis tracks](#41-analysis-tracks)
  * [4.2. File tracks](#42-file-tracks)
* [6. Organize the tracks by groups](#6-organize-the-tracks-by-groups)
  * [6.1. Edition](#61-edition)
  * [6.2. Layout](#62-layout)
* Export results and images - To do
* Save and consolidate documents - To do
* [8. Track properties](#8-track-properties)
  * [8.1. Processor](#81-processor)
  * [8.2. Graphical](#82-graphical)
  * [8.3. Plugin](#83-plugin)
* Group properties - To do
* [10. Audio files layout](#10-audio-files-layout)
  * [10.1. Audio files information](#101-audio-files-information)
  * [10.2. Audio files configuration](#102-audio-files-configuration)
  * [10.3. Audio files recovery](#103-audio-files-recovery)
* [11. Audio settings](#11-audio-settings)
* Plugins management - To do
* Command-line interface - To do
* [14. Credits](#14-credits)

<div style="page-break-after: always;"></div>

## 1. Introduction

**Partiels** is an application for the **analysis of digital audio files** intended for *researchers* in signal processing, *musicologists*, *composers*, and *sound designers*. It offers a dynamic and ergonomic interface in order to **explore the content and the characteristics of sounds**.

Partiels allows **analysis** one or several audio files using *Vamp* plug-ins, **loading** data files, **editing**, **organizing** and **visualizing** the analyses inside groups, and **exporting** the results as images or text files (in order to be used in other applications such as *Max*, *Pure Data*, *Open Music*, etc.).

- Versions for *Windows*, *Mac* & *Linux*  
- Multiformat & multichannel support  
- Multifile support (useful to compare audio files)  
- Analyzing audio with *Vamp* plug-ins  
- Visualizing results as *spectrogram*, *lines*, and *markers*  
- Drawing and editing results (copy/cut/paste/delete/duplicate/etc.)  
- Organizing and ordering analyses in groups  
- Exporting results to *PNG*, *JPEG*, *CSV*, *JSON*, *CUE* & *SDIF* file formats  
- Loading results from *CSV*, *JSON*, *CUE* & *SDIF* file formats  
- Converting results from *SDIF* to *JSON* file formats and inversely  
- Batch processing (useful to apply the same set of analyses on several audio files)  
- Command line interface to analyze, export and convert results  
- Consolidating documents (useful to share documents/analysis with other users across different platforms)  
- Audio playback with loop  

### 1.1. System Requirements

- MacOS 10.13 and higher (64bit - Universal Intel/Silicon)
- Linux (64 bit)
- Windows 10 and 11 (64 bit).

### 1.2. Vamp Plug-ins

[Vamp](https://www.vamp-plugins.org/) is an audio processing plug-in system for plug-ins that extract descriptive information from audio data developed at the Centre for Digital Music, Queen Mary, University of London.*

Alongside Partiels, a set of analyses present in the AudioSculpt 3 application and based on the audio engines developed by the Ircam Analysis-Synthesis team are ported as Vamp plug-ins: 
- [SuperVP](https://forum.ircam.fr/projects/detail/supervp-vamp-plugin/)
- [IrcamBeat](https://forum.ircam.fr/projects/detail/ircambeat-vamp-plugin/)  
- [IrcamDescriptor](https://forum.ircam.fr/projects/detail/ircamdescriptor-vamp-plugin/) 
- [PM2](https://forum.ircam.fr/projects/detail/pm2-vamp-plugin/)

<div style="page-break-after: always;"></div>

## 2. Overview

<p align="center"><img src="Images/section-0.1-v1.0.6.jpg" alt="Example" width="1250"/></p>

## 3. Create and open documents

When you open Partials for the first time or create a new document via the main menu `File → New...` or with the keyboard shortcut `⌘ Cmd + N` (Mac) or `Ctrl + N` (Linux/Windows), the application presents the interface for starting a new document. 

<p align="center"><img src="Images/section-create-and-open-documents.1-v1.0.6.png" alt="Example" width="412"/></p>

You are prompted to load one or more audio files or pre-existing Partiels document file. The application supports audio file extensions *.aac*, *.aiff*, *.aif*, *.flac*, *.m4a*, *.mp3*, *.ogg*, *.wav* and *.wma*. A Partiels document file has the extension *.ptldoc*. You can use the file browser window by clicking on the `Load` button (**A1**) or drag the files directly into the interface (**A1**). You can also load a Partiels document file via the main menu `File → Open...` or with the keyboard shortcut `⌘ Cmd + O` (Mac) or `Ctrl + O` (Linux/Windows). And you can select one of the recently used Partiels documents from the list on the right (**A2**) or via the main menu `File → Open Recent`.

If you have loaded one or more audio files, a second interface prompts you to add a new analysis track by clicking on the `Add Track` button (**A3**) to display the analysis plugin loading window. Alternatively, you can also use a pre-existing Partial document as a template. Click on the `Load Template` button (**A4**) to display the file browser window or select one of the recently used Partiels documents from the list on the right (**A2**).

<p align="center"><img src="Images/section-create-and-open-documents.2-v1.0.6.png" alt="Example" width="412"/></p>

If you use a Partiels document as a template, all the analyses in this document will be applied to the audio files you have selected. The *block size* and *step size* analysis parameters can be adapted automatically by activating the corresponding toggle button (**A6**) if the sample rate used in the initial document differs from that of your audio files.

If you add a new track with an analysis plugin, you can refer to the [next section](#add-new-plugin-tracks) of this manual.

## 4. Create new tracks

A track contains and displays data from a sound analysis. This data can be of three different types, each with its own representation:

<p align="center"><img src="Images/section-tracks.1-v1.0.6.png" alt="Example" width="536"/></p>

- Matrix: A time point with a duration, and a vector of numerical values (e.g. the data generated by an FFT analysis). Matrices are represented by an image where the horizontal axis corresponds to time, the vertical axis to a vector and the color varies according to the numerical value of the elements of the vector (**1**). 
- Marker: A time point with a duration, and potentially a text label (e.g. data generated by a transient detection). Each marker is represented by a vertical line whose horizontal position depends on the time and thickness of the duration with potentially the adjacent label text (**2**). 
- Point: a time point with a duration, and potentially a numerical value (e.g. the data generated by an estimation of the fundamental frequency). The points define segments whose horizontal position depends on the time and vertical position depends on the numerical value. A point with a non-zero duration defines a horizontal line whose length depends on the duration. A point without numerical value stops the succession of segments (**3**).  

Tracks can be created in two ways, from the analysis of a Vamp plug-in or from a result file.

## 4.1. Analysis tracks

When you click on the button to add analysis tracks via the interface for starting a new document, the window for adding analysis plugins is displayed. If you are already working on a document, you can bring up this window to add new tracks via the main menu `Edit → Add New Track`, the keyboard shortcut `⌘ Cmd + T` (Mac) or `Ctrl + T` (Linux/Windows), or by using the drop-down menu of the `+` button ([Overview](#overview) - **K1**) on the main interface. 

All plug-ins installed on your computer are listed in the interface. You can organize the plug-ins by name, feature, maker, category or version in ascending or descending alphabetical order by clicking on the corresponding entry in the interface header (**P1**).

<p align="center"><img src="Images/section-add-new-plugin-tracks.1-v1.0.6.png" alt="Example" width="416"/></p>

You can also search for specific plug-ins by clicking on the text filter entry (**P2**) or by using the keyboard shortcut `⌘ Cmd + F` (Mac) or `Ctrl + F` (Linux/Windows), then typing the keyword corresponding to the plug-ins you wish to display (e.g. *tempo* or *spectrogram* for the feature or *ircam* for the manufacturer).

You can select one or more plug-ins and then press the `Return ⏎` key (or you can simply double click on a plug-in) to add a new track for each plug-in in your document (**P3**). If you are working on a new blank document, a default group is automatically created in which your new tracks appear. If you are working on a document that already contains groups and tracks, the new tracks are inserted after the last selected track or in the last selected group.

If you want to modify the analysis and graphical properties of a track, please refer to the [Track properties](#track-properties) section.

## 4.2. File tracks

A new track can be created by importing pre-calculated results from a file. This file can be generated from another Partiels document to avoid recalculating the analyses or from another application. The supported formats are JSON (*.json*), CSV (*.csv*), CUE (*.cue*) and SDIF (*.sdif*). You can bring up the file browser to select a file via the main menu `File → Import...` or with the keyboard shortcut `⌘ Cmd + ⇧ Shift + I` (Mac) or `Ctrl + Shift + I` (Linux/Windows). Once your file is selected, the `Load File...` window appears, allowing you to set the unit and range of the imported values. 

<p align="center">
<img src="images/selection-import-new-result tracks.1-v1.0.6.png" width="348"/>
</p>

If you are importing a JSON file containing an extra description of the analysis used to calculate the analysis, the default unit and range of values will be taken from that data. In addition, the extra descriptions are saved so that the original analysis can be re-run if necessary.

<p align="center">
<img src="images/selection-import-new-result tracks.2-v1.0.6.png" width="348"/>
</p>

If you are importing an SDIF file, you will be able to define the frame code, matrix code, row and column to import.

If you are working on a new blank document, a default group is automatically created in which your new tracks appear. If you are working on a document that already contains groups and tracks, the new tracks are inserted after the last selected track or in the last selected group.

## 6. Organize the tracks by groups

The analyses of a document are represented by tracks. These analysis tracks are organized in groups. Each group can contain several tracks, allowing the analysis representations to be superimposed. In a group, the tracks are superimposed from bottom to top. 

<p align="center">
<img src="images/section-organize-the-tracks-by-groups.1-v1.0.6.png" width="416"/>
</p>

It is therefore often preferable to place matrix type analyses, such as a sonogram, at the bottom and point or marker type analyses, such as spectral centroid or transient detection, on top. Thus the lines and markers will be displayed above the matrices.

## 6.1. Edition

Move tracks and groups by clicking on the track or group header to select it and then dragging the item to the desired location. In this way you can reorder tracks within a group, move a track within another group and reorder groups between themselves. 

<p align="center">
<img src="images/section-organize-the-tracks-by-groups.2-v1.0.6.png" width="416"/>
</p>

Use the keyboard modifier `Ctrl` during the move action of the selected track or group to duplicate it and move the copy of the item to the desired location.

> Selecting a track or a group with the keyboard modifier `Ctrl` without moving it allows you to quickly create a copy of the item that will be placed just above it. Then you can modify the analysis parameters of the copy to compare the results, for example.

Select a group or track by clicking on the item's header (or multiple tracks in a group by using the keyboard modifier `⇧ Shift`), then use the keyboard shortcuts `⌦ Delete` or `⌫ Backspace` to delete it.

Use the main menu `Edit → Add New Group`, the keyboard shortcuts `⌘ Cmd + G` (Mac) or `Ctrl + G` (Linux/Windows), or the drop-down menu of the `+` button ([Overview](#overview) - **K1**) on the main interface to create a new group that will be inserted after the last selected item.

## 6.2. Layout

Use the button (**K1**) on the header of a group to expand the group and show all the tracks of the group (e.g., *Group 2*) or conversely, to shrink the group and hide all the tracks of the group (e.g., *Group 1*). Use the button (**K2**) to expand or shrink all the groups in the document.

<p align="center">
<img src="images/section-organize-the-tracks-by-groups.3-v1.0.6.png" width="416"/>
</p>

The height of tracks and groups is variable and depends on two modes, manual (default) or automatic. Click on the button (**K3**) to activate or deactivate the automatic or manual mode. 

In automatic mode, the height of the tracks and groups will always be adapted according to the organization of the groups and tracks in the document and the height of the main window. 

In manual mode, the height of tracks and groups is free, click and drag on the bottom border of tracks and groups to resize them (**K4**). 

> In manual mode, you can still optimize the height of tracks and groups to the height of the main window by clicking the button (**K3**) with the keyboard modifier `⇧ Shift`.

## 8. Track properties

The track properties panel allows you to access information about a track and to change the processor and graphic properties. You can display a track's properties panel by clicking on the corresponding button in the track's header ([Overview](#overview) - **K2**) or via the corresponding entry in the drop-down menu displayed by the properties button of the group header containing the track ([Overview](#overview) - **K3**).

<p align="center">
<img src="images/section-track-properties.1-v1.0.5.png" width="232"/>
</p>

The first property of a track is its name. The default name of the track corresponds to either the name of the analysis plugin if a plugin has been used to create the track or to the name of the results file if a results file has been used to create the track. The name of the track can be modified using the corresponding text field. The properties of a track are then organized in three categories: processor, graphical, plugin.  

### 8.1. Processor

The processor section allows you to modify all the properties that control the analysis engine or the loading of the results file of the track. Thus, the properties are related to the result file, the plugin parameters and the track state.

#### 8.1.1. File

The results file information is displayed if the analysis results have been loaded from a file. Note that, if the analysis results were loaded from a JSON file containing plugin information or a file resulting from the consolidated track, the processor's parameters can still be accessible and used to update the analysis results.

<p align="center">
<img src="images/section-track-properties.2-v1.0.5.png" width="232"/>
</p>

- The results file can be revealed in the operating system's file browser by clicking on the text button. 

- The results file can be detached from the track (only if the track contains plugin information) by clicking on the text button with the `shift` keys pressed. In this case, the track will use the plugin information to run the analysis and produce new results.

#### 8.1.2. Parameters 

Depending on the characteristics of the plugin, the section allows accessing the default plugin parameters. If the plugin expects frequency domain input signal, the section gives access to the window type, the block size and the step size in order to transform the time domain signal into a frequency domain signal with a Fast Fourier Transform. Otherwise, if the plugin expects time domain input signal, the section gives access to the block type if the plugin doesn't have a preferred block size, the step size if the plugin doesn't have a preferred step size or doesn't expect the step size to  be equal to the block size.

<p align="center">
<img src="images/section-track-properties.3-v1.0.5.png" width="232"/>
</p>

- Window Type: The window function of the FFT in order to transform the time domain signal into a frequency domain signal if the plugin expects frequency domain input signal.

- Block Size: The window size of the FFT in order to transform the time domain signal into a frequency domain signal signal if the plugin expects frequency domain input data or the size of the blocks of samples if the plugin expects time domain input signal. 

- Step Size: The step size of the FFT in order to transform the time domain signal into a frequency domain signal if the plugin expects frequency domain signal or the size between each block of samples if the plugin expects time domain input signal. 

The section allows accessing the plugin-specific parameters that are used to perform the analysis. Depending on their specifications, the values of the parameters can be controlled by a toggle button (on/off), a number field (integer or floating-point values), or a dropdown menu (list of items). Modifying the parameters triggers the analysis and produces new results. If the track is loaded from a file or if the analysis results have been edited, the application displays a dialog window warning you that the file will be detached from the track before performing the analysis and asking you if you want to proceed (you can still undo the operation to retrieve the previous analysis results). 

<p align="center">
<img src="images/section-track-properties.4-v1.0.5.png" width="196"/>
</p>

The preset menu allows you to restore the factory value of the parameters. You can also save the current state of the parameters in a file on your operating system and load this file to restore the state (the file can be used in any other track that used the same plugin).

#### 8.1.3. State

The current state of the plugin is displayed at the bottom of the section informing if the analysis or loading succeeded or failed and if the results have been edited.

<p align="center">
<img src="images/section-track-properties.5-v1.0.5.png" width="196"/>
</p>

If the analysis or loading failed, clicking on the warning icon prompts a dialog window offering to solve the problem when possible (by loading another plugin or another file if it is not found or by restoring the default state of the plugin if its initialization failed).

### 8.2. Graphical

The graphical section allows modifying all the properties that control the rendering of the analysis results of the track. The properties depend on the type of analysis results: markers (e.g. beat detection), points (e.g. energy estimation), or columns (e.g. spectrogram).

#### 8.2.1. Colors

A color can be modified using the color selector that is prompted by clicking on the colored button. You can drag and drop a colored button on another one, to copy the color from one property to another. The color map can be modified using the dropdown menu. 

<p align="center">
<img src="images/section-track-properties.6.a-v1.0.5.png" width="196"/><br>
<img src="images/section-track-properties.6.b-v1.0.5.png" width="196"/>
</p>

- Color Map: The color map used to render the columns.
- Foreground Color: The color used to render the markers or the segments between points.
- Background Color: The color used to render the background behind the markers or the segments between points.
- Text Color: The color used to render the labels of the markers or the points values.
- Shadow Color: The shadow color used to render the shadow of the markers or the segments between points.

#### 8.2.2. Ranges

The value range of the points and the columns results can be adapted to optimize the graphical rendering. It will corresponds to the range of the vertical axe for the points rendering or to the color mapping for the column rendering. By default, the value range is based on the information given by the plugin or stored in the results file if available, otherwise it will be based on the minimum and maximum values of the results but it can also be modified manually.

<p align="center">
<img src="images/section-track-properties.7-v1.0.5.png" width="196"/>
</p>

- Value Range Mode: Select the value range mode: default (if available), results, manual (selected automatically when the range doesn't match one of the two other mode)
- Value Range Min.: The minimum possible value for the results (the value differs from the visible value that corresponds to the zoom on the vertical axes for the points rendering or the color mapping for the column rendering).
- Value Range Max.: The maximum possible value for the results (the value differs from the visible value that corresponds to the zoom on the vertical axes for the points rendering or the color mapping for the column rendering).
- Value Range Link: If the visible value range of the points corresponding to the vertical axe is linked to the the vertical axe of the group containing the track.  
- Value Range: The visible value range for the column rendering (the range controls the color mapping).
- Num. Bins: The maximum number of bins (of values) by column (the value is fixed). 
- Bin Range Link: If the visible bin range of the columns corresponding to the vertical axe is linked to the the vertical axe of the group containing the track. 
- Grid: Prompt a dialog window to control the grid properties of the track
  - Tick Reference Value: The reference value from which the ticks are distributed.
  - Main Tick Interval: The number of small ticks between main ticks. 
  - Power Base: The power base used to compute the tick distribution.
  - Ruler Mode: A set of presets for the main tick interval and the power base that can be used to easily corresponds to amplitude range, frequency range, etc.
  
#### 8.2.3. Channels

The channel button allows to show or hide channels of the analysis results. This channels layout is only graphical and doesn't change the audio analysis nor the audio playback (please refer to the audio files layout section).

### 8.3. Plugin

The graphical section displays information about the plugin such as the name, the feature, the maker, the version, the  markers, the category and the description and the copyright.

## 10. Audio files layout

The audio files layout panel allows you to access information about the audio files of the document, reveal the audio files in the operating system's file browser, and rearrange the files configuration of the audio reader. You can display the audio files layout panel by clicking on the corresponding button in the main interface ([Overview](#overview) - **K4**).

<p align="center">
<img src="images/section-audio-files-layout.1-v1.0.5.png" width="383"/>
</p>  

### 10.1. Audio files information

Select an audio file by clicking on its name in section (**S1**) to display its information (format, sample rate, bits, length in samples, duration in seconds, number of channels, metadata, etc.) in section (**S2**) of the panel. The audio file can be revealed in the operating system's file browser by clicking on the text button on top of section (**S2**).

### 10.2. Audio files configuration

The audio files layout is used by both the audio playback engine and the audio analysis engine so modifying the audio files layout triggers the reanalysis of all the analysis tracks that have not been consolidated. 

> If you need a different channel routing for audio playback than for audio analysis (to mute some channels for example), you can use the routing matrix of the [audio settings](#audio-settings).  

An item of the audio files layout in section (**S1**) is defined by an index, an audio file and a channel configuration. Each item corresponds to a channel of the audio files layout. So different audio files can be assigned to the different channels of the audio files layout using for each a specific channel layout. 

- Add items: Click on the `+` button to select one or several files using the file browser window or drag and drop one or several from the operating system's file browser. A new item is created for each channel of a new file,  (so adding a stereo audio file, creates two new items assigned to channels one and two of the audio file). ⚠️ Using audio files with different sample rates is not supported and may lead to invalid analysis results and poor audio playback, the app will warn you to avoid such issues.

- Delete items: Select one or several items (with the `ctrl` or `shift` keys) by clicking on the name part of the item and press the `delete ⌫` key to remove items from the audio files layout.

- Move an item: Click on the `#` index part of the item, drag the item to a new position above or below and drop the item to reorder the audio file configuration and update the indices.
<p align="center">
<img src="images/section-audio-files-layout.2-v1.0.5.png" width="243"/>
</p>

- Copy an item: Click on the `#` index part of the item with the `ctrl` key pressed to duplicate an item, then drag and drop to insert the new item the desired position.

- Modify an item's channels: Click on the channel drop-down menu and select the desired configuration (*mono* sums all channels into one channel).  
<p align="center">
<img src="images/section-audio-files-layout.3-v1.0.5.png" width="243"/>
</p>

- Undo changes: Use the undo key command `ctrl or ⌘ + Z` and  to restore previous states. Note the undo/redo mechanism is relative to this panel only (until the changes are applied to the document).

- Redo changes: Use the redo key commands `ctrl or ⌘ + Y` or `ctrl or ⌘ + shift ⇧+ Z` to reapply changes. Note the undo/redo mechanism is relative to this panel only (until the changes are applied to the document).

- Apply changes: If the audio files layout differs from the one currently used by the document, click the `Apply` text button in section (**S3**) to apply the new audio files layout to the document. Note that when the changes are applied to the document, the undo/redo mechanism can be used in the document to restore the previous state of the audio files layout.

- Reset changes: If the audio files layout differs from the one currently used by the document, click the `Reset` text button in section (**S3**) to restore the audio files layout of the document. 

### 10.3. Audio files recovery

If audio files cannot be found (files have been moved or deleted for example), the item names corresponding to the missing files are greyed out and a warning appears at the bottom of the (**S1**) section. 

<p align="center">
<img src="images/section-audio-files-layout.4-v1.0.5.png" width="232"/>
</p>

Click on an item's name to display a dialog window asking you to recover the missing audio file using the operating system's file browser. 

## 11. Audio settings

The audio settings window allows you to control device management and channels routing for audio playback. These settings have no effect on the analysis or graphics rendering. The audio settings window can be displayed via the main menu `Partials → Audio Settings...` (Mac) or `Help → Audio Settings...` on (Linux/Windows) or with the keyboard shortcut `⌘ Cmd + ,` (Mac) or `⌘ Cmd + P` (Linux/Windows).

<p align="center">
<img src="images/section-audio-settings.1-v1.0.6.png" width="348"/>
</p>

- Driver (**A1**): The dropdown menu allows to select the audio driver from those available on the computer (typically, CoreAudio on Mac, WASAPI, DirectSound, and ASIO on Windows, ALSA, and Jack on Linux).  
- Output Device (**A2**): The dropdown menu allows to select the audio output device from those connected to the computer and supported by the driver.
- Sample Rate (**A3**): The dropdown menu allows to select the sample rate from those supported by the output device. Higher sample rates increase the audio quality in high frequencies at the expense of increased CPU drain.
- Buffer Size (**A4**): The dropdown menu (Mac) or the number entry (Windows & Linux) allows selecting the buffer size from those supported by the output device. Lower buffer size reduces the latency at the expense of increased CPU drain. If dropouts or crackle occur during playback, the buffer size should be reduced.
- Channels Routing (**A5**): The button matrix allows you to configure to which output channels the application's channels are sent. The number of application channels depends on the [audio files layout](#audio-files-layout). The number of output channels depends on the selected audio device. The matrix can be used to mute some channels of the audio files layout.
- Audio Device Panel (**A6**): The Audio Device Panel button opens the audio device panel of the manufacturer if supported by the audio driver.

When changed, the audio settings are saved automatically on the computer and restored when reopening the application. The audio settings can be deleted manually to restore the default configuration when reopening the application. The audio settings are located:
- Mac: *~/Library/Application Support/Ircam/Partiels.audio.settings*
- Window: *C:\\Users\\"username"\\AppData\\Roaming\\Ircam\\Partiels.audio.settings*
- Linux: *~/Ircam/Partiels.audio.settings*

## 14. Credits

**Partiels** is designed and developed by Pierre Guillot at [IRCAM](https://www.ircam.fr/) IMR Department.  
**Vamp** is designed and developed at the Centre for Digital Music, Queen Mary, University of London.
