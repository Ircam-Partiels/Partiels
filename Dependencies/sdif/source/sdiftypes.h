/* This document ../sdif/sdiftypes.h generated Tue Sep 24 15:36:54 2002 by xmltostyp.pl from sdiftypes.xml
*/
#ifndef _SDIFTYPES_H_
#define _SDIFTYPES_H_

#define SDIFTYPES_STRING \
"\n"\
"1MTD 1NVT { NVTText }\n"\
"1MTD 1TYP { TYPText }\n"\
"1MTD 1IDS { IDSText }\n"\
"1FTD 1NVT { 1NVT NameValueTable;  }\n"\
"1FTD 1TYP { 1TYP TypeDefinitions;  }\n"\
"1FTD 1IDS { 1IDS StreamInfo;  }\n"\
"\n"\
"1MTD 1GAI { Gain }\n"\
"1FTD 1GAI { 1GAI Gain;  }\n"\
"1MTD IWIN { WindowIdentifier, WindowSize }\n"\
"1MTD 1WIN { Samples }\n"\
"1FTD 1WIN { IWIN WindowInfo; 1WIN Window;  }\n"\
"1MTD 1CHA { Channel1, Channel2 }\n"\
"\n"\
"1MTD 1FQ0 { Frequency, Confidence, Score, RealAmplitude }\n"\
"1FTD 1FQ0 { 1FQ0 FundamentalFrequencyEstimate;  }\n"\
"\n"\
"1MTD 1PIC { Frequency, Amplitude, Phase, Confidence }\n"\
"1MTD 1TRC { Index, Frequency, Amplitude, Phase }\n"\
"1MTD 1HRM { Index, Frequency, Amplitude, Phase }\n"\
"1FTD 1PIC { 1PIC PickedPeaks;  }\n"\
"1FTD 1TRC { 1TRC SinusoidalTracks;  }\n"\
"1FTD 1HRM { 1HRM HarmonicPartials;  }\n"\
"1MTD 1HRE { MeanDeltaFrequency, Harmonicity, WeightedHarmonicity }\n"\
"1FTD 1HRE { 1HRE HarmonicityEstimate;  }\n"\
"\n"\
"1MTD IENV { HighestBinFrequency, ScaleType, BreakFrequency }\n"\
"1MTD 1ENV { Env }\n"\
"1FTD 1ENV { IENV SpectralEnvelopeInfo; 1ENV SpectralEnvelope; 1GAI Gain;  }\n"\
"1MTD ITFC { SamplingRate, Order }\n"\
"1MTD 1CEC { CepstralCoefficients }\n"\
"1FTD 1CEC { 1CEC CepstralCoefs;  }\n"\
"1MTD 1ARA { AutoRegressiveCoefficients }\n"\
"1MTD 1ARK { ReflectionCoefficients }\n"\
"1MTD 1ARR { AutoCorrelationCoefficients }\n"\
"1FTD 1ARA { 1GAI Gain; 1ARA ARACoefs;  }\n"\
"1FTD 1ARK { 1GAI Gain; 1ARK ARKCoefs;  }\n"\
"1FTD 1ARR { 1ARR ARRCoefs;  }\n"\
"\n"\
"1MTD 1FOF { Frequency, Amplitude, BandWidth, Tex, DebAtt, Atten, Phase }\n"\
"1MTD 2RES { Frequency, Amplitude, DecayRate, Phase }\n"\
"1MTD 1RES { Frequency, Amplitude, BandWidth, Saliance, Correction }\n"\
"1MTD 1DIS { Distribution, Amplitude }\n"\
"1FTD 1NOI { 1DIS NoiseDistribution;  }\n"\
"1FTD 1FOB { 1FQ0 FundamentalFrequencyEstimate; 1FOF Formants; 1CHA Channels;  }\n"\
"1FTD 1REB { 1RES Filters; 1CHA Channels;  }\n"\
"\n"\
"1MTD ISTF { DFTPeriod, WindowDuration, FFTSize }\n"\
"1MTD 1STF { Real, Imaginary }\n"\
"1FTD ISTF { ISTF FourierTransformInfo;  }\n"\
"1FTD 1STF { ISTF FourierTransformInfo; 1STF FourierTransform; 1WIN Window;  }\n"\
"\n"\
"1MTD INRG { Scale, NormalisationFactor }\n"\
"1MTD 1NRG { Energy }\n"\
"1FTD 1NRG { INRG ScaleAndFactor; 1NRG Energy; IWIN WindowInfo; 1WIN Window;  }\n"\
"1MTD 1BND { LowerFrequencyLimit, UpperFrequencyLimit }\n"\
"1FTD 1BND { 1BND Bands;  }\n"\
"\n"\
"1MTD ITDS { SamplingRate }\n"\
"1MTD 1TDS { Sample }\n"\
"\n"\
"1MTD 1PEM { Identifier, Parameter1, Parameter2, Parameter3 }\n"\
"1MTD ITMR { Index, Frequency, Amplitude, Phase }\n"\
"1MTD ITMI { Index }\n"\
"1MTD 1BEG { Id }\n"\
"1MTD 1END { Id }\n"\
"1MTD 1SEG { Confidence }\n"\
"1FTD 1SEG { 1SEG Segmentation;  }\n"\
"1MTD 1LAB { Chars }\n"\
"1FTD 1MRK { 1BEG SegmentStart; 1END SegmentEnd; 1SEG Segmentation; 1LAB Label; 1PEM PeriodMarker; ITMR TransientMarkerRepresentation; ITMI TransientMarkerIdentifier;  }\n"\
"\n"\
"1MTD 1VUN { VoicingCoefficient }\n"\
"1MTD 1VUF { CuttingFrequency }\n"\
"1FTD 1VUV { 1VUN VoicedUnvoicedNorm; 1VUF VoicedUnvoicedFreq;  }\n"\
"\n"\
"1MTD 1MID { Status, Data1, Data2 }\n"\
"1MTD 1SYX { Data }\n"\
"1FTD 1MID { 1MID MIDIEvent; 1SYX MIDISystemExclusive;  }\n"\
"\n"\
"1MTD EMPM { Value, Index }\n"\
"1MTD EMJR { Record }\n"\
"1FTD EFPM { EMPM Tableau; EMJR EndRecording;  }\n"\


#endif
