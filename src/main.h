#ifndef __MAIN_H__
#define __MAIN_H__

#include <windows.h>
#include "Struct_Algorithm.h"
/*  To use this exported function of dll, include this header
 *  in your project.
 */

#ifdef BUILD_DLL
    #define DLL_EXPORT __declspec(dllexport)
#else
    #define DLL_EXPORT __declspec(dllimport)
#endif


#ifdef __cplusplus
extern "C"
{
#endif


// The analogue timing part:
//    function double Timing returns the waveform in array ret
//    function int inquery_array_size returns the needed size of the array needed by Timing.
double DLL_EXPORT
Analog_Timing(
    double loop,
    double sample_rate,
    char *conf_path,
    char *to_be_mapped_channels,
    char *mapping_files,
    // data storage information
    int     mem_size,      // size of ret array built in Labview
    double  *ret,
    //p_LvArray  ret,          // an array to store c result
    int     *samples_each, // output from c, the sample number for each channel
    int     *channels_num  // output from c, the number of channels
);

int DLL_EXPORT
Analog_inquery_array_size(
    double loop,
    double sample_rate,
    char *conf_path
);


// for external use of ControlPoint stucture
ControlPointStruct* DLL_EXPORT
CtrlPt_File_To_CtrlPtStruct(char *conf_path);

void DLL_EXPORT
CtrlPt_Struct_Close(ControlPointStruct* c);

CP_Data DLL_EXPORT
CtrlPt_DataRead(ControlPointStruct *c, int ch, int point, int attr);

double DLL_EXPORT
CtrlPt_InqueryProperty(
    char    *sCtrlPtFile,
    char    *sNameProperty
);


// Digital Control and Sequence generation part:
typedef int int32_t;
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;

int DLL_EXPORT
Digital_C_ReadCtrlPoints_To_ArrayOfFPGACommands(
    int32_t iLoop,
	char    *sCtrlPtFile,
	int32_t *nContainer,
	uint8_t *pContainer
);

void DLL_EXPORT
Digital_ResetDev( int32_t *nByteArrayIdx, uint8_t *pByteArray);

void DLL_EXPORT
Digital_Arm(int32_t *nByteArrayIdx, uint8_t *pByteArray);

void DLL_EXPORT
Digital_SetChValueToInit(int32_t *nByteArrayIdx, uint8_t *pByteArray);

void DLL_EXPORT
Digital_SetChValue(int32_t *nByteArrayIdx, uint8_t *pByteArray, int32_t *lChList, int32_t *lValList, int32_t nListLength);

void DLL_EXPORT
Digital_SetInitValue(int32_t *nByteArrayIdx, uint8_t *pByteArray, int32_t *lChList, int32_t *lValList, int32_t nListLength);

void DLL_EXPORT
Digital_WritePeriod( int32_t *nByteArrayIdx, uint8_t *pByteArray, uint32_t nPeriod);

void DLL_EXPORT
Configure_SetSamplingRate(int32_t *nByteArrayIdx, uint8_t *pByteArray, uint32_t nfreq);

/*
void DLL_EXPORT
Digital_PrepareDataToSend(int32_t nIntArray, int32_t* pIntArray, int32_t *nByteArrayIdx, uint8_t *pByteArray );
*/

/*
void DLL_EXPORT
Digital_LV_ReadCtrlPoints_To_ArrayOfFPGACommands(
	int32_t iLoop,
	int32_t *nCtrlPtsOfUpCh,
	double  *pCtrlPtsOfUpCh,
	int32_t *nCtrlPtsOfDownCh,
	double  *pCtrlPtsOfDownCh,
	int32_t *nByteArrayIdx,
	uint8_t *pByteArray
);
*/

#ifdef __cplusplus
}
#endif

#endif // __MAIN_H__
