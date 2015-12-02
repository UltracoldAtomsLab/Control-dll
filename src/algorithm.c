#define _USE_MATH_DEFINES
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
//#include <assert.h>
#include "Struct_Algorithm.h"

#ifdef __cplusplus
extern "C"
{
#endif
// ******************************************************
// **                  Main Algorithm                  **
// ******************************************************
ControlPointStruct*
Parse_FileTo2DArray(FILE *f)
{
    // FILE *f: input-file, number of boards, number of channels per board
    // return:  output-control point data
    char  linbuf[2048], *cur, *cur2;
    ControlPointStruct *ret;

    int nBoards;
    int nAllCh, nCh_OnEachBoard[128];
    int *nCPt_OnEachChannel, MaxNumPoint;
    int NumAttr = 8;

    int i, j, sum, bd, ch, point, attr;
    enum VariationShape s;
    double forSine[4];

    /* pseudo code
    parse needed info for the constructor
    ret = CreateCtrlPtStruct(nAllCh, MaxNumPoint, NumAttr);
    for each Channel
        for each attribute
            if attribute is 2nd reserved
                for each point
                    if the previous attribute of this point is sinusoidal
                        parse subvalue inside this segment
                    else
                        WriteCtrlPtData(ret, ich, ipoint, iattr, parsed_data); // to transfer FILE to structure
            else
                for each point
                    WriteCtrlPtData(ret, ich, ipoint, iattr, parsed_data); // to transfer FILE to structure
    */

    // first line: parsing the number of boards and how many channels on each board
    fseek ( f , 0 , SEEK_SET );
    fgets(linbuf, 2048, f); // read 2048 charactoers or til EOF/NL
    nCh_OnEachBoard[0] = strtol(linbuf, &cur, 10); //cells[1][1] in the excel
    for(i = 1; i < 128; ++i)
    {
        nCh_OnEachBoard[i] = strtol(cur+1, &cur, 10);  //cells[1][i+1]
        if(nCh_OnEachBoard[i]==0)
        {
            nBoards = i;
            break;
        }
    }
    //assert(i!=128);

    // sum up how many channels total, and allocate the array for storing numbers of ctrl points of all channels.
    for(j = 0, sum=0; j < i; ++j)
        sum += nCh_OnEachBoard[j];
    nAllCh = sum;
    nCPt_OnEachChannel = (int*) malloc(sizeof(int)*(nAllCh));

    // second line of the csv: other attributes, to skip
    // it would be skipped by the next fgets()
    fgets(linbuf, 2048, f); // read 2048 charactoers or til EOF/NL

    // third line starts:
    MaxNumPoint = 0;
    for(bd = 0, i = 0; bd < nBoards; ++bd)
    {
        fgets(linbuf, 2048, f); // read 2048 charactoers or til EOF/NL
        cur = linbuf-1;
        for(ch = 0; ch < nCh_OnEachBoard[bd]; ++ch)
        {
            nCPt_OnEachChannel[i+ch] = strtol(cur+1, &cur, 10); // cells[3+bd][ch+1] in excel
            if (MaxNumPoint < nCPt_OnEachChannel[ch])
                MaxNumPoint = nCPt_OnEachChannel[ch];
        }
        i += nCh_OnEachBoard[bd];
    }

    // build the manager
    ret = CreateCtrlPtStruct(nAllCh, MaxNumPoint, NumAttr, nCPt_OnEachChannel);

    // next fgets function would get the first line of the very first channel
    for(ch = 0; ch < nAllCh; ++ch)
    {
        // running over the fields in csv file
        for(attr = 0; attr < NumAttr; ++attr)
        {
            fgets(linbuf, 2048, f);
            cur = linbuf-1;
            // the treatment of attribute is divided into two parts, the 2nd reserved row and the others.
            // for the 2nd reserved row, it is possible to have more than one double-precision number in
            // an excel cell, e.g. Sine case.
            // so we have to handle this case by the following if-control
            if(attr != eAttrReserve)// the other cases
            {
                for(point = 0; point < nCPt_OnEachChannel[ch]; ++point) // point as the order of point
                    WriteCtrlPtDataD(ret, ch, point, attr, strtod(cur+1, &cur));
            }
            else// the 2nd reserved row
            {
                for(point = 0; point < nCPt_OnEachChannel[ch]; ++point) // point as the order of point
                {
                    // read the shape, to see whether it is problematic sinusoidal
                    s = (VariationShape)ReadCtrlPtData(ret, ch, point, eAttrShape).d;
                    if(s == eShapeSine)
                    {
                        // sub values seperated by ";"
                        cur2 = strtok(cur+1,";");
                        for(i = 0; i < 4; ++i)
                        {
                            forSine[i] = (cur2 != NULL)? strtod(cur2,NULL) : 0.0;
                            if(cur2!=NULL)
                                cur2 = strtok(NULL,";");
                        }
                        WriteCtrlPtDataP(ret, ch, point, attr, forSine, 4/*forSine size*/);
                        //updating cur
                        ++cur;// first time required since it was ended up with a "," last time.
                        while(*cur!=',' && *cur!='0')
                            ++cur;
                    }
                    else // if not sinusoidal, simply write the value
                    {
                        WriteCtrlPtDataD(ret, ch, point, attr, strtod(cur+1, &cur));
                    }
                }
            }
        }
    }
    // finalizing
    free(nCPt_OnEachChannel);
    return ret;
}
int
FreeProductOf_Parse_FileTo2DArray(ControlPointStruct *c)
{
    DestroyCtrlPtStruct(c);
    return 0;
}
int
FindWaveformSampleNum(ControlPointStruct *c, double loop, double sample_rate)
{
    // find the maximum time of each channel
    // and then multiply it by sample_rate and return.
    int ch;
    double ret = 0;
    double val, val2, valInc, val2Inc, value;
    for(ch = 0; ch < c->nChannels; ++ch){
        val     = ReadCtrlPtData(c,ch,c->nCpEachCh[ch]-1,eAttrTime).d;
        val2    = ReadCtrlPtData(c,ch,c->nCpEachCh[ch]-1,eAttrInter).d;
        valInc  = ReadCtrlPtData(c,ch,c->nCpEachCh[ch]-1,eAttrTimeInc).d;
        val2Inc = ReadCtrlPtData(c,ch,c->nCpEachCh[ch]-1,eAttrInterInc).d;
        value   = val + valInc*loop  +  val2 + val2Inc*loop;
        if(value > ret)
            ret = value;
    }
    return lround(ret*sample_rate)+1;
}

int
Build_OneChannel(ControlPointStruct *c, WaveformStruct *w, double loop, double sample_rate, int ch)
{
#ifndef D_DLL
    int verbose = 1;
#endif

    int iCPt, iWPt;
    const CP_Data *curr;
    double dVCurr, dVPrev;     // double Value Current/Previous
    int iTCurr, iTPrev, iICurr; // integer Time Current/Previous, integer Interval Current
    int ret = 0;
    int i;

    double data;
    double slope, offset; // for linear, adiabatic
    double time_const, amp/*, offset*/, E; //for exponential
    double freq/*, amp*/;

    //for the case iCPt = 0, the initial value
    iWPt = 0;
    iCPt = 0;
    curr = util_CPS_Lookup(c,ch,iCPt,0);
    dVCurr = curr[eAttrValue].d + curr[eAttrValueInc].d*loop;
    WriteWaveformData(w, ch, iWPt++, dVCurr);

#ifndef D_DLL
    if(verbose)
        printf("iWPt(after) = %d\n",iWPt);
#endif

    // for regular cases
    for(iCPt = 1; iCPt < c->nCpEachCh[ch]; ++iCPt)
    {

        curr = util_CPS_Lookup(c,ch,iCPt,0);
        iTPrev = iWPt;
        iTCurr = lround((curr[eAttrTime].d  + curr[eAttrTimeInc].d*loop) *sample_rate);
        iICurr = lround((curr[eAttrInter].d + curr[eAttrInterInc].d*loop)*sample_rate);
        dVPrev = dVCurr;
        dVCurr = curr[eAttrValue].d + curr[eAttrValueInc].d*loop;
        //printf("==================iICurr = %d\n", iICurr);

        // chech previous last sample is earlier than current one
        // if so, padding the last value til (including) iTCurr.
        if(iTPrev > iTCurr + 1){
            ret = 1;
            fprintf(stderr,"Timing error for Channel %d, Control point %d\n", ch, iCPt);
            iWPt = iTCurr + 1;
        }
        else while(iWPt <= iTCurr)
            WriteWaveformData(w, ch, iWPt++, dVPrev);

#ifndef D_DLL
        if(verbose){
            printf("iTPrev = %d, iTCurr = %d, iWPt(after) = %d\n",iTPrev,iTCurr,iWPt);
        }
#endif


        switch(lround(curr[eAttrShape].d)){
            case eShapeLinear:
            case eShapeLinear_dup:
                offset = dVPrev;
                slope = (dVCurr - offset)/(double)iICurr;
                data = offset;
                for(i = 1; i <= iICurr; ++i){
                    data += slope;// equivalent to: offset + slope*(double)i;
                    WriteWaveformData(w, ch, iWPt++, data);
                }
                //assert(iWPt == iTCurr + iICurr + 1);
                // resulting in Waveform[iTCurr] = dVPrev, Waveform[iTCurr+iICurr] = dVCurr;
            break;
            case eShapeAdiabatic:
                offset = 1/sqrt(dVPrev);
                slope = (1/sqrt(dVCurr)-offset)/(double)iICurr;
                data = offset;
                for(i = 1; i <= iICurr; ++i){
                    data += slope;
                    WriteWaveformData(w, ch, iWPt++, 1.0/pow(data,2));
                }
                //assert(iWPt == iTCurr + iICurr + 1);
            break;
            case eShapeExponential:
                // algorithm:
                //     A*exp(    0/tc)+c = Vprev
                //     A*exp(t_int/tc)+c = Vcurr
                //     both t_int and tc are in unit integer, and solving for A and c to draw
                time_const = curr[eAttrReserve].d;
                if(time_const==0.0){
                    ret = 1;
                    fprintf(stderr,"Dividing zero error for Channel %d, Control point %d\n", ch, iCPt);
                    for(i = 1; i <= iICurr; ++i)
                        WriteWaveformData(w, ch, iWPt++, dVCurr);
                    //assert(iWPt == iTCurr + iICurr + 1);
                }
                else{
                    time_const = (dVCurr>dVPrev)? time_const : -time_const;
                    E = exp((double)iTCurr/time_const);
                    amp = (dVCurr-dVPrev)/(E-1.0);
                    offset = (dVCurr-dVPrev*E)/(1.0-E);

                    data = amp;
                    E = exp(1.0/time_const);
                    for(i = 1; i <=iICurr; ++i){
                        data *= E;
                        WriteWaveformData(w, ch, iWPt++, data+offset);
                    }
                    //assert(iWPt == iTCurr + iICurr + 1);
                }
            break;
            case eShapeSine:
                if(dVCurr != dVPrev){
                    ret = 1;
                    fprintf(stderr, "Not match voltage value in sine shape for Channel %d, Control point %d\n", ch, iCPt);
                }
                amp  = (curr[eAttrReserve].dp)[0] + (curr[eAttrReserve].dp)[2]*loop;
                freq = (curr[eAttrReserve].dp)[1] + (curr[eAttrReserve].dp)[3]*loop;
                freq *= (2.0*M_PI/sample_rate); // make it an angular frequency
                /* // if the freq means cycles inside the interval
                freq = (curr[eAttrReserve].dp)[1] + (curr[eAttrReserve].dp)[3]*loop;
                freq *= (2*M_PI/iICurr)
                */
                data = 0;
                //printf("iICurr=%d, iWPt=%d, dVCurr=%f, amp = %f, freq = %f\n\n", iICurr, iWPt, dVCurr, amp, freq);
                for(i = 1; i <=iICurr; ++i){

                    data += freq;
                    WriteWaveformData(w, ch, iWPt++, dVCurr+amp*sin(data));
                    //printf("%f\t", dVCurr+amp*sin(data));
                }
                WriteWaveformData(w, ch, iWPt-1, dVCurr); // assure it being dVCurr regardless the not closed cycle.
                //assert(iWPt == iTCurr + iICurr + 1);
            break;
        }
    }
    //assert(iWPt <= w->nSamples);

    // padding last value to the end
    dVCurr = ReadWaveformData(w, ch, iWPt-1);
    while(iWPt < w->nSamples)
        WriteWaveformData(w, ch, iWPt++, dVCurr);
    return ret;
}
int
Build_AllChannel(ControlPointStruct *c, WaveformStruct *w, double loop, double sample_rate)
{
    // struct's: input-the structures storing necessary data
    // int loop: input-which (ordinal number) loop is this
    // The WaveformStruct input through pointer is modified.
    int i;
    for (i = 0; i < c->nChannels; ++i)
        Build_OneChannel(c,w,loop,sample_rate,i);
    return 0;
}

#ifdef __cplusplus
}
#endif
