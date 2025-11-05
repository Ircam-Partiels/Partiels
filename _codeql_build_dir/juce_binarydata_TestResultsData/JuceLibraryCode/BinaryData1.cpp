/* ==================================== JUCER_BINARY_RESOURCE ====================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#include <cstring>

namespace TestResultsData
{

//================== Columns-backup.dat ==================
static const unsigned char temp_binary_data_0[] =
{ 80,84,76,67,76,83,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,96,144,142,194,122,222,147,194,237,176,166,194,228,131,203,194,182,88,178,172,249,198,55,63,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,40,65,111,194,192,230,117,194,177,9,138,194,
234,155,164,194,182,88,178,172,249,198,71,63,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,101,241,99,194,78,10,111,194,235,146,136,194,204,159,175,194,3,0,0,0,0,0,0,0,6,4,165,46,58,213,81,63,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,44,25,103,194,140,59,118,194,237,6,151,194,
91,99,158,194,51,154,209,153,248,198,87,63,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,162,104,83,194,2,177,99,194,233,186,127,194,44,226,159,194,97,48,254,4,183,184,93,63,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,99,1,81,194,187,197,103,194,90,250,128,194,198,227,156,194,
0,0 };

const char* Columnsbackup_dat = (const char*) temp_binary_data_0;

}

#include "TestResultsData.h"

namespace TestResultsData
{

const char* getNamedResource (const char* resourceNameUTF8, int& numBytes);
const char* getNamedResource (const char* resourceNameUTF8, int& numBytes)
{
    unsigned int hash = 0;

    if (resourceNameUTF8 != nullptr)
        while (*resourceNameUTF8 != 0)
            hash = 31 * hash + (unsigned int) *resourceNameUTF8++;

    switch (hash)
    {
        case 0x94d5df17:  numBytes = 262; return Columnsbackup_dat;
        case 0x32b8bc15:  numBytes = 310; return Columns_dat;
        case 0x2461c42a:  numBytes = 653; return Columns_json;
        case 0x8b1b5f2f:  numBytes = 27; return Error_csv;
        case 0x8b1b5f5c:  numBytes = 288; return Error_cue;
        case 0x8b1b60c0:  numBytes = 186; return Error_dat;
        case 0xd853b4df:  numBytes = 83; return Error_json;
        case 0x4f314195:  numBytes = 202; return ErrorMultipleType_json;
        case 0xffd90b51:  numBytes = 170; return ErrorNoTime_json;
        case 0x8d7bd152:  numBytes = 125; return MarkerNoType_json;
        case 0xd203418c:  numBytes = 142; return MarkersReaperM_csv;
        case 0xd249b711:  numBytes = 264; return MarkersReaperR_csv;
        case 0x58a767b3:  numBytes = 151; return Markersbackup_dat;
        case 0x7213e820:  numBytes = 137; return Markers_csv;
        case 0x7213e84d:  numBytes = 305; return Markers_cue;
        case 0x7213e9b1:  numBytes = 191; return Markers_dat;
        case 0xd06c4a0e:  numBytes = 189; return Markers_json;
        case 0x721407a7:  numBytes = 184; return Markers_lab;
        case 0x7d11f7f4:  numBytes = 137; return MarkersSpace_csv;
        case 0x104ae4c3:  numBytes = 137; return MarkersTab_csv;
        case 0xb05179d6:  numBytes = 1828; return PluginList_json;
        case 0xdaf1dffc:  numBytes = 51952; return PluginListDescription_json;
        case 0xbb9098dd:  numBytes = 169; return Pointsbackup_dat;
        case 0xc35bc0ca:  numBytes = 244; return Points_csv;
        case 0xc35bc25b:  numBytes = 225; return Points_dat;
        case 0xa81f86a4:  numBytes = 326; return Points_json;
        case 0x08305a24:  numBytes = 301; return PointsOptional_json;
        case 0x9908f98a:  numBytes = 244; return PointsSpace_csv;
        case 0xc82153d9:  numBytes = 244; return PointsTab_csv;
        case 0x051f8eee:  numBytes = 225; return PointsWithExtra_csv;
        case 0x9ed57d00:  numBytes = 568; return PointsWithExtra_json;
        case 0x662d42a5:  numBytes = 157; return PointsWithExtraFiltered_csv;
        case 0x5f7e4029:  numBytes = 348; return PointsWithExtraFiltered_json;
        case 0xeb6fdfcd:  numBytes = 33471; return SpectralCentroidFailure_csv;
        case 0x5182c68e:  numBytes = 33452; return SpectralCentroidSV_csv;
        default: break;
    }

    numBytes = 0;
    return nullptr;
}

const char* namedResourceList[] =
{
    "Columnsbackup_dat",
    "Columns_dat",
    "Columns_json",
    "Error_csv",
    "Error_cue",
    "Error_dat",
    "Error_json",
    "ErrorMultipleType_json",
    "ErrorNoTime_json",
    "MarkerNoType_json",
    "MarkersReaperM_csv",
    "MarkersReaperR_csv",
    "Markersbackup_dat",
    "Markers_csv",
    "Markers_cue",
    "Markers_dat",
    "Markers_json",
    "Markers_lab",
    "MarkersSpace_csv",
    "MarkersTab_csv",
    "PluginList_json",
    "PluginListDescription_json",
    "Pointsbackup_dat",
    "Points_csv",
    "Points_dat",
    "Points_json",
    "PointsOptional_json",
    "PointsSpace_csv",
    "PointsTab_csv",
    "PointsWithExtra_csv",
    "PointsWithExtra_json",
    "PointsWithExtraFiltered_csv",
    "PointsWithExtraFiltered_json",
    "SpectralCentroidFailure_csv",
    "SpectralCentroidSV_csv"
};

const char* originalFilenames[] =
{
    "Columns-backup.dat",
    "Columns.dat",
    "Columns.json",
    "Error.csv",
    "Error.cue",
    "Error.dat",
    "Error.json",
    "ErrorMultipleType.json",
    "ErrorNoTime.json",
    "MarkerNoType.json",
    "Markers-Reaper-M.csv",
    "Markers-Reaper-R.csv",
    "Markers-backup.dat",
    "Markers.csv",
    "Markers.cue",
    "Markers.dat",
    "Markers.json",
    "Markers.lab",
    "MarkersSpace.csv",
    "MarkersTab.csv",
    "PluginList.json",
    "PluginListDescription.json",
    "Points-backup.dat",
    "Points.csv",
    "Points.dat",
    "Points.json",
    "PointsOptional.json",
    "PointsSpace.csv",
    "PointsTab.csv",
    "PointsWithExtra.csv",
    "PointsWithExtra.json",
    "PointsWithExtraFiltered.csv",
    "PointsWithExtraFiltered.json",
    "SpectralCentroid-Failure.csv",
    "SpectralCentroid-SV.csv"
};

const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8);
const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8)
{
    for (unsigned int i = 0; i < (sizeof (namedResourceList) / sizeof (namedResourceList[0])); ++i)
        if (strcmp (namedResourceList[i], resourceNameUTF8) == 0)
            return originalFilenames[i];

    return nullptr;
}

}
