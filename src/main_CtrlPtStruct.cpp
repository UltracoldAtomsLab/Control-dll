#include "main.h"
#include "Struct_Algorithm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif


// compare the strings, the first with delimitor, the second is to compare
bool StringCompare(
    char *TheString,
    char *ComparingString,
    char  del,
    char **nextOfTheString
)
{
    int i;
    for(i = 0; ComparingString[i] != '\0'; ++i)
        if(TheString[i] != ComparingString[i])
            break;

    // the same through out the word
    if(ComparingString[i] == '\0' && TheString[i]==del)
    {
        (*nextOfTheString) = TheString + i + 1;
        return true;
    }

    // til next delimitor or the end
    for(;TheString[i]!=del && TheString[i]!='\0';++i);
    if(TheString[i]==del)
    {
        (*nextOfTheString) = TheString + i + 1;
        if(**nextOfTheString == '\n')
            (*nextOfTheString) = NULL;
        return false;
    }
    else
    {
        (*nextOfTheString) = NULL;
        return false;
    }
    return false;
}


double DLL_EXPORT
CtrlPt_InqueryProperty(
    char    *sCtrlPtFile,
    char    *sNameProperty
)
{
    FILE     *f;
    char      linbuf[2048];
    char      *next;
    bool      isTheSame;
    double    ret = 1.0/0.0;

    // open file
    f = fopen(sCtrlPtFile, "r");
    if(f == NULL){
        printf("unknown path: %s", sCtrlPtFile);
        return ret;
    }

    // find the length
    if(sNameProperty[0]=='\0')
    {
        fclose(f);
        return ret;
    }

    fseek ( f , 0 , SEEK_SET );
    // first line -- skip
    fgets(linbuf, 2048, f); // read 2048 charactoers or til EOF/NL

    // second line: the properties are stored here
    fgets(linbuf, 2048, f); // read 2048 charactoers or til EOF/NL

    next = linbuf;
    while(next != NULL)
    {
        isTheSame = StringCompare( next, sNameProperty, ',', &next);
        if(isTheSame)
        {
            if(next != NULL)
            {
                while(*next > '9' && *next <'0' && *next != ',' && *next != '\0')
                    ++next;
                if(*next <= '9' && *next >= '0')
                    ret = strtod(next,NULL);
            }
            break;
        }
    }
    fclose(f);
    return ret;
}

ControlPointStruct* DLL_EXPORT
CtrlPt_File_To_CtrlPtStruct(
    char *conf_path
){
    ControlPointStruct* ret;
    FILE* f;

    f = fopen(conf_path, "r");
    if(f == NULL){
        printf("unknown path: %s", conf_path);
        return NULL;
    }
    ret = Parse_FileTo2DArray(f);
    fclose(f);
    return ret;
}

void DLL_EXPORT
CtrlPt_Struct_Close(ControlPointStruct* c){
    FreeProductOf_Parse_FileTo2DArray(c);
}

CP_Data DLL_EXPORT
CtrlPt_DataRead(ControlPointStruct *c, int ch, int point, int attr){
    return *util_CPS_Lookup(c,ch,point,attr);
}


#ifdef __cplusplus
}
#endif
