#include "main.h"
#include "Struct_Algorithm.h"
#include <stdio.h>
#include <stdlib.h>
#include <vector>


#define USE_50M_CLK


// FPGA commands
#define RETURN_IDLE 0x00
#define SET_PERIOD  0x01
#define SET_CH_WAVE 0x02
#define SET_CH_INIT 0x03
#define SET_CH_VAL  0x04
#define SET_SPL_RATE    0x05
#define CMD_ARM     0x80
#define CMD_TO_INIT 0x81
#define CMD_RESET_COUNT 0x82
#define CMD_RESET_DEV   0xFF

#define NC          0x00

#ifdef __cplusplus
extern "C"
{
#endif


///===============================================================================================



void parse(uint32_t target, int byteNum, int32_t &iContainer, uint8_t *pContainer)
{
    unsigned char ch[8];
    for(int i = 0; i < byteNum; ++i){
        ch[i] = target & 0xff;
        target = target >> 8;
    }
    for(int i = byteNum-1; i>=0; --i)
        pContainer[iContainer++] = ch[i];
}
void parse_2(uint32_t target, int32_t &iContainer, uint8_t *pContainer){parse( target, 2, iContainer, pContainer);}
void parse_4(uint32_t target, int32_t &iContainer, uint8_t *pContainer){parse( target, 4, iContainer, pContainer);}
void parse_8(uint32_t target, int32_t &iContainer, uint8_t *pContainer){parse( target, 8, iContainer, pContainer);}

int DLL_EXPORT Digital_C_ReadCtrlPoints_To_ArrayOfFPGACommands(
    int32_t iLoop,           //iteration times
	char    *sCtrlPtFile,    //Digital Control Point file [csv]
	int32_t *nContainer,     //
	uint8_t *pContainer      //
){
    // load DLLs
    //HINSTANCE hGetProcIDDLL = LoadLibrary(timing_dll);
    //if(hGetProcIDDLL == NULL)
    //{
    //    perror("load library error\n");
    //    FreeLibrary(hGetProcIDDLL);
    //    return 1;
    //}

    // declare the function pointer
    //ControlPointStruct*   (__cdecl *CtrlPt_File_To_CtrlPtStruct)   (char*);
    //void                  (__cdecl *CtrlPt_Struct_Close)           (ControlPointStruct*);
    //CP_Data               (__cdecl *CtrlPt_DataRead)               (ControlPointStruct*,int,int, int);

    // assign the function pointer
    //CtrlPt_File_To_CtrlPtStruct = (ControlPointStruct* (__cdecl*)(char*))                GetProcAddress(hGetProcIDDLL,"CtrlPt_File_To_CtrlPtStruct");
    //CtrlPt_Struct_Close         = (void (__cdecl*)(ControlPointStruct*))                 GetProcAddress(hGetProcIDDLL,"CtrlPtStruct_Close");
    //CtrlPt_DataRead             = (CP_Data(__cdecl *)(ControlPointStruct*,int,int, int)) GetProcAddress(hGetProcIDDLL,"CtrlPt_DataRead");

    //if(CtrlPt_File_To_CtrlPtStruct==NULL || CtrlPt_Struct_Close==NULL||CtrlPt_DataRead==NULL)
    //{
    //    perror("not the correct dll!\n");
    //    FreeLibrary(hGetProcIDDLL);
    //    return 2;
    //}

    // main part start!
    ControlPointStruct  *CtrlPts;
    CP_Data             cpt_v, cpt_v_prev, cpt_t, cpt_inc;
    int                 status = 0;

    CtrlPts = CtrlPt_File_To_CtrlPtStruct(sCtrlPtFile);
    if(CtrlPts==NULL)
        return -1;

	// we shift the time by 0.02ms(1 clock-cycle) to reserve the first cycle as the initializing
	// the channel initialized to be 1 would should have its last change-at time equal to 1.

	int nChannels = CtrlPts->nChannels; // both Up and Down have 8 channels
	//const int nCPTKinds = 8; // 8 ctrl point kinds: change-at, change-in, voltage value, and the threes increment, and additional two reserved values

	int ByteArrayMaxIdx = *nContainer;
	int &iContainer = *nContainer;
	uint8_t InitialVoltage[128];
	iContainer = 0;

	// repeat time set to 0
	pContainer[iContainer++] = SET_PERIOD;// indicating to send repeat period
	parse_4(0, iContainer, pContainer);// period time data
	pContainer[iContainer++] = RETURN_IDLE;// ending 0

    // parsing the waveform
	pContainer[iContainer++] = SET_CH_WAVE;// indicating to send waveform
	std::vector<uint32_t> waveform;
	for(int iCh = 0; iCh < nChannels; ++iCh)
	{
		waveform.clear();

		// i = 0 case is the initialization
		cpt_v = CtrlPt_DataRead (CtrlPts, iCh, 0, eAttrValue);
		InitialVoltage[iCh] = (cpt_v.d != 0);

		for(int i = 1; i < CtrlPts->nCpEachCh[iCh]; ++i)
		{
		    cpt_v_prev = CtrlPt_DataRead (CtrlPts, iCh, i-1, eAttrValue);
		    cpt_v      = CtrlPt_DataRead (CtrlPts, iCh, i,   eAttrValue);
			if((cpt_v_prev.d==0) xor (cpt_v.d==0))
			{
				cpt_t   = CtrlPt_DataRead (CtrlPts, iCh, i, eAttrTime);
				cpt_inc = CtrlPt_DataRead (CtrlPts, iCh, i, eAttrTimeInc);
#ifdef USE_50M_CLK
				uint32_t temp = (uint32_t)((cpt_t.d+cpt_inc.d*(iLoop))*50000); // 1s = 50M clock-cycles => 1ms = 50k cycles
                status = (temp >= 0)? 0 : -1;
#else
                uint32_t temp = (uint32_t)((cpt_t.d+cpt_inc.d*iLoop)*20); // sampling rate clock
                temp += (int32_t)(nOffsetTime_us/50.0); // /1000*20: /1000 for trans from ms to us, then *20 for sampling rate 20k
                status = (temp >= 0)? 0 : -1;
#endif
				waveform.push_back(temp);
			}
		}

		// store to the container
		pContainer[iContainer++] = (uint32_t)iCh;// channel id
		/*   Remain unknown  */
		parse_2(waveform.size(), iContainer, pContainer);// number of edges of the channel
		while(!waveform.empty())// clock-cycle
		{
			parse_4(waveform.back(), iContainer, pContainer);
			waveform.pop_back();
		}
	}

	pContainer[iContainer++] = 255;// sending channel id 255, indicating to terminate
	pContainer[iContainer++] = RETURN_IDLE;

	pContainer[iContainer++] = SET_CH_INIT;
	for(unsigned int i = 0; i < nChannels; ++i){
        pContainer[iContainer++] = i;
        pContainer[iContainer++] = InitialVoltage[i];
	}
	pContainer[iContainer++] = 255;// sending channel id 255, indicating to terminate
	pContainer[iContainer++] = RETURN_IDLE;

	pContainer[iContainer++] = CMD_TO_INIT;
	pContainer[iContainer++] = NC;
	pContainer[iContainer++] = RETURN_IDLE;

	CtrlPt_Struct_Close(CtrlPts);
	//FreeLibrary(hGetProcIDDLL);
	return status;
}


// ===============================================
void DLL_EXPORT Digital_ResetDev(
    int32_t *nContainer,
	uint8_t *pContainer
)
{
    int     &iContainer = *nContainer;
    iContainer = 0;

    pContainer[iContainer++] = CMD_RESET_DEV;
    pContainer[iContainer++] = NC;
	pContainer[iContainer++] = RETURN_IDLE;
}
// ===============================================
void DLL_EXPORT Digital_Arm(
    int32_t *nContainer,
	uint8_t *pContainer
)
{
    int     &iContainer = *nContainer;
    iContainer = 0;

    pContainer[iContainer++] = CMD_ARM;
    pContainer[iContainer++] = NC;
	pContainer[iContainer++] = RETURN_IDLE;
}
// ===============================================
void DLL_EXPORT Digital_SetChValueToInit(
    int32_t *nContainer,
	uint8_t *pContainer
)
{
    int     &iContainer = *nContainer;
    iContainer = 0;

    pContainer[iContainer++] = CMD_TO_INIT;
    pContainer[iContainer++] = NC;
	pContainer[iContainer++] = RETURN_IDLE;
}
// ===============================================
void DLL_EXPORT Digital_SetChValue(
    int32_t *nContainer,
	uint8_t *pContainer,
	int32_t *lChList,
	int32_t *lValList,
	int32_t nListLength
)
{
    int     &iContainer = *nContainer;
    iContainer = 0;

    pContainer[iContainer++] = SET_CH_VAL;
    for(int i = 0; i < nListLength; ++i){
        if(lChList[i]==255){
            perror("error channel number 255\n");
            break;
        }
        pContainer[iContainer++] = lChList[i];
        pContainer[iContainer++] = lValList[i];
    }
    pContainer[iContainer++] = 255; // enter channel number 255 to indicate an end.
	pContainer[iContainer++] = RETURN_IDLE;
}
// ===============================================
void DLL_EXPORT Digital_SetInitValue(
    int32_t *nContainer,
	uint8_t *pContainer,
	int32_t *lChList,
	int32_t *lValList,
	int32_t nListLength
)
{
    int     &iContainer = *nContainer;
    iContainer = 0;

    pContainer[iContainer++] = SET_CH_INIT;
    for(int i = 0; i < nListLength; ++i){
        if(lChList[i]==255){
            perror("error channel number 255\n");
            break;
        }
        pContainer[iContainer++] = lChList[i];
        pContainer[iContainer++] = lValList[i];
    }
    pContainer[iContainer++] = 255; // enter channel number 255 to indicate an end.
	pContainer[iContainer++] = RETURN_IDLE;
}
// ===============================================
void DLL_EXPORT Digital_WritePeriod(
    int32_t *nContainer,
	uint8_t *pContainer,
	uint32_t nPeriod
)
{
    int     &iContainer = *nContainer;
    iContainer = 0;

    // repeat time set to 0
	pContainer[iContainer++] = SET_PERIOD;// indicating to send repeat period
	parse_4(nPeriod, iContainer, pContainer);// period time data
	pContainer[iContainer++] = RETURN_IDLE;// ending 0
}

void DLL_EXPORT Configure_SetSamplingRate(
    int32_t *nContainer,
    uint8_t *pContainer,
    uint32_t nfreq //[kHz]
)
{
    /*=====================================
    Range:
    clk_cnt_max : 10~2550
    freq        : 9.8kHz~2.5MHz
    =====================================*/
    int     &iContainer = *nContainer;
    iContainer = 0;

    pContainer[iContainer++] = SET_SPL_RATE;
    uint8_t clk_cnt_max = (uint8_t) (25000/nfreq); //25000 kHz = 50/2 Mhz = 50000/2 kHz
    pContainer[iContainer++] = clk_cnt_max/10;
}


#ifdef __cplusplus
}
#endif
