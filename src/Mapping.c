#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Struct_Algorithm.h"

#ifdef __cplusplus
extern "C"
{
#endif

int
Map_ReadFromFile(FILE *f, double **key, double **value)
{
    // FILE *f:        the file pointer
    // double **key:   output-the lookup key
    // double **value: output-the lookup value
    // int:            return the size of the array

    int domain_size;
    int i;
    int mode;
    char linebuf[1024], *cur;
    double *x, *fx;

    // counting the line number, which is also the number of key-value pairs
    domain_size = 0;
    fseek ( f , 0 , SEEK_SET ); // back to the beginning

    // check whether the first column is x or f(x)
    fgets(linebuf,1024,f);
    if(linebuf[0]=='x')
        mode = 0;
    else if(linebuf[0] =='f' && linebuf[1]=='(' && linebuf[2] == 'x' && linebuf[3] == ')')
        mode = 1;
    else
    {
        return -1;
    }

    while(!feof(f)){
        fgets(linebuf,1024,f);
        ++domain_size;
    }

    *key   = x  = (double*)malloc(sizeof(double)*domain_size);
    *value = fx = (double*)malloc(sizeof(double)*domain_size);

    // parsing and storing the key-value
    i = 0;
    fseek ( f , 0 , SEEK_SET ); // back to the beginning
    fgets(linebuf,1024,f); // skipping the first line
    while(!feof(f)){
        fgets(linebuf,1024,f);
        if(mode == 0)
        {
            // the first column is x
            x[i]  = strtod(linebuf, &cur);
            fx[i] = strtod(cur+1, &cur);
        }
        else
        {
            // the first column is f(x)
            fx[i] = strtod(linebuf, &cur);
            x[i]  = strtod(cur+1, &cur);
        }
        ++i;
    }
    --i;
    if(fx[i] == fx[i-1] && x[i] == x[i-1]) // repeated reading due to the csv format issue
        --domain_size;

	// keep the key value ascendant, invert the array if necessary
	if(x[0] > x[1]){
		*key   = (double*)malloc(sizeof(double)*domain_size);
		*value = (double*)malloc(sizeof(double)*domain_size);
		for(i = 0; i < domain_size; ++i){
			(*key)[i]   = x[domain_size-1-i];
			(*value)[i] = fx[domain_size-1-i];
		}
		free(x);
		free(fx);
	}

    return domain_size;
}


int
FreeProductOf_Map_ReadFromFile(double *key, double *value){
    free(key);
    free(value);
    return 0;
}

int
Map_LookupPowerToVoltage(double *mapKey, double *mapValue, int size, int ch, WaveformStruct *w){
    // double *mapKey:  input-key of the map
    // double *mapValue:   input-value of the map
    // int Ch:            the channel to be mapped
    // WaveformStruct* w: inout-modified wave as output

    int     i, iMap;
    double  dWaveCur, dWavePre;
    double *slope, *wavedata;
#ifdef TIMING_EXE
    FILE *f = fopen(ch==8?"8.txt":"9.txt","w");
#endif

    // implement slope[] for first order hold, to save time
    // the zeroth component of slope[] is not accessible
    slope = (double*)malloc(sizeof(double)*size);
    for(iMap = 1; iMap < size; ++iMap){
        slope[iMap] = (mapValue[iMap]-mapValue[iMap-1])/(mapKey[iMap]-mapKey[iMap-1]);
#ifdef TIMING_EXE
        printf("slope[%d] = %.7f\n",iMap,slope[iMap]);
#endif
    }
    iMap = 1;

    wavedata = util_WFS_Lookup(w, ch, 0);
    dWaveCur = *wavedata - 1;// just make it diff, not to go to the onset of if-control at the first time
    for(i = 0; i < w->nSamples; ++i){
        dWavePre = dWaveCur;
        dWaveCur = *wavedata;
#ifdef TIMING_EXE
        fprintf(f,"%.7f, %.7f\n",dWavePre, dWaveCur);
#endif
        if(dWavePre==dWaveCur){
            *wavedata = *(wavedata-1);
            ++wavedata;
        }
        else{
            while(dWaveCur>mapKey[iMap] && iMap < size)
                ++iMap;
            while(dWaveCur<mapKey[iMap-1] && iMap > 0)
                --iMap;
            if(iMap == 0)
                *(wavedata++) = mapValue[iMap++]; // ensure iMap being within 1~(size-1)
            else if(iMap == size)
                *(wavedata++) = mapValue[--iMap]; // ensure iMap being within 1~(size-1)
            else{
                *(wavedata++) = mapValue[iMap-1] + slope[iMap]*(dWaveCur-mapKey[iMap-1]);
            }
        }
    }
#ifdef TIMING_EXE
    fclose(f);
#endif
    return 0;
}

static void inline copy_and_assure_last_semicolon(const char* ref, char *cur){
    int i;
    for(i = 0; ref[i]!='\0';++i)
        cur[i] = ref[i];
    if(i!=0 && cur[i-1] != ';'){
        cur[i]=';';
        ++i;
    }
    cur[i] = '\0';
}

int Map_Mapping(
    char           *s_channels_to_map,
    char           *s_map_files,
    WaveformStruct *w
){
    FILE     *f;
    int      chs[16];
    char     *(map_files[16]);
    char     s_chs[128];
    char     s_map[1024];
    char     *cur   = s_chs;
    char     *cur_p = s_map;
    int      nChsToMap = 0;
    int      nMap;
    double   *key, *value;
    int      i;

    // put into the internal memory
    copy_and_assure_last_semicolon(s_channels_to_map, s_chs);
    copy_and_assure_last_semicolon(s_map_files, s_map);

    while(cur != NULL && *cur != '\0'){
        // parse the channels
        chs[nChsToMap] = atoi(cur);
        cur = strchr(cur,';');     // must be found by the previous assuring

        //parse the paths
        map_files[nChsToMap] = cur_p;
        cur_p = strchr(cur_p,';'); // must be found by the previous assuring
        cur_p[0] = '\0';

        ++nChsToMap;
        ++cur;
        ++cur_p;
    }

    for(i = 0; i < nChsToMap; ++i){
        f = fopen(map_files[i],"r");
        if(f == NULL){
            printf("unknown path: %s", map_files[i]);
            return -1.0;
        }
        nMap = Map_ReadFromFile(f, &key, &value);
        fclose(f);
        Map_LookupPowerToVoltage(key,value,nMap,chs[i],w);
        FreeProductOf_Map_ReadFromFile(key,value);
    }
    return 0;
/*
    f = fopen(YAG1_path,"r");
    if(f == NULL){
        printf("unknown path: %s", conf_path);
        return -1.0;
    }
    ch = 8;
    nMap = Map_ReadFromFile(f, &key, &value);
    fclose(f);
    Map_LookupPowerToVoltage(key,value,nMap,ch,w);
    FreeProductOf_Map_ReadFromFile(key,value);

    f = fopen(YAG2_path,"r");
    if(f == NULL){
        printf("unknown path: %s", conf_path);
        return -1.0;
    }
    ch = 9;
    nMap = Map_ReadFromFile(f, &key, &value);
    fclose(f);
    Map_LookupPowerToVoltage(key,value,nMap,ch,w);
    FreeProductOf_Map_ReadFromFile(key,value);
*/
}

#ifdef __cplusplus
}
#endif
