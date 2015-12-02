#include <stdio.h>
#include <stdlib.h>
#include "Struct_Algorithm.h"

// constructor-like function
WaveformStruct*
CreateWaveformStruct(int NumCh, int MaxNumSample){
    WaveformStruct *ret;
    int i;
    ret            =(WaveformStruct*) malloc(sizeof(WaveformStruct));
    ret->nChannels = NumCh;
    ret->nSamples  = MaxNumSample;
    ret->Data      =(double*) malloc(sizeof(double)*NumCh*MaxNumSample);
    ret->pChBeg    =(double**) malloc(sizeof(double*)*NumCh);

    for(i = 0; i < NumCh; ++i)
        ret->pChBeg[i] = (ret->Data) + i*MaxNumSample;
    return ret;
}
// destructor-like function
void
DestroyWaveformStruct(WaveformStruct *w){
    free(w->Data);
    free(w->pChBeg);
    free(w);
}

double*
util_WFS_Lookup(WaveformStruct *w, int ch, int sample){
    return w->pChBeg[ch]+sample;
}

int
WriteWaveformData(WaveformStruct *w, int ch, int sample, double data){
    *util_WFS_Lookup(w,ch,sample) = data;
    return 0;
}

double
ReadWaveformData(WaveformStruct *w, int ch, int sample){
    return *util_WFS_Lookup(w,ch,sample);
}

void
waveformRecord(FILE *f, WaveformStruct *w, int ch){
    int i;
    for(i = 0; i < w->nSamples; ++i)
        fprintf(f,"%d,%.10f\n",i,*util_WFS_Lookup(w,ch,i));
}
