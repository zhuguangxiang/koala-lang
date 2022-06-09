#include "stackmap-reader.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#define FIRST_FUNC_OFFSET 10
uint32_t numOfRecords = 0;

void readLocations(locationInfo *locations, uint16_t numOfLocations)
{
    printf(
        "\t***************** Locations(Marked References)Info "
        "*****************\n");
    for (int i = 0; i < numOfLocations; i++) {
        if (locations->kind == 4) {
            locations = locations + 1;
            continue;
        }
        printf("\tKind : %d\n", locations->kind);
        printf("\tFlags : %d\n", locations->flags);
        printf("\tLocation Size : %d\n", locations->locSize);
        printf("\tReg Num : %d\n", locations->regNum);
        printf("\tReserved : %d\n", locations->reserved);
        printf("\tOffset : %d\n\n", locations->offset);
        locations = locations + 1;
    }
}

void readRecords(recordInfo *recordsInfo)
{
    printf("***************** Records(Safe Points) Info *****************\n");
    for (int i = 0; i < numOfRecords; i++) {
        printf("Patch id : %ld\n", recordsInfo->id);
        printf("Code Offset : %d\n", recordsInfo->codeOffset);
        printf("Flags : %d\n", recordsInfo->flags);
        uint16_t numOfLocations = recordsInfo->numLocations;
        printf("Number of locations (marked references) : %d\n\n",
               numOfLocations);
        readLocations((locationInfo *)(((uint16_t *)recordsInfo) + 8),
                      numOfLocations);

        uint8_t *temp = ((uint8_t *)recordsInfo) + numOfLocations * 12 + 16;
        printf("Padding : %d\n", *((uint32_t *)temp));
        printf("Padding : %d\n", *((uint16_t *)(temp + 4)));
        int numLiveOuts = *((uint16_t *)(temp + 6));
        printf("Number of LiveOuts : %d\n\n", numLiveOuts);
        temp = temp + 8 + numLiveOuts * 4;
        recordsInfo = (recordInfo *)(temp + 4);
    }
}

void readFuncInfo(functionInfo *functionInfo, uint32_t numOfFunc,
                  uint32_t numOfConsts)
{
    printf(
        "***************** Functions (which enable GCs) Info "
        "*****************\n");
    for (int i = 0; i < numOfFunc; i++) {
        printf("Function address : %lu\n", functionInfo->address);
        printf("Stack size : %ld\n", functionInfo->stackSize);
        printf("Function calls count(safepoints) : %ld\n\n",
               functionInfo->callsiteCount);
        functionInfo = functionInfo + 1;
    }
    readRecords((recordInfo *)((uint32_t *)functionInfo + numOfConsts));
}

void readHeader(stackMapHeader *header)
{
    printf("Number of function (which enable GCs) : %d\n",
           header->numFunctions);
    printf("Number of constants : %d\n", header->numConstants);
    numOfRecords = header->numRecords;
    printf("Total number of function calls(safepoints) : %d\n\n", numOfRecords);

    readFuncInfo((functionInfo *)(header + 1), header->numFunctions,
                 header->numConstants);
}

stackMap *stackMapPointer;

stackMap *newStackMap()
{
    return malloc(sizeof(stackMap));
}

function *newFunction()
{
    function fn;
    return malloc(sizeof(fn.address) + sizeof(fn.stackSize) +
                  sizeof(fn.stackSize));
}

int calSizeOfCallSite(int callSiteSizeInBytes, void *recordPtr)
{
    recordPtr = (uint16_t *)recordPtr + 3;
    callSiteSizeInBytes = callSiteSizeInBytes + 6;
    int numOfLiveOuts = *(uint16_t *)recordPtr;
    recordPtr = (uint16_t *)recordPtr + 1;
    callSiteSizeInBytes = callSiteSizeInBytes + 2;
    callSiteSizeInBytes = callSiteSizeInBytes + numOfLiveOuts * 4 + 4;
    return callSiteSizeInBytes;
}

void readLocation(locationInfo *locationPtr, locationInfo *tempLocationPtr)
{
    locationPtr->kind = tempLocationPtr->kind;
    locationPtr->flags = tempLocationPtr->flags;
    locationPtr->locSize = tempLocationPtr->locSize;
    locationPtr->regNum = tempLocationPtr->regNum;
    locationPtr->reserved = tempLocationPtr->reserved;
    locationPtr->offset = tempLocationPtr->offset;
}

int readCallSite(void *recordPtr, callSiteInfo *callSiteInfo)
{
    int callSiteSizeInBytes = 0;
    callSiteInfo->id = *(uint64_t *)recordPtr;
    recordPtr = (uint64_t *)recordPtr + 1;
    callSiteSizeInBytes = callSiteSizeInBytes + 8;
    callSiteInfo->codeOffset = *(uint32_t *)recordPtr;
    recordPtr = (uint32_t *)recordPtr + 1;
    callSiteSizeInBytes = callSiteSizeInBytes + 4;
    callSiteInfo->flags = *(uint16_t *)recordPtr;
    recordPtr = (uint16_t *)recordPtr + 1;
    callSiteSizeInBytes = callSiteSizeInBytes + 2;
    uint16_t numLocations = *(uint16_t *)recordPtr;
    callSiteInfo->numLocations = numLocations;

    locationInfo *locationsInfo = calloc(numLocations, sizeof(locationInfo));
    callSiteInfo->locations = locationsInfo;
    locationInfo *loc = (locationInfo *)((uint16_t *)recordPtr + 1);
    callSiteSizeInBytes = callSiteSizeInBytes + 2;
    for (size_t i = 0; i < numLocations; i++) {
        readLocation(locationsInfo + i, loc + i);
    }
    callSiteSizeInBytes = callSiteSizeInBytes + numLocations * 12;
    recordPtr = (uint8_t *)recordPtr + numLocations * 12;
    return calSizeOfCallSite(callSiteSizeInBytes, recordPtr);
}

int readFunction(functionInfo *funcPtr, void *recordPtr, function *fn)
{
    fn->address = funcPtr->address;
    fn->stackSize = funcPtr->stackSize;
    fn->callsiteCount = funcPtr->callsiteCount;
    callSiteInfo *callSitesInfo =
        calloc(fn->callsiteCount, sizeof(callSiteInfo));
    fn->callSitesInfo = callSitesInfo;

    int callSitesSizeInBytes = 0;
    for (int i = 0; i < fn->callsiteCount; i++) {
        int callSiteSizeInBytes = readCallSite(recordPtr, callSitesInfo + i);
        recordPtr = (uint8_t *)recordPtr + callSiteSizeInBytes;
        callSitesSizeInBytes = callSitesSizeInBytes + callSiteSizeInBytes;
    }
    return callSitesSizeInBytes;
}

void dump()
{
    printf("\n%s\n", "***** New stack map *****");
    printf("Num of functions : %d\n", stackMapPointer->numFunctions);
    printf("Total number of call sites : %d\n\n", stackMapPointer->numRecords);

    if (stackMapPointer->numFunctions)
        printf("%s\n", "***** Dump functions info *****");

    function *fn = stackMapPointer->functions;
    for (size_t i = 0; i < stackMapPointer->numFunctions; i++) {
        printf("Function address : %lx\n", fn->address);
        printf("Function stack size : %lu\n", fn->stackSize);
        printf("Function call site count : %lu\n", fn->callsiteCount);

        callSiteInfo *callSiteInfo = fn->callSitesInfo;
        for (size_t j = 0; j < fn->callsiteCount; j++) {
            printf("\tID : %lu\n", callSiteInfo->id);
            printf("\tFlags : %d\n", callSiteInfo->flags);
            printf("\tCode offset : %d\n", callSiteInfo->codeOffset);
            printf("\tLocations : %d\n", callSiteInfo->numLocations);

            locationInfo *loc = callSiteInfo->locations;
            for (size_t k = 0; k < callSiteInfo->numLocations; k++) {
                printf("\t\tKind : %d\n", loc->kind);
                printf("\t\tFlag : %d\n", loc->flags);
                printf("\t\tLocation Size : %d\n", loc->locSize);
                printf("\t\tRegister Num : %d\n", loc->regNum);
                printf("\t\tOffset : %d\n", loc->offset);
                loc = loc + 1;
            }
            callSiteInfo = callSiteInfo + 1;
        }
        fn = fn + 1;
    }
}

void readStackMap(uint64_t *t)
{
    void *stackMapPrt = (void *)t;
    stackMapPointer = newStackMap();
    // printf("\nStackMap location : %p\n", stackMapPrt);
    uint8_t *version = (uint8_t *)stackMapPrt;
    // printf("StackMap version : %d\n\n", *version);

    stackMapPrt = ((uint8_t *)stackMapPrt) + 4; // skip header 8*4
    uint32_t numOfFuncs = *(uint32_t *)stackMapPrt;
    stackMapPointer->numFunctions = numOfFuncs;

    stackMapPrt = ((uint8_t *)stackMapPrt) + 4; // skip functions count 8*4
    uint32_t numOfConstants = *(uint32_t *)stackMapPrt;

    stackMapPrt = ((uint8_t *)stackMapPrt) + 4; // skip constants count 8*4
    stackMapPointer->numRecords = *(uint32_t *)stackMapPrt;

    stackMapPrt = ((uint8_t *)stackMapPrt) + 4; // skip records count 8*4

    void *stackMapRecordPointer =
        (uint64_t *)stackMapPrt + numOfFuncs * 3 + numOfConstants;
    functionInfo *functionPtr = (functionInfo *)stackMapPrt;

    function *functionsInfo = calloc(numOfFuncs, sizeof(function));
    stackMapPointer->functions = functionsInfo;

    for (size_t i = 0; i < numOfFuncs; i++) {
        int callSitesSizePerFuncInBytes =
            readFunction(functionPtr, stackMapRecordPointer, functionsInfo + i);
        functionPtr = functionPtr + 1;
        stackMapRecordPointer =
            (uint8_t *)stackMapRecordPointer + callSitesSizePerFuncInBytes;
    }
    dump();
}

int *getHeapRefs(uint64_t *callLocations, int numOfCallLocations)
{
    int stackSize = 0;
    int *offsets = calloc(1, sizeof(int));
    *offsets = 0xdeadbeaf;
    int numOfOffsets = 0;
    function *firstFunc = stackMapPointer->functions;
    for (size_t cl = 1; cl < numOfCallLocations + 1; cl++) {
        uint64_t callLoc = *(callLocations + cl);
        printf("callLoc:%lx\n", callLoc);
        // if (cl == 1) callLoc = callLoc - FIRST_FUNC_OFFSET;
        uint8_t matched = 0;
        for (size_t f = 0; f < stackMapPointer->numFunctions; f++) {
            function *fn = firstFunc + f;
            uint64_t addr = fn->address;
            callSiteInfo *firstRec = fn->callSitesInfo;
            for (size_t r = 0; r < fn->callsiteCount; r++) {
                callSiteInfo *rec = firstRec + r;
                if (callLoc - rec->codeOffset != addr) {
                    continue;
                }
                matched = 1;
                locationInfo *firstLocInfo = rec->locations;
                uint16_t nl = rec->numLocations;
                for (size_t l = 3; l < nl; l = l + 2) {
                    numOfOffsets = numOfOffsets + 1;
                    *offsets = numOfOffsets;
                    offsets =
                        realloc(offsets, sizeof(int) * (numOfOffsets + 1));
                    *(offsets + numOfOffsets) =
                        (firstLocInfo + l)->offset + stackSize;
                }
                stackSize = stackSize + fn->stackSize;
                break;
            }
            if (matched == 1) break;
        }
    }
    return offsets;
}
