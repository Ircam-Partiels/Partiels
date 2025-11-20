## **AVAILABLE INFORMATION**

### 2. **Research article (LaTeX)** describing

You will use these source to:

* the context
* the objectives
* capabilities of the Partiels application
* challenges of analysis and exploration
* bibliography

---
```latex
% Template LaTeX file for DAFx-25 papers
%
% To generate the correct references using BibTeX, run
%     latex, bibtex, latex, latex
% modified...
% - from DAFx-00 to DAFx-02 by Florian Keiler, 2002-07-08
% - from DAFx-03 to DAFx-04 by Gianpaolo Evangelista, 2004-02-07 
% - from DAFx-05 to DAFx-06 by Vincent Verfaille, 2006-02-05
% - from DAFx-06 to DAFx-07 by Vincent Verfaille, 2007-01-05
%                          and Sylvain Marchand, 2007-01-31
% - from DAFx-07 to DAFx-08 by Henri Penttinen, 2007-12-12
%                          and Jyri Pakarinen 2008-01-28
% - from DAFx-08 to DAFx-09 by Giorgio Prandi, Fabio Antonacci 2008-10-03
% - from DAFx-09 to DAFx-10 by Hannes Pomberger 2010-02-01
% - from DAFx-10 to DAFx-12 by Jez Wells 2011
% - from DAFx-12 to DAFx-14 by Sascha Disch 2013
% - from DAFx-15 to DAFx-16 by Pavel Rajmic 2015
% - from DAFx-16 to DAFx-17 by Brian Hamilton 2016
% - from DAFx-17 to DAFx-18 by Annibal Ferreira and Matthew Davies 2017
% - from DAFx-18 to DAFx-19 by Dave Moffat 2019
% - from DAFx-19 to DAFx-20-21-22 by Gianpaolo Evangelista 2019-21
% - from DAFx-20-21-22 to DAFx-23 by Federico Fontana 2022-11-25
% - from DAFx-23 to DAFx-24 by Matteo Scerbo 2023-10-05
% - from DAFx-24 to DAFx-25 by Leonardo Gabrielli 2024-12-13
%
% Template with hyper-references (links) active after conversion to pdf
% (with the distiller) or if compiled with pdflatex.
%
% 20060205: added package 'hypcap' to correct hyperlinks to figures and tables
%                      use of \papertitle and \paperauthorA, etc for same title in PDF and Metadata
% 20190205: Package 'hypcap' removed, and replaced with 'caption', to allow for the inclusion
%		       of a CC UP licence.
% 20221125: Package 'nimbusserif' commented. PDF test template generated with
%                      pdfTeX 3.14159265-2.6-1.40.20 (TeX Live 2019/Debian) kpathsea version 6.3.1
%
% 1) Please compile using latex or pdflatex.
% 2) If using pdflatex, you need your figures in a file format other than eps! e.g. png or jpg is working
% 3) Please use "paperftitle" and "pdfauthor" definitions below

%------------------------------------------------------------------------------------------
%  !  !  !  !  !  !  !  !  !  !  !  ! user defined variables  !  !  !  !  !  !  !  !  !  !  !  !  !  !
% Please use the following commands to define title and author(s) of the paper.
% paperauthorA MUST be the the first author of the paper
% Please comment the unused definitions 
\def\papertitle{Partiels - Exploring, Analyzing and Understanding Sounds}
\def\paperauthorA{Pierre Guillot}
%\def\paperauthorB{Author Two}
%\def\paperauthorC{Author Three}
%\def\paperauthorD{Author Four}
%\def\paperauthorE{Author Five}
%\def\paperauthorF{Author Six}
%\def\paperauthorG{Author Seven}
%\def\paperauthorH{Author Eight}
%\def\paperauthorI{Author Nine}
%\def\paperauthorJ{Author Ten}

% Authors' affiliations have to be set below

%------------------------------------------------------------------------------------------
\documentclass[twoside,a4paper]{article}
\usepackage{etoolbox}
\usepackage{dafx_25}
\usepackage{amsmath,amssymb,amsfonts,amsthm}
\usepackage{euscript}
%\usepackage[latin1]{inputenc}
\usepackage[T1]{fontenc}
\usepackage[utf8]{inputenc}
%\usepackage[T1]{fontenc}
%\usepackage{lmodern}
%\usepackage{nimbusserif}
\usepackage{ifpdf}
\usepackage[english]{babel}
\usepackage{caption}
\usepackage{subfig} % or can use subcaption package
\usepackage{color}

\input glyphtounicode
\pdfgentounicode=1

\setcounter{page}{1}
\ninept

% build the list of authors and set the flag \multipleauth to handle the et al. in the copyright note (in DAFx_24.sty)
%==============================DO NOT MODIFY =======================================
\newcounter{numauth}\setcounter{numauth}{1}
\newcounter{listcnt}\setcounter{listcnt}{1}
\newcommand\authcnt[1]{\ifdefined#1 \stepcounter{numauth} \fi}

\newcommand\addauth[1]{
\ifdefined#1 
\stepcounter{listcnt}
\ifnum \value{listcnt}<\value{numauth}
\appto\authorslist{, #1}
\else
\appto\authorslist{~and~#1}
\fi
\fi}
%======DO NOT MODIFY UNLESS YOUR PAPER HAS MORE THAN 10 AUTHORS========================
%==we count the authors defined at the beginning of the file (paperauthorA is mandatory and already accounted for)
\authcnt{\paperauthorB}
\authcnt{\paperauthorC}
\authcnt{\paperauthorD}
\authcnt{\paperauthorE}
\authcnt{\paperauthorF}
\authcnt{\paperauthorG}
\authcnt{\paperauthorH}
\authcnt{\paperauthorI}
\authcnt{\paperauthorJ}
%==we create a list of authors for pdf tagging, for example: paperauthorA, paperauthorB, ... and paperauthorF (last author)
\def\authorslist{\paperauthorA}
\addauth{\paperauthorB}
\addauth{\paperauthorC}
\addauth{\paperauthorD}
\addauth{\paperauthorE}
\addauth{\paperauthorF}
\addauth{\paperauthorG}
\addauth{\paperauthorH}
\addauth{\paperauthorI}
\addauth{\paperauthorJ}
%====================================================================================

\usepackage{times}
% Saves a lot of ouptut space in PDF... after conversion with the distiller
% Delete if you cannot get PS fonts working on your system.


% pdf-tex settings: detect automatically if run by latex or pdflatex
\newif\ifpdf
\ifx\pdfoutput\relax
\else
   \ifcase\pdfoutput
      \pdffalse
   \else
      \pdftrue
   \fi
\fi

\ifpdf % compiling with pdflatex
  \usepackage[pdftex,
    pdftitle={\papertitle},
    pdfauthor={\authorslist},
    pdfsubject={Proceedings of the 28th International Conference on Digital Audio Effects (DAFx25)},
    colorlinks=false, % links are activated as color boxes instead of color text
    bookmarksnumbered, % use section numbers with bookmarks
    pdfstartview=XYZ % start with zoom=100% instead of full screen; especially useful if working with a big screen :-)
  ]{hyperref}
  \pdfcompresslevel=9
  \usepackage[pdftex]{graphicx}
 % \usepackage[figure,table]{hypcap}
\else % compiling with latex
  \usepackage[dvips]{epsfig,graphicx}
  \usepackage[dvips,
    pdftitle={\papertitle},
    pdfauthor={\authorslist},
    pdfsubject={Proceedings of the 28th International Conference on Digital Audio Effects (DAFx25)},
    colorlinks=false, % no color links
    bookmarksnumbered, % use section numbers with bookmarks
    pdfstartview=XYZ % start with zoom=100% instead of full screen
  ]{hyperref}
  % hyperrefs are active in the pdf file after conversion
  %\usepackage[figure,table]{hypcap}
\fi
\usepackage[hypcap=true]{caption}
\title{\papertitle}

%-------------SINGLE-AFFILIATION SINGLE-AUTHOR HEADER STARTS (uncomment below if your paper has a single author)----------------------------------------
\affiliation{
\paperauthorA\,\thanks{\vspace{-3mm}}}
 {\href{https://www.ircam.fr/}{IRCAM - Centre Pompidou}\\STMS UMR 9912\\Paris, France\\
{\tt \href{mailto:pierre.guillot@ircam.fr}{pierre.guillot@ircam.fr}}}
%
%Please note that the copyright notice should be separated from the text by a line (like a footnote). This works automatically when you have an \sthanks command 
%in the authors' line. However, if your paper does not require an \sthanks command, please use an empty (vertical space eating) \thanks command as follows:
% \affiliation{
% \paperauthorA\,\thanks{\vspace{-3mm}}}
% {\href{https://dafx25.dii.univpm.it/}{Dept. of Information Engineering} \\ Universit\`a Politecnica delle Marche \\ Ancona, IT\\
% {\tt \href{mailto:dafx25@dii.univpm.it}{dafx25@dii.univpm.it}}
% }
%-------------SINGLE-AFFILIATION SINGLE-AUTHOR HEADER ENDS-------------------------------------------------------------------------------------------------------------------
\hyphenpenalty=100000
\begin{document}
% more pdf-tex settings:
\ifpdf % used graphic file format for pdflatex
  \DeclareGraphicsExtensions{.png,.jpg,.pdf}
\else  % used graphic file format for latex
  \DeclareGraphicsExtensions{.eps}
\fi

%\makeatletter
%\pdfbookmark[0]{\@pdftitle}{title}
%\makeatother

\maketitle
\begin{sloppypar}
\begin{abstract}
This article presents Partiels, an open-source application developed at IRCAM to analyze digital audio files and explore sound characteristics. The application uses Vamp plug-ins to extract various information on different aspects of the sound, such as spectrum, partials, pitch, tempo, text, and chords. Partiels is the successor to AudioSculpt, offering a modern, flexible interface for visualizing, editing, and exporting analysis results, addressing a wide range of issues from musicological practice to sound creation and signal processing research. The article describes Partiels' key features, including analysis organization, audio file management, results visualization and editing, as well as data export and sharing options, and its interoperability with other software such as Max and Pure Data. In addition, it highlights the numerous analysis plug-ins developed at IRCAM, based in particular on machine learning models, as well as the IRCAM Vamp extension, which overcomes certain limitations of the original Vamp format.
\end{abstract}

\section{Introduction}
\label{sec:intro}
Partiels\footnote{\url{https://github.com/Ircam-Partiels/Partiels}} is an application for analyzing digital audio files, designed to provide a dynamic, ergonomic interface for exploring the content and characteristics of sound (spectrum, fundamental frequency, tempo, chords, spectral centroid, speech analysis, etc.). The Partiels application is available for Windows, macOS, and Linux.

The application is based on the Vamp analysis plug-in format, developed at C4DM (Centre for Digital Music) - Queen Mary University of London. In parallel with the development of the main application, Vamp plug-ins were developed using IRCAM audio engines (notably SuperVP \cite{Depalle:91}, PM2 \cite{Depalle:93}\cite{Röbel:06}, IrcamBeat \cite{Peeters:07} and IrcamDescriptors \cite{Peeters:04}) and external technologies (notably Whisper \cite{Radford:22} and Crepe \cite{Kim:18}). Moreover, Partiels lets you use all existing Vamp analysis plug-ins (such as those from the BBC, C4DM, Pompeu Fabra University, etc.)\footnote{\url{https://www.vamp-plugins.org/}}.

The Partiels application and its plug-ins have been developed at IRCAM within the Innovation and Research Means department since October 2020. The first version of Partiels was published on the IRCAM Forum in September 2021. In December 2023, the application and plug-ins became free, and since September 2024, Partiels has been available as an open-source project on GitHub under the GPLv3 license.  

This article begins by presenting the background to the project and its objectives, and then goes on to describe the main features offered by the application\footnote{The application includes a large number of features that cannot be described in detail in this article, which aims to provide a general overview of the tool. Readers requiring further information on a specific feature can refer to the Partiels user manual.}. Finally, it describes current developments and prospects for the project.

\section{Project origins}

Partiels and the first analysis plug-ins associated with it derive from the AudioSculpt \cite{Bogaards:04}\cite{Bogaards:042} application, a software package built around libraries and command-line tools, SuperVP, PM2, Ircam Descriptors, and Ircam Beat, developed at Ircam within the Sound Analysis and Synthesis team. AudioSculpt offered a graphical interface for displaying and interacting with analysis results and transforming sounds. The software is now deprecated, as its monolithic architecture and dependencies have prevented it from being updated and adapted to current hardware and operating systems\footnote{AudioSculpt is a mainly graphical application with a monolithic approach (all processing and analysis modules are embedded in the application). The application's code is based entirely on Apple's Carbon API, which is not cross-platform, has been deprecated since 2012, and is obsolete on 64-bit computers, meaning that it has not run on Apple computers for several years. As a result, the demand from users for an update to this software was growing. However, due to this monolithic approach and the obsolescence of almost all code, the development of a new version, even an elementary one, would require resources that are not available to IRCAM.}. The disappearance of this tool, popular within the IRCAM community, has created a major gap among signal processing researchers, musicologists, composers, computer music producers, teachers, sound designers, etc. It had become urgent to offer an alternative that would meet this demand, while avoiding the limitations that had led to the software's obsolete status. 

\begin{figure*}[ht]
\centerline{\includegraphics[width=\linewidth]{section-organize-the-tracks-by-groups.1-v2.0.9}}
\caption{\label{overview}{\it Overview of the Partiels application containing two groups of analyses. A first group (shrunk) with waveform and beat markers, and a second group (expanded) with spectrogram analysis and spectral centroid.}}
\end{figure*}

The challenge was to offer a short-term solution to this problem. However, although they share similar practices and uses, the audiences concerned by this expectation have significantly different needs and uses. In signal processing, for example, it is important to be able to compare the results of different analysis engines. Musicologists, on the other hand, need to generate images and diagrams to analyze sounds. In composition, a frequent request is to be able to export analysis results for use in other software packages such as Max\footnote{\url{https://cycling74.com/}}, Pure Data \cite{Puckette:97}, and OpenMusic \cite{Bresson:11} to produce sounds or scores. Moreover, since the last version of AudioSculpt, new issues and practices have emerged, in line with the evolution of digital audio and computing in general. 

The main functional specifications were as follows:
\begin{description}
\item[$\bullet$] to quickly and easily launch and modify analyses and their parameters,
\item[$\bullet$] to correct analysis results textually and graphically,
\item[$\bullet$] to improve visualization and comprehension of analyses, 
\item[$\bullet$] to facilitate communication and permeability with other software environments,
\item[$\bullet$] to easily apply a set of analyses to a series of audio files,
\item[$\bullet$] to compare several audio files and/or the different channels of the files, 
\item[$\bullet$] to share analyses and results with other users. 
\end{description}

It was necessary to develop a tool powerful enough to respond to these complex and heterogeneous practices. The aim was to offer a relatively simple and ergonomic interface, to be accessible to a varied and potentially non-initiated public, while using a flexible and dynamic architecture that would allow it to evolve and integrate new technologies and analyses.

Other tools such as Praat \cite{Boersma:06}, iAnalyse \cite{Couprie:08}, Spek\footnote{\url{https://www.spek.cc/}}, and Sonic Visualizer \cite{Cannam:10}\cite{Cannam:15} for sound analysis already exist, each offering solutions adapted to specific and varied needs. But none of these applications met all our specifications. One major obstacle was that, although Partiels is open-source, the algorithms developed by the Sound Analysis and Synthesis team are proprietary, preventing their direct integration into other software. Sonic Visualizer, which is closest to our needs, has an architecture based on plug-ins, enabling the integration of external technologies. But the analyses developed at IRCAM have complex, multidimensional data models with different types and require dynamic inputs, which did not allow them to be integrated into this software. The new software solution proposed is based on the Vamp plug-in format \cite{Cannam:06}\cite{Cannam:10}, incorporating an extension, Ircam Vamp Extension (IVE)\footnote{\url{https://github.com/Ircam-Partiels/ircam-vamp-extension}}, to meet specific needs. 

\section{General overview}

Partiels lets you control and apply analyses to digital audio files, organize, visualize, and edit results, and navigate through their representation. Data can be exported as images and in various text formats. Partiels offers additional features including a batch processing system, a command line interface, and the ability to send results via Open Sound Control (OSC) \cite{Wright:97}. 

\subsection{Organizing and managing analyses}

A Partiels document is associated with one or more audio files. This document has one or more groups, which themselves include analysis tracks. These tracks are stacked one on top of the other within a group. Groups are distributed one on top of the other within a document, as shown in Fig. \ref{overview}.

Partiels is a plug-ins host. Without plug-ins, it can only perform waveform and spectrogram analyses. All complex analyses are performed by dynamically loaded Vamp plug-ins. At startup, the application searches for the plug-ins installed on the machine\footnote{The plug-in search parameters and access paths can be configured in the application properties} and determines the various analyses available, which are then offered to the user. These analyses can generate three types of results, as established by the Vamp format, and which define their graphical representation mode: markers (time, duration, text) displayed as vertical lines to which text is potentially associated, points (time, duration, numerical value) displayed as segments, vectors (time, duration, list of numerical values) displayed as an image, most often for a spectrogram. 

When an analysis is created, it is necessarily added to a group. It is then possible to modify the organization within a group and to move or duplicate an analysis from one group to another. It is also possible to modify analysis track parameters via the properties window. These parameters are divided into two categories, as shown in Fig. \ref{properties}: 

\begin{figure}[ht]
\centerline{\includegraphics[width=\columnwidth]{section-track-properties.2-v2.0.9}}
\caption{\label{properties}{\it Property window of a spectral centroid analysis track with a section corresponding to analysis engine parameters and a section corresponding to graphical rendering parameters.}}
\end{figure}

\begin{description}
\item[$\bullet$] Analysis engine parameters (such as Fourier transform window size, transient detection threshold, neural network model, etc.) that generate a new analysis on the fly when changed.
\item[$\bullet$] Graphical rendering parameters (such as colors, font, text positioning, scale and range, grid, etc.) that generate on-the-fly updates of representations.
\end{description}
This approach enables graphical representations to be matched to the type of analysis and its context, in particular to highlight certain data in relation to others. On-the-fly updating of analyses and their representations is particularly useful for experimenting, understanding parameters, and adapting them as required.

\subsection{Audio file playback and management}

Partiels offers an engine with an audio file layout system, as shown in Fig. \ref{audiofiles}, and multichannel management linked to analysis. Each playback channel is associated with one channel of the audio file (or the sum of the channels). 

\begin{figure}[ht]
\centerline{\includegraphics{Screenshot4}}
\caption{\label{audiofiles}{\it Audio files layout window for analysis and playback with three files used for three separate channels.}}
\end{figure}

This approach makes it possible to compare different audio files and/or different channels of the same file in order, for example, to highlight differences in interpretations of the same piece, the artifacts of different encoding, or to enable analysis of audio spatialization effects (such as the spread of a stereophonic panning). During audio playback, each track in the layout system can be sent to a specific sound card channel via a routing matrix, in order to adapt to the analysis context and playback hardware. Finally, audio transport offers a looping system that can be magnetized to marker tracks to focus on specific time sections.

\subsection{Visualizing analysis results}

The main graphical interface represents the evolution of results over time, over which the playhead moves automatically during audio playback (or manually with the mouse).

\begin{figure}[ht]
\centerline{\includegraphics[width=\columnwidth]{Screenshot5}}
\caption{\label{table}{\it Table of values containing the results of the marker values of an analysis with the VAX plug-in resulting from the realignment of the Whisper analysis, which generated the sentence "Bienvenue aux ateliers du Forum de l'IRCAM.".}}
\end{figure}

\subsubsection{Instant representation}

The application also offers a representation of results on the left-hand side of the global interface, as well as a representation in the form of a table of numerical and textual values in a floating window, as shown in Fig. \ref{table}. These complementary representations are synchronized on the playhead, enabling joint reference to the different interfaces, so that the advantages of each type of visualization can be exploited simultaneously.

\subsubsection{Open Sound Control}

In addition, the results can be sent to third-party applications such as Pure Data, Max, or SuperCollider \cite{frauenberger:11} via OSC, as shown in Fig. \ref{pdpatch}, to generate further representations (and/or sonify the results). The Partiels distribution provides example patches for this purpose. 

\begin{figure}[h]
\centerline{\includegraphics[width=\columnwidth]{Screenshot6}}
\caption{\label{pdpatch}{\it Pure Data patch displaying the values of a spectrogram and a pitch detection based on data sent by Partiels in OSC.}}
\end{figure}

\subsection{Navigating and editing of analysis results}

Partiels offers two modes of interaction inspired by Max and Pure Data: navigation and editing. The navigation mode is dedicated to exploring results, offering a series of actions and shortcuts to control vertical and horizontal zooms for easy movement through analysis results. The tooltip window displays results corresponding to a given time (and position on the vertical axis when relevant). The editing mode enables you to modify analysis results directly on the graphical representation, by drawing curves, creating markers, copying and pasting results, etc. These operations can also be carried out textually via the results table. When analysis results are modified by the user, the data becomes independent of the plug-in analysis and is saved in binary files associated with the document. This makes it possible not only to share the document with modifications, but also to undo and redo actions. 

\subsection{Exporting and sharing analyses}

One of the major challenges of the project is to offer a tool that can be integrated into the existing uses and practices of users. To achieve this, Partiels offers interoperability with music creation software such as OpenMusic, Max, and Pure Data (or other digital audio workstations), as well as spreadsheet-type tools (or even simple text editors). 

Partiels can be used to export analysis results in a variety of formats to suit different needs and contexts. It supports images in PNG or JPEG formats, and in addition to the usual size-related features, Partiels offers the option of exporting tracks independently or preserving group overlay. It also supports various text formats, including JSON, which offers the possibility of embedding analysis information (in order to restart the analysis afterwards, if necessary). It also offers support for the CSV format and its derivatives, which can be easily loaded into various software environments such as Pure Data, Max, Reaper\footnote{\url{https://www.reaper.fm/}}, and others. This makes it possible, for example, to use this data to control synthesizers or, more simply, to segment audio files. More specific export formats, such as SDIF or CUE, are detailed in the user manual. It is also possible to export a single analysis, a group of analyses, or an entire document.

Partiels also offers a batch processing system with a graphical user interface that lets you apply a series of analyses to a set of audio files and automatically export the results in the desired format. In a development context, the command line interface can similarly be used to apply a set of analyses to an audio file and export the results. To facilitate the reproduction of analyses, it is possible to use an existing document as a template to apply its analyses to other audio files. This feature is available via the graphical interface or the command line interface.

Finally, Partiels offers the possibility of consolidating documents to facilitate the sharing of analyses and their results from one user to another, or from one computer to another. This action creates a folder associated with the Partiels document file, containing audio files and binary versions of the analysis results. So even if the computer or the user receiving the Partiels document does not have the plug-ins required for analysis, the results are still available.

\section{Analysis plug-ins and extensions}

Partiels is based on Vamp plug-ins. It is compatible with all the analysis plug-ins already available\footnote{\url{https://www.vamp-plugins.org/}} and distributed by numerous institutions such as Queen Mary University, the BBC, Pompeu Fabra University, and others. The project also includes several analysis plug-ins developed at IRCAM, which have required the creation of a Vamp extension.

\subsection{IRCAM plug-ins}

A great deal of work has gone into ensuring that the analyses are compatible with the main operating systems (macOS, Windows, and Linux), so that these tools are accessible to as many people as possible. This was particularly an important challenge for plug-ins based on machine learning technology\footnote{We managed to integrate these technologies in the plug-in as statically linked libraries to ensure the portability of the binaries and avoid the conflicts with third-party installations.}, notably the TensorFlow\footnote{\url{https://www.tensorflow.org}} \cite{Abadi:16}, a deep learning framework optimized for distributed training, and PyTorch \cite{Paszke:19}.

Some of these plug-ins are based on technologies developed at IRCAM by the Sound Analysis and Synthesis team of the STMS laboratory:
\begin{description}
\item[$\bullet$] SuperVP: This plug-in offers numerous analyses based on the eponymous phase vocoder \cite{Depalle:91}. These include pitch estimators (for melodic and percussive instruments, and one based on the FCN neural network \cite{Ardaillon:19}), spectrogram analysis using FFT, LPC, Cepstrum, True Envelop, transient detection, marker generation using spectral difference detection, formant analysis, and voice cutoff frequency analysis.
\item[$\bullet$] IrcamBeat: This plug-in allows you to estimate the tempo of an audio file and generate markers at each beat (which adapts to tempo fluctuations) \cite{Peeters:07}.
\item[$\bullet$] IrcamDescriptors: This plug-in offers four spectral descriptors (centroid, decrease, roll-off, and spread) and two perceptual descriptors (loudness and sharpness) \cite{Peeters:04}.
\item[$\bullet$] PM2: This plug-in offers harmonic and inharmonic partial detection with frequency, phase, and amplitude information, and a marker-based segmentation model for chord detection \cite{Depalle:93}\cite{Röbel:06}.
\item[$\bullet$] VAX: This plug-in is based on neural network models for text-to-audio alignment. It displays a probability matrix of Latin characters present in the text and segments and realigns a given text \cite{Doras:23}, as shown in Fig. \ref{table}.
\end{description}
The other plug-ins are based on technologies external to IRCAM, such as:
\begin{description}
\item[$\bullet$] Crepe: This plug-in offers height estimation using a set of neural network models developed by Jong Wook Kim, Justin Salamon, Peter Li, and Juan Pablo Bello \cite{Kim:18} and based on the TensorFlow framework \cite{Abadi:16}.
\item[$\bullet$] Whisper: This plug-in enables audio transcription using neural network models developed by OpenAI \cite{Radford:22}. The integration is based on the Whisper.cpp C/C++ library developed by Georgi Gerganov\footnote{\url{https://github.com/ggerganov/whisper.cpp}} for optimized real-time inference.
\item[$\bullet$] Basic Pitch: This plug-in enables multi-phonic pitch estimation using models developed by Spotify \cite{Bittner:22} and based on TensorFlow \cite{Abadi:16}.  
\end{description}

\subsection{The IRCAM VAMP extension}

When implementing the plug-ins, we came up against some limitations of the Vamp format. The Vamp plug-in SDK does not allow chaining of analyses, and only accepts scalar digital parameters and audio streams as input. This restriction prevents the development of complex approaches, such as aligning text to audio with the VAX engine, which requires text to be supplied upstream of the analysis. The IRCAM Vamp Extension (IVE) library, developed as part of this project, overcomes this constraint. It offers new interfaces to plug-ins, associated with those offered by the Vamp SDK, so that they can receive the results of other analyses in conjunction with the input parameters. In Partiels, this enables analysis chains to be created. For example, Whisper analysis is used to transcribe the text of an audio file, and the results are sent to VAX analysis to align the text with the audio. Updating an input plug-in parameter or modifying its results automatically triggers recalculation of the resulting analysis. Thus, changing the Whisper model or manually correcting the text restarts the analysis of the VAX plug-in and realigns the text to take account of the modifications.

A further restriction lies in the output formats of the analysis results as defined by the Vamp SDK. The result can be either a label, a scalar, or a vector of numerical values. In many cases, it is necessary to augment this output with additional numerical data to offer, for example, a confidence score for transients, or the amplitude and phase associated with the frequency of a partial. The IVE library offers the option of adding additional numerical data to the output data. In Partials, these values are displayed in addition to the main result data. They can also be used to modify representations by filtering results according to value thresholds. In this way, it is possible to hide certain partials whose amplitude is below a minimum threshold.

\section{Conclusions}
The Partiels project is now at an advanced stage of development. It offers a set of tools whose functionalities meet a large number of needs, contexts, and use cases, thanks to the analyses available via its numerous plug-ins. The software is already in use, both inside and outside IRCAM, in the fields of research and artistic creation, as well as in teaching, notably at Sorbonne University in musicology courses and the Sciences et Musicology curriculum.

However, there are still many aspects that can be completed and improved. One of the challenges is to continue to make this tool accessible to as many people as possible. In particular, we plan to offer video tutorials and translations of the interfaces (for the moment, the software is only available in English). On the other hand, we consider integrating certain IVE features directly into the Vamp SDK so that these improvements can be used in other Vamp hosts (Sonic Visualizer, Max, Pure Data, etc.) and so that IRCAM plug-ins are better supported outside Partiels. Generally speaking, IVE could be used as an experimental space before more stable integration within the Vamp SDK. As part of this experimental approach, we would like to integrate a graphical API into IVE to enable more specific representations for certain analyses, particularly when the results are multidimensional. In this regard, we are currently working on an adaptation of The Snail \cite{Hélie:17} analyses to generate a spectrogram from data where the color of a pixel depends on multiple data (and not just a single amplitude value). We also want to continue developing Max and Pure Data patches to offer alternative representations of the data and also to facilitate the sonification of this data. Finally, we are continually developing new analysis plug-ins based on new technologies that we are developing at IRCAM or on external models that use PyTorch in particular.

\section{Acknowledgments}
First of all, we would like to thank Queen Mary University and, more specifically, Chris Cannam, who developed Sonic Visualizer and the Vamp plug-in SDK. We also thank the Sound Analysis and Synthesis research team of the STMS laboratory at IRCAM, and more specifically Axel Röbel, Frédéric Cornu, Guillaume Doras, and Yann Teytaut for their contributions to the analysis engines. Finally, we would like to thank Matthew Harris, who contributed to the development of some of the Vamp plug-ins, SuperVP, IrcamBeat, IrcamDescriptors, and PM2.

\end{sloppypar}

%\newpage
\nocite{*}
\bibliographystyle{IEEEbib}
\bibliography{DAFx25_Partiels} % requires file DAFx25_tmpl.bib

\end{document}

```

```latex
@inproceedings{Abadi:16, 	
    Author = {M. Abadi and P. Barham and J. Chen and Z. Chen and A. Davis and J. Dean and M. Devin and S. Ghemawat and G. Irving and M. Isard and M. Kudlur and J. Levenberg and R. Monga and S. Moore and D. G. Murray and B. Steiner and P. Tucker and V. Vasudevan and P. Warden and M. Wicke and Y. Yu and X. Zheng},
	Title = {{TensorFlow}: A system for large-scale machine learning},
    Booktitle = {12th USENIX Symposium on Operating Systems Design and Implementation (OSDI '16)}, 
    Address = {Savannah, USA},
    Year = {2016}}

@inproceedings{Ardaillon:19, 	
    Author = {L. Ardaillon and A. Röbel},
    Title = {Fully-Convolutional Network for Pitch Estimation of Speech Signals},
    Booktitle = {Insterspeech 2019},
    Address = {Graz, Austria},
	Year = {2019}}

@inproceedings{Bittner:22, 	
    Author = {R. M. Bittner and J. J. Bosch and D. Rubinstein and G. Meseguer-Brocal and S. Ewert},
	Title = {A lightweight instrument-agnostic model for polyphonic note transcription and multipitch estimation},
    Booktitle = {Proceedings of the IEEE International Conference on Acoustics, Speech, and Signal Processing (ICASSP)}, 
    Address = {Singapore},
    Year = {2022}}

@inproceedings{Boersma:06, 	
    Author = {P. Boersma and V. van Heuven},
	Title = {Praat, a system for doing phonetics by computer},
    Booktitle = {Glot International}, 
    Volume = 5,
    Pages = {341–345}, 
    Year = {2001}}

@inproceedings{Bogaards:04, 	
    Author = {N. Bogaards and A. Röbel},
	Title = {An interface for analysis-driven sound processing},
    Booktitle = {in AES 119th Convention}, 
    Address = {New York, USA}, 
    Year = {2004}}

@inproceedings{Bogaards:042, 	
    Author = {N. Bogaards and A. Röbel and X. Rodet},
	Title = {Sound analysis and processing with {AudioSculpt} 2},
    Booktitle = {Proceedings of the International Computer Music Conference}, 
    Address = {Miami, USA}, 
    Year = {2004}}

@inproceedings{Bresson:11, 	
    Author = {J. Bresson and C. Agon and G. Assayag},
	Title = {{OpenMusic} – Visual programming environment for music composition, analysis and research},
    Booktitle = {Proceedings of the 19th ACM international conference on Multimedia}, 
    Address = {Scottsdale, USA}, 
    Year = {2011}}

@inproceedings{Cannam:06, 	
    Author = {C. Cannam and C. Landone and M. Sandler and J. P. Bello},
	Title = {{The Sonic Visualiser}: A visualisation Platform for Semantic Descriptors from Musical Signals},
    Booktitle = {Proceedings of 7th International Conference on Music Information Retrieval}, 
    Address = {Victoria, Canada}, 
    Year = {2006}}

@inproceedings{Cannam:10, 	
    Author = {C. Cannam and C. Landone and M. Sandler},
	Title = {{Sonic Visualiser}: An open source application for viewing, analysing, and annotating music audio files},
    Booktitle = {Proceedings of the ACM Multimedia 2010 International Conference}, 
    Address = {Firenze, Italy}, 
    Year = {2010}}

@inproceedings{Cannam:15, 	
    Author = {C. Cannam and E. Benetos and M. E. P. Davies and S. Dixon and C. Landone and M. Levy and M. Mauch and K. Noland and D. Stowell},
	Title = {{MIREX} 2015 entry: Vamp plugins from the centre for digital music},
    Booktitle = {Proceedings of the International Symposium on Music Information Retrieval},
    Address = {Urbana-Champaign, USA}, 
    Year = {2015}}

@inproceedings{Couprie:08, 
	Author = {P. Couprie},
	Title = {{iAnalyse} : un logiciel d’aide à l’analyse musicale. Journées d’Informatique Musicale},
    Booktitle = {Actes des Journées d’Informatique Musicale}, 
    Address = {Albi, France}, 
    Year = {2008}}

@inproceedings{Depalle:91, 	
    Author = {P. Depalle and G. Poirrot},
	Title = {{SVP}: A modular system for analysis, processing and synthesis of sound signals},
    Booktitle = {Proceedings of the International Computer Music Conference},
    Address = {Montreal, Canada},
    Year = {1991}} 

@inproceedings{Depalle:93, 	
    Author = {P. Depalle and G. Garcia and X. Rodet},
	Title = {Tracking of partials for additive sound synthesis using hidden {Markov} models},
    Booktitle = {Proceedings of the IEEE International Conference on Acoustics, Speech, and Signal Processing}, 
    Address = {Minneapolis, USA},
    Year = {1993}}

@article{Doras:23, 	
    Author = {G. Doras and Y. Teytaut and A. Röbel},
	Title = {A linear memory {CTC-based} algorithm for text-to-voice alignment of very long audio recordings},
    Journal = {Applied Sciences},
    Volume = {13},
    Year = {2023},
    Number = {3},
    Article-Number = {1854}}

@book{frauenberger:11,
	URL = {http://mitpress.mit.edu/catalog/item/default.asp?ttype=2&tid=12571},
	Title = {The {SuperCollider} {Book}},
	Publisher = {MIT Press},
	Author = {C. Frauenberger},
	Year = {2011},
}

@inproceedings{Hélie:17, 	
    Author = {T. Hélie and C. Picasso},
	Title = {{The Snail}: A real-time software application to visualize sounds},
    Booktitle = {Proceedings of the 20th International Conference on Digital Audio Effects (DAFx-17)}, 
    Address = {Edinburgh, UK},
    Year = {2017}}

@inproceedings{Kim:18, 	
    Author = {J. W. Kim and J. Salamon and P. Li and J. P. Bello},
	Title = {{CREPE}: A convolutional representation for pitch estimation},
    Booktitle = {Proceedings of the IEEE International Conference on Acoustics, Speech, and Signal Processing (ICASSP)}, 
    Address = {Alberta, Canada},
    Year = {2018}}

@inproceedings{Paszke:19, 	Author = {A. Paszke and S. Gross and F. Massa and A. Lerer and J. Bradbury and G. Chanan and T. Killeen and Z. Lin and N. Gimelshein and L. Antiga and A. Desmaison and A. Kopf and E. Yang and Z. DeVito and M. Raison and A. Tejani and S. Chilamkurthy and B. Steiner and L. Fang and J. Bai and S. Chintala},
	Title = {{PyTorch}: An imperative style, high-performance deep learning library},
    Booktitle = {Conférence: Advances in Neural Information Processing Systems (NeurIPS 2019)}, 
    Address = {Vancouver, Canada},
    Year = {2019}}

@techreport{Peeters:04, 	
    Author = {P. Peeters},
	Title = {A large set of audio features for sound description (similarity and classification) in the CUIDADO project},
    Institution = {IRCAM}, 
    Address = {Paris, France},
    Year = {2004}}

@inproceedings{Peeters:07, 	
    Author = {P. Peeters},
	Title = {Template-Based Estimation of Time-Varying Tempo},
    Booktitle = {EURASIP Journal on Advances in Signal Processing, Volume 2007, Issue 1},
    Year = {2007}}

@inproceedings{Puckette:97, 	
    Author = {M. Puckette},
	Title = {{Pure Data}: another integrated computer music environment},
    Booktitle = {Proceedings of the International Computer Music Conference}, 
    Address = {Thessaloniki, Greece},
    Year = {1997}}

@inproceedings{Radford:22, 	
    Author = {A. Radford and J. W. Kim and T. Xu and G. Brockman and C. McLeavey and I. Sutskever},
	Title = {Robust Speech Recognition via Large-Scale Weak Supervision},
    Booktitle = {https://arxiv.org/abs/2212.04356}, 
    Year = {2022}}

@inproceedings{Röbel:06, 	
    Author = {A. Röbel},
	Title = {Estimation of partial parameters for non stationary sinusoids},
    Booktitle = {Proceedings of the International Computer Music Conference}, 
    Address = {New Orleans, USA}, 
    Year = {2006}}

@inproceedings{Wright:97, 	
    Author = {M. Wright and A. Freed},
	Title = {{Open SoundControl}: A new protocol for communicating with sound synthesizers},
    Booktitle = {Proceedings of the International Computer Music Conference}, 
    Address = {Thessaloniki, Greece}, 
    Year = {1997}}
```