/* =========================================================================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#pragma once

namespace TestResultsData
{
    extern const char*   Columnsbackup_dat;
    const int            Columnsbackup_datSize = 262;

    extern const char*   Columns_dat;
    const int            Columns_datSize = 310;

    extern const char*   Columns_json;
    const int            Columns_jsonSize = 653;

    extern const char*   Error_csv;
    const int            Error_csvSize = 27;

    extern const char*   Error_cue;
    const int            Error_cueSize = 288;

    extern const char*   Error_dat;
    const int            Error_datSize = 186;

    extern const char*   Error_json;
    const int            Error_jsonSize = 83;

    extern const char*   ErrorMultipleType_json;
    const int            ErrorMultipleType_jsonSize = 202;

    extern const char*   ErrorNoTime_json;
    const int            ErrorNoTime_jsonSize = 170;

    extern const char*   MarkerNoType_json;
    const int            MarkerNoType_jsonSize = 125;

    extern const char*   MarkersReaperM_csv;
    const int            MarkersReaperM_csvSize = 142;

    extern const char*   MarkersReaperR_csv;
    const int            MarkersReaperR_csvSize = 264;

    extern const char*   Markersbackup_dat;
    const int            Markersbackup_datSize = 151;

    extern const char*   Markers_csv;
    const int            Markers_csvSize = 137;

    extern const char*   Markers_cue;
    const int            Markers_cueSize = 305;

    extern const char*   Markers_dat;
    const int            Markers_datSize = 191;

    extern const char*   Markers_json;
    const int            Markers_jsonSize = 189;

    extern const char*   Markers_lab;
    const int            Markers_labSize = 184;

    extern const char*   MarkersSpace_csv;
    const int            MarkersSpace_csvSize = 137;

    extern const char*   MarkersTab_csv;
    const int            MarkersTab_csvSize = 137;

    extern const char*   PluginList_json;
    const int            PluginList_jsonSize = 1828;

    extern const char*   PluginListDescription_json;
    const int            PluginListDescription_jsonSize = 51952;

    extern const char*   Pointsbackup_dat;
    const int            Pointsbackup_datSize = 169;

    extern const char*   Points_csv;
    const int            Points_csvSize = 244;

    extern const char*   Points_dat;
    const int            Points_datSize = 225;

    extern const char*   Points_json;
    const int            Points_jsonSize = 326;

    extern const char*   PointsOptional_json;
    const int            PointsOptional_jsonSize = 301;

    extern const char*   PointsSpace_csv;
    const int            PointsSpace_csvSize = 244;

    extern const char*   PointsTab_csv;
    const int            PointsTab_csvSize = 244;

    extern const char*   PointsWithExtra_csv;
    const int            PointsWithExtra_csvSize = 225;

    extern const char*   PointsWithExtra_json;
    const int            PointsWithExtra_jsonSize = 568;

    extern const char*   PointsWithExtraFiltered_csv;
    const int            PointsWithExtraFiltered_csvSize = 157;

    extern const char*   PointsWithExtraFiltered_json;
    const int            PointsWithExtraFiltered_jsonSize = 348;

    extern const char*   SpectralCentroidFailure_csv;
    const int            SpectralCentroidFailure_csvSize = 33471;

    extern const char*   SpectralCentroidSV_csv;
    const int            SpectralCentroidSV_csvSize = 33452;

    // Number of elements in the namedResourceList and originalFileNames arrays.
    const int namedResourceListSize = 35;

    // Points to the start of a list of resource names.
    extern const char* namedResourceList[];

    // Points to the start of a list of resource filenames.
    extern const char* originalFilenames[];

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding data and its size (or a null pointer if the name isn't found).
    const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes);

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding original, non-mangled filename (or a null pointer if the name isn't found).
    const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8);
}
