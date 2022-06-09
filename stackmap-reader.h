#include <stdint.h>

void readStackMap(uint64_t* stackMapPrt);

void printNumOfFunctions();

void findFuncCalls(int* loc);

typedef struct __attribute__((packed)) {
    uint8_t version;
    uint8_t reserved1;
    uint16_t reserved2;
    uint32_t numFunctions;
    uint32_t numConstants;
    uint32_t numRecords;
} stackMapHeader;

typedef struct __attribute__((packed)) {
    uint64_t address;
    uint64_t stackSize;
    uint64_t callsiteCount;  
} functionInfo;

typedef struct __attribute__((packed)) {
    uint64_t id;
    uint32_t codeOffset;  // from the entry of the function
    uint16_t flags;
    uint16_t numLocations;
} recordInfo;

typedef struct __attribute__((packed)) {
    uint8_t kind;       // possibilities come from location_kind_t, but is one byte in size.
    uint8_t flags;      // expected to be 0
    uint16_t locSize;
    uint16_t regNum;    // Dwarf register num
    uint16_t reserved;  // expected to be 0
    int32_t offset;     // either an offset or a "Small Constant"
} locationInfo;

typedef struct {
    uint64_t id;
    uint32_t codeOffset;  // from the entry of the function
    uint16_t flags;
    uint16_t numLocations;
    locationInfo* locations; // Array of locations
} callSiteInfo;

typedef struct {
    uint64_t address;
    uint64_t stackSize;
    uint64_t callsiteCount; 
    callSiteInfo* callSitesInfo; // Array of call sites per function
} function;

typedef struct {
    uint32_t numFunctions;
    uint32_t numRecords;    
    function* functions; // Array of functions
} stackMap;
