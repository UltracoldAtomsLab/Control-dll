#ifndef STRUCT_ALGORITHM
#define STRUCT_ALGORITHM

#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*
#pragma pack(push)
#pragma pack(1)
typedef struct{
    int dimSize;
    double data[1];
}t_LvArray, *p_LvArray, **h_LvArray;
#pragma pack(pop)
*/

typedef enum VariationShape{
    eShapeLinear = 0,
    eShapeAdiabatic = 3,
    eShapeExponential = 4,
    eShapeLinear_dup=5, // same as Linear
    eShapeSine=6
}VariationShape;

typedef enum AttrType{
    eAttrTime     = 0,
    eAttrInter    = 1,
    eAttrValue    = 2,
    eAttrTimeInc  = 3,
    eAttrInterInc = 4,
    eAttrValueInc = 5,
    eAttrShape    = 6,
    eAttrReserve  = 7
}AttrType;


// **********************************************************
// **                 ControlPointStruct                   **
// **********************************************************
typedef union CP_Data{
    double   d;
    double  *dp;
} CP_Data;

typedef struct ControlPointStruct{
    CP_Data   *Data;       // the data, *dp for "reserve" attribute, which may extend
    CP_Data  **pChBeg;     // Each pointer points the begining of a channel in Data.
    int       *nCpEachCh;  // number of control point for each channel
    int        nChannels;  // number of channel
    int        nCpMax;     // the maximum of control point among all channel
    int        nAttr;      // number of attributes for each control point

    double    *lReserve[65536];    // for efficient releasing memory which is allocated for reserve attribute.
    int        nInReserve;         // number of items in lReserve
} ControlPointStruct;

ControlPointStruct* CreateCtrlPtStruct  (int NumCh, int MaxNumPoint, int NumAttr, int *nCpEachCh);
void                DestroyCtrlPtStruct (ControlPointStruct* c);
CP_Data*            util_CPS_Lookup     (ControlPointStruct *c, int ch, int point, int attr);
int                 WriteCtrlPtDataD    (ControlPointStruct *c, int ch, int point, int attr, double data);
int                 WriteCtrlPtDataP    (ControlPointStruct *c, int ch, int point, int attr, double *data, int size);
CP_Data             ReadCtrlPtData      (ControlPointStruct *c, int ch, int point, int attr);
void                display             (FILE *f, ControlPointStruct *c);


// ******************************************************
// **                  WaveformStruct                  **
// ******************************************************
typedef struct WaveformStruct{
    double  *Data;       // the data
    double **pChBeg;     // Each pointer points the begining of a channel in Data.
    int      nChannels;  // number of channels
    int      nSamples;   // number of samples for each channel
} WaveformStruct;

WaveformStruct* CreateWaveformStruct  (int NumCh, int MaxNumSample);
void            DestroyWaveformStruct (WaveformStruct *w);
double*         util_WFS_Lookup       (WaveformStruct *w, int ch, int sample);
int             WriteWaveformData     (WaveformStruct *w, int ch, int sample, double data);
double          ReadWaveformData      (WaveformStruct *w, int ch, int sample);
void            waveformRecord        (FILE *f, WaveformStruct *w, int ch);

// ******************************************************
// **                  Main Algorithm                  **
// ******************************************************
ControlPointStruct*   Parse_FileTo2DArray(FILE *f);
int                   FreeProductOf_Parse_FileTo2DArray(ControlPointStruct *c);
int                   FindWaveformSampleNum(ControlPointStruct *c, double loop, double sample_rate);
int                   Build_OneChannel(ControlPointStruct *c, WaveformStruct *w, double loop, double sample_rate, int ch);
int                   Build_AllChannel(ControlPointStruct *c, WaveformStruct *w, double loop, double sample_rate);
int                   Map_ReadFromFile(FILE *f, double **key, double **value);
int                   FreeProductOf_Map_ReadFromFile(double *key, double *value);
int                   Map_LookupPowerToVoltage(double *mapKey, double *mapValue, int size, int ch, WaveformStruct *w);
int                   Map_Mapping(char *s_channels_to_map, char *s_map_files, WaveformStruct *w);

#ifdef __cplusplus
}
#endif

#endif
