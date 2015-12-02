#ifdef D_DLL
#include "main.h"
#endif
#include "Struct_Algorithm.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef __cplusplus
extern "C"
{
#endif


#ifdef D_DLL
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            // attach to process
            // return FALSE to fail DLL load
            break;

        case DLL_PROCESS_DETACH:
            // detach from process
            break;

        case DLL_THREAD_ATTACH:
            // attach to thread
            break;

        case DLL_THREAD_DETACH:
            // detach from thread
            break;
    }
    return TRUE; // succesful
}
#endif





#ifdef D_DLL
double DLL_EXPORT Analog_Timing(
    double loop,
    double sample_rate,
    char *conf_path,
    char *s_channels_to_map,
    char *s_map_files,
    // data storage information
    int     mem_size,
    double  *ret, //p_LvArray ret,
    int     *samples_each,
    int     *channels_num
){
#else
int main(){
    double   loop = 0.0;
    double   sample_rate = 20.0;// this defines the number of sample in a time unit of ctrl point timing file
    char     conf_path[128] = "MOT_Conf.csv";
    char     s_channels_to_map[128] = "";//"9;10;";//"";
    char     s_map_files[128] = "";//"fitYAG1.csv;fitYAG2.csv;";
    char     filename[128];
#endif
    FILE     *f;
    int      nSample;
    ControlPointStruct  *c;
    WaveformStruct      *w;
    int      t;

    t = clock();
    // parsing timing data
    f = fopen(conf_path, "r");
    if(f == NULL){
        printf("unknown path: %s", conf_path);
        return -1.0;
    }
    c = Parse_FileTo2DArray(f);
    fclose(f);

    // build waveform
    nSample = FindWaveformSampleNum(c, loop, sample_rate);
    w = CreateWaveformStruct(c->nChannels, nSample);
    Build_AllChannel(c,w,loop,sample_rate);


    // further mapping the waveform
    Map_Mapping(s_channels_to_map,s_map_files, w);

    // putting the data
#ifdef D_DLL
    *samples_each = w->nSamples;
    *channels_num = w->nChannels;

    int i;
    int index_max = (w->nChannels)*(w->nSamples);
    for(i = 0; i < mem_size && i < index_max; ++i)
        ret[i] = w->Data[i];
    if ( i == mem_size && i != index_max)
        fprintf(stderr,"Run out of the provided container, larger array required!\n");

    // clearing the data
    FreeProductOf_Parse_FileTo2DArray(c);
    DestroyWaveformStruct(w);

    t = clock() - t;
    return (double)t/(double)CLOCKS_PER_SEC;

#else

    t = clock() - t;
    printf("the time spent is %.4f\n", (double)t/(double)CLOCKS_PER_SEC);

    f = fopen("CtrlPtRead.txt","w");
    display(f, c);
    fclose(f);
    for(int ch = 0; ch < w->nChannels; ++ch){
        sprintf(filename,"ch%d.csv",ch);
        f = fopen(filename,"w");
        waveformRecord(f,w,ch);
        fclose(f);
    }
    return 0;
#endif
}

#ifdef D_DLL
int DLL_EXPORT Analog_inquery_array_size(
    double loop,
    double sample_rate,
    char *conf_path
){
    FILE     *f;
    int      nSample, nRet;
    ControlPointStruct  *c;

    f = fopen(conf_path, "r");
    c = Parse_FileTo2DArray(f);
    fclose(f);

    // build waveform
    nSample = FindWaveformSampleNum(c, loop, sample_rate);
    nRet = c->nChannels*nSample;
    FreeProductOf_Parse_FileTo2DArray(c);

    return nRet;
}
#endif

#ifdef __cplusplus
}
#endif
