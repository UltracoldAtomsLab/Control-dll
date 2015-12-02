#include <stdio.h>
#include <stdlib.h>
#include "Struct_Algorithm.h"

#ifdef __cplusplus
extern "C"
{
#endif
// constructor-like function
ControlPointStruct*
CreateCtrlPtStruct(int NumCh, int MaxNumPoint, int NumAttr, int *nCpEachCh){
    ControlPointStruct *ret;
    int i;
    ret            =(ControlPointStruct*) malloc(sizeof(ControlPointStruct));
    ret->nChannels = NumCh;
    ret->nCpMax    = MaxNumPoint;
    ret->nAttr     = NumAttr;
    ret->Data      =(union CP_Data*) malloc(sizeof(union CP_Data)*NumCh*MaxNumPoint*NumAttr);
    ret->pChBeg    =(union CP_Data**) malloc(sizeof(union CP_Data*)*NumCh);
    ret->nCpEachCh =(int*) malloc(sizeof(int)*NumCh);
    for(i = 0; i < NumCh; ++i){
        ret->pChBeg[i] = (ret->Data) + i*MaxNumPoint*NumAttr;
        ret->nCpEachCh[i] = nCpEachCh[i];
    }
    ret->nInReserve = 0;
    return ret;
}
// destructor-like function
void
DestroyCtrlPtStruct(ControlPointStruct* c){
    int i;
    free(c->Data);
    free(c->pChBeg);
    free(c->nCpEachCh);
    for(i=0; i < c->nInReserve; ++i)
        free(c->lReserve[i]);
    free(c);
}

CP_Data*
util_CPS_Lookup(ControlPointStruct *c, int ch, int point, int attr){
    return c->pChBeg[ch]+point*c->nAttr+attr;
}

int
WriteCtrlPtDataD(ControlPointStruct *c, int ch, int point, int attr, double data){
    util_CPS_Lookup(c, ch, point, attr)->d = data;
    return 0;
}

int
WriteCtrlPtDataP(ControlPointStruct *c, int ch, int point, int attr, double *data, int size){
    double *d = (double*) malloc(sizeof(double)*size);
    int i;
    c->lReserve[c->nInReserve++] = d;
    for(i = 0; i < size; ++i)
        d[i] = data[i];
    util_CPS_Lookup(c, ch, point, attr)->dp = d;
    return 0;
}

CP_Data
ReadCtrlPtData(ControlPointStruct *c, int ch, int point, int attr){
    return *util_CPS_Lookup(c, ch, point, attr);
}


void
display(FILE *f, ControlPointStruct* c){
    int i,j,k,l;
    const int PointPerLine = 5;
    for(i = 0; i < c->nChannels; ++i){
        fprintf(f, "\nChannel %d:\n",i);
        fprintf(f, "the number of control point in this channel is %d.\n",c->nCpEachCh[i]);
        for(j = 0; j < c->nCpEachCh[i]; j += PointPerLine){
            fprintf(f, "point %d to %d\n",j+1, j+PointPerLine);
            for(k = 0; k < c->nAttr; ++k){
                fprintf(f, "Attr %d:  ",k);
                for(l = 0; l < PointPerLine; ++l){
                    if(j+l >= c->nCpEachCh[i])
                        break;
                    fprintf(f, "%10.2f", ReadCtrlPtData(c,i,j+l,k).d);
                }
                fprintf(f, "\n");
            }
            fprintf(f, "\n");
        }
        fprintf(f, "=======================================\n");
    }
}

#ifdef __cplusplus
}
#endif
