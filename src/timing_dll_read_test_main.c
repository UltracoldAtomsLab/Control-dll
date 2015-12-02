#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#pragma pack(push)
#pragma pack(1)
typedef struct{
    int dimSize;
    double data[1];
}t_LvArray, *p_LvArray, **h_LvArray;
#pragma pack(pop)


int main()
{
    // *************** declaration ********************
    // load the library
    HINSTANCE hGetProcIDDLL = LoadLibrary("C:\\Users\\Herbert\\Documents\\c\\timing_dll\\bin\\Debug\\timing_dll.dll");
    if(hGetProcIDDLL == NULL){
        printf("load library error\n");
        return 1;
    }
    // declare the function pointer

    //double (__cdecl *Timing)(double,double,char*,char*,char*,int,double*,int*,int*);
    double (__cdecl *Timing)(double,double,char*,char*,char*,p_LvArray,int*,int*);
    double spent_time;
    p_LvArray  cont = (p_LvArray)malloc(sizeof(t_LvArray)+sizeof(double)*1000000);
    cont->dimSize = 1000000;
    int    sample_num_each, channel_num;
    int    i;

    // assign the function pointer
    //Timing = (double (__cdecl*)(double,double,char*,char*,char*,int,double*,int*,int*)) GetProcAddress(hGetProcIDDLL,"Timing");
    Timing = (double (__cdecl*)(double,double,char*,char*,char*,p_LvArray,int*,int*)) GetProcAddress(hGetProcIDDLL,"Timing");
    if(Timing!=NULL)
        spent_time = Timing(
                        0,               //loop
                        20,              //sample_rate
                        "C:\\Users\\Herbert\\Documents\\c\\timing_dll\\bin\\Debug\\MOTConf.csv",   //conf_path
                        "C:\\Users\\Herbert\\Documents\\c\\timing_dll\\bin\\Debug\\fitYAG1.csv",   //Yag1
                        "C:\\Users\\Herbert\\Documents\\c\\timing_dll\\bin\\Debug\\fitYAG2.csv",   //Yag2
                        //1000000,         //mem_size
                        cont,            //container
                        &sample_num_each,//sample_each
                        &channel_num     //channels_num
                        );
    else{
        printf("function read error\n");
        return 1;
    }

    FILE *f = fopen("test0.csv","w");
    for(i = 0; i < sample_num_each; ++i)
        fprintf(f,"%d,%.10f\n",i,cont->data[sample_num_each+i]);
    fclose(f);
    printf("Hello world!\n");
    free((void*)cont);
    return 0;
}
