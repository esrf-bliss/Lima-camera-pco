//+--------------------------------------------------------------------------
//
//
//  Copyright (C) Silicon Software GmbH, 2017.
//
//  File: hap.h
//
//  Contents:
//
//
// Revision: 1997 erzeugt
// Okt. 99: Erweiterung um Registerinformationen
// Nov. 99: Erweiterung um Multihap-Funktionalität und Versionskontrolle
//---------------------------------------------------------------------------

#pragma once
#ifndef __HAP2H
#define __HAP2H

#include <stdint.h>

#include "hap_os.h"
#include "os_type.h"

// --------------------------------------------------------------------------

#define HAP_ERR_LOAD_RBT -100
#define HAP_ERR_MULTIRAM_ALLOC -101
#define HAP_ERR_ADD_TEXT -102
#define HAP_ERR_ADD_MODULE_RAW -103
#define HAP_ERR_ADD_VAS_DLL -104
#define HAP_ERR_ADD_LL_FILE -105
#define HAP_ERR_ADD_XML_FILE -106
#define HAP_ERR_ADD_CONFIG_TO_FILE -107
#define HAP_ERR_ADD_VASSRCLIB_DLL -108
#define HAP_ERR_BITFILE_TYPE_DONT_MATCH -109
#define HAP_ERR_ADD_TAG -110
#define HAP_ERR_ADD_DLL -111
#define HAP_ERR_LOAD_DESIGN -112
#define HAP_ERR_CHECKREGISTERLINKS -113
#define HAP_ERR_INCOMPATIBLE_LIBRARY -114 /** Library version mismatch*/
#define HAP_ERR_ADD_VASI_EVA_DLL -115
#define HAP_ERR_ADD_VA_RUNTIME_DLL -115

// --------------------------------------------------------------------------

class HAP;
class HAP_module;
class HAP_param;
class HAP_channel_param;

class hap_module_ext_private;
class hap_module_ext;

class fpga_design;

struct hap_param_link;
struct hap_tparam;
struct HapData;

struct hap_param {
    unsigned int tag;
    unsigned int len;
    int moduleIndex;
    int paramIndex;
    char name[64];
    uint64_t value;  // change
    double valueDouble;
    unsigned int flags;

    // -------------------  V1.1 ---------------------

    int type;  // uint64 = 0, double = 1
    int fieldSize;
    const uint64_t *valueField;
    const double *doubleField;
    const char *paramString;
    int paramStringLen;
    unsigned int reserved[17];  // reserved for later use

    // -------------------  V1.2 ---------------------
};

struct hap_channel_param {
    unsigned int tag;
    unsigned int len;
    unsigned int direction;
    int moduleIndex;
    int linkIndex;
    char name[64];
    unsigned int formatIndex;
    unsigned int arith;
    unsigned int flavor;
    unsigned int width;
    unsigned int pos;
    unsigned int parallel;
    unsigned int kernelRows;
    unsigned int kernelCols;
    unsigned int maxWidth;
    unsigned int maxHeight;
};

struct hap_event_data {
    unsigned char uiq;
    unsigned char bit;
    unsigned char datalen;
    const char *name;
    int regid;
    int is_firmware_event;
};

#if 0
// TODO: Check for removal

#define REGISTER 0x1
#define PULSE 0x2

// Such simple defines tend to create a conflict at some point or another
// ID conflicts with OS X framework headers, for example
// Commented out, check how these can eb replaced
enum HapInfoType {
    ID = 100,
    NUMBER = 200,
    NUMBERINT = 210,
    NUMBERDOUBLE = 220,
    STRING = 300
};

#endif

enum HapInfoType {
    HAP_INFO_TYPE_ID = 100,
    HAP_INFO_TYPE_NUMBER = 200,
    HAP_INFO_TYPE_NUMBERINT = 210,
    HAP_INFO_TYPE_NUMBERDOUBLE = 220,
    HAP_INFO_TYPE_STRING = 300
};

// --------------------------------------------------------------------------
// This is the public API needed for VA.
// Don't add functionality needed by the Runtime here, see hap_p.h!
// --------------------------------------------------------------------------

extern "C" fpga_design *HapLoadHapFileWithType(const char *DesignName, int Type, int expandDlls);
extern "C" int HapUnloadHapFile(fpga_design *Design);

extern "C" unsigned int HapGetRegisterAddress(fpga_design *design, int HRegister);
extern "C" int HapGetNrOfRegister(fpga_design *design);
extern "C" const char *const HapGetRegisterName(fpga_design *design, const int nr);
extern "C" int HapGetRegisterByName(fpga_design *design, const char *name);
extern "C" int HapGetRegisterBitRange(fpga_design *design, const int nr);
extern "C" int HapGetRegisterBitPos(fpga_design *design, const int nr);
extern "C" int HapGetRegisterDirection(fpga_design *design, const int nr);
extern "C" uint64_t HapGetRegisterInitValue(fpga_design *design, const int nr);

extern "C" int HapGetNrOfModules(fpga_design *design);
extern "C" const char *const HapGetModuleName(fpga_design *design, int index);
extern "C" const char *const HapGetModuleDecoratedName(fpga_design *mDesign, int mIndex);
extern "C" const char *const HapGetModuleFullName(fpga_design *design, int index);
extern "C" const char *const HapGetModuleTyp(fpga_design *design, int index);
extern "C" int HapGetModuleProcessId(fpga_design *design, int index);
extern "C" hap_module_ext *HapGetModuleByNumber(fpga_design *design, int index);
extern "C" const char *const HapGetModuleDllName(fpga_design *mDesign, int mIndex);
extern "C" const char *const HapGetModuleDllDir(fpga_design *mDesign);
extern "C" struct hap_param *HapModuleGetParam(hap_module_ext *mod, unsigned int index);
extern "C" int HapModuleGetParamCount(hap_module_ext *mod);
extern "C" const struct hap_channel_param *HapModuleGetChannel(hap_module_ext *mod, unsigned int index);
extern "C" int HapModuleGetChannelCount(hap_module_ext *mod);

extern "C" int HapGetNrOfParam(fpga_design *design, int mindex);
extern "C" int HapGetAbsolutParamIndex(fpga_design *design, int mindex, int localpindex);
extern "C" const char *const HapGetParamName(fpga_design *design, int mindex, int pindex);
extern "C" int HapGetRelativeParamIndex(fpga_design *design, int mindex, int absolutepindex);
extern "C" const char *const HapGetParamFullName(fpga_design *design, int mindex, int pindex);

extern "C" int HapGetNrOfDmas(fpga_design *design);
extern "C" int HapGetDmaModuleNr(fpga_design *mDesign, int dmaIndex);
extern "C" int HapGetDmaChannel(fpga_design *design, int dmaIndex);
extern "C" int HapGetDmaWidth(fpga_design *design, int dmaIndex);
extern "C" int HapGetDmaHeight(fpga_design *design, int dmaIndex);
extern "C" int HapGetDmaFormat(fpga_design *design, int dmaIndex);
extern "C" int HapGetDmaBitWidth(fpga_design *design, int dmaIndex);
extern "C" int HapGetProcessNrFromDmaChannel(fpga_design *design, int dmaChannel);

extern "C" const void *const HapGetUserInfoByName(fpga_design *design, const char *name);

extern "C" int HapGetNrOfProcess(fpga_design *design);
extern "C" int HapGetProcessClk(fpga_design *design, int index);

extern "C" int HapGetNrOfCams(fpga_design *design);
extern "C" int HapGetCamChannel(fpga_design *design, int dmaIndex);
extern "C" int HapGetNrOfCamsPerProcess(fpga_design *mDesign, int processId);
extern "C" int HapGetCamChannelPerProcess(fpga_design *mDesign, int camIndex, int processId);

extern "C" const char *const HapGetDesignVersion(fpga_design *design);



#endif
