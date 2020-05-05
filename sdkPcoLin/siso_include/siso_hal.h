// +---------------------------------------------------------------------------
//
//
//  Copyright (C) Silicon Software GmbH, 2005-2010
//
//  File:       siso_hal.h
//
// ----------------------------------------------------------------------------+

#pragma once

#include "fgrab_define.h"
#include "os_type.h"

#include <stdint.h>
#include <sys/types.h>

class dma_mem;
class SisoHW;
class fpga_design;

struct fg_dma_control;
struct me_buf_status;
struct VAdevice;

#include <map>

enum JTAGStateEnum {
    JTAG_STATE_INVALID = -1,
    JTAG_STATE_RESET = 0,
    JTAG_STATE_IDLE = 1,
    JTAG_STATE_SHIFT_DR = 2,
    JTAG_STATE_SHIFT_IR = 3,
    JTAG_STATE_EXIT = 4,
};

enum UiqTransmissionModeEnum {
    UIQ_TRANSMISSION_SINGLE_BYTE_WORD = 1,
    UIQ_TRANSMISSION_DOUBLE_BYTE_WORD = 2,
    UIQ_TRANSMISSION_QUAD_BYTE_WORD = 4
};

// Me5 Flashing flags
static const int SHAL_SKIP_BOARD_TYPE_CHECK_FLAG = 0x1;  // Skip board type check during flashing

// Reconfiguration flags
static const int SHAL_FLAG_RECONFIGURATION_FORCE = 0x1;  // Force reconfiguration (neglect Tweak Properties)

#ifndef ASCII_TO_INT
#define ASCII_TO_INT
#define ASCII_STR_TO_INT(x) (static_cast<unsigned int>((x[0] << 24) | (x[1] << 16) | (x[2] << 8) | (x[3])))
#define ASCII_CHAR4_TO_INT(d, c, b, a) (static_cast<unsigned int>((d << 24) | (c << 16) | (b << 8) | (a)))
#define ASCII_CHAR2_TO_INT(b, a) (static_cast<unsigned int>((b << 8) | (a)))
#endif

class HWListBase;

typedef enum siso_hwlist_sort_key { UNDEFINED_SORT_KEY, PCI_ADDRESS_SORT_KEY, SERIAL_NUMBER_SORT_KEY } siso_hwlist_sort_key;

typedef std::map<uint32_t, unsigned int> siso_hwlist_sort_map;

typedef std::map<unsigned int, HWListBase *> siso_hwlist_map;

#ifdef SISO_HAL_STATIC
#define SISO_HAL_API
#else
#ifdef siso_hal_EXPORTS
#define SISO_HAL_API __declspec(dllexport)
#else                 /* siso_hal_EXPORTS */
#define SISO_HAL_API  //__declspec(dllimport)
#endif                /* siso_hal_EXPORTS */
#endif

// If you need to change the layout of this line then make sure
// the RegEx in testcases/CMakeLists.txt that looks after this
// is changed accordingly.
#define SHAL_MAX_DEVICE_INDEX 31

extern "C" void Log(const int level, const char *format, ...);

extern "C" {

enum IndexRange { IndexGlobal = 0, IndexLibrary = 1 };

SISO_HAL_API int SHalInitLibraries();
SISO_HAL_API int SHalFreeLibraries();

SISO_HAL_API int SHalGetDeviceCount();
SISO_HAL_API int SHalGetMaxDevices();
SISO_HAL_API unsigned int SHalGetDeviceSpecificIndex(const unsigned int globalIndex);

SISO_HAL_API struct VAdevice *SHalInitDevice(const unsigned int GlobalNr);
SISO_HAL_API int SHalFreeDevice(struct VAdevice *board);

// SHalGetDevice returns a HAL-internal handle for the device
// This handle should be only used to get information about the device
// and never should be used in a structure returned to the user or
// retained for a longer time
SISO_HAL_API struct VAdevice *SHalGetDevice(const unsigned int GlobalNr);
SISO_HAL_API int SHalPutDevice(struct VAdevice *board);

SISO_HAL_API unsigned int SHalGetBoardIndex(const struct VAdevice *board, const enum IndexRange range);

SISO_HAL_API int SHalConfigureFpga(struct VAdevice *board, fpga_design *DesignHandle);
SISO_HAL_API int SHalConfigureSlave(struct VAdevice *board, fpga_design *DesignHandle);
SISO_HAL_API int SHalUnloadFpga(struct VAdevice *board);
SISO_HAL_API int SHalReleaseFpga(struct VAdevice *board);

SISO_HAL_API int SHalGetInterfaceClk(struct VAdevice *board, fpga_design *DesignHandle);
SISO_HAL_API int SHalGetFpgaClk(struct VAdevice *board, fpga_design *DesignHandle);
SISO_HAL_API int SHalSetFpgaClk(struct VAdevice *board, fpga_design *DesignHandle, int ClkFreq);
SISO_HAL_API int SHalSetInterfaceClk(struct VAdevice *board, fpga_design *DesignHandle, int ClkFreq);

SISO_HAL_API int SHalRegisterWrite(struct VAdevice *boardHandle, int HRegister, uint64_t value);
SISO_HAL_API int SHalRegisterRead(struct VAdevice *boardHandle, int HRegister, uint64_t *value);

SISO_HAL_API dma_mem *SHalGetVirtualUserBuffer(struct VAdevice *boardHandle, const size_t Size, const frameindex_t NrOfSubbuffer);
SISO_HAL_API int SHalFreeVirtualUserBuffer(struct VAdevice *boardHandle, dma_mem *MemHandle);
SISO_HAL_API int SHalAddUserBuffer(struct VAdevice *boardHandle, dma_mem *MemHandle, void *Buffer, size_t Size,
                                   const frameindex_t NrOfSubbuffer);
SISO_HAL_API int SHalDelUserBuffer(struct VAdevice *boardHandle, dma_mem *MemHandle, const frameindex_t BufferHandle);

SISO_HAL_API int SHalUnlockBuffer(struct VAdevice *boardHandle, dma_mem *MemHandle, frameindex_t BufferNr);
SISO_HAL_API int SHalGetBufferStatus(struct VAdevice *boardHandle, dma_mem *MemHandle, const int dummy, const frameindex_t data,
                                     struct me_buf_status *outdata);

SISO_HAL_API int SHalDmaStart(struct VAdevice *board, const unsigned int chan, dma_mem *mem, const struct fg_dma_control *flags);
SISO_HAL_API int SHalDmaStop(struct VAdevice *board, int DmaChan, int nMode,
                             int nTimeout);  // NOTE: mode and timeout are ignored in lower layers; since applets call this with
                                             // arbitrary parameters, we cannot just implement this as it was supposed to be, but we
                                             // have to use additional api
SISO_HAL_API frameindex_t
SHalDmaWaitForBuffer(struct VAdevice *board, int DmaChan, frameindex_t subbufnum, dma_mem *MemHandle, int timeout);
SISO_HAL_API frameindex_t
SHalDmaWaitForBufferExtended(struct VAdevice *board, frameindex_t picNr, int nFlag, dma_mem *MemHandle, int timeout);

SISO_HAL_API frameindex_t SHalDmaTestBuffer(struct VAdevice *board, const unsigned int port, dma_mem *MemHandle);
SISO_HAL_API uint32_t SHalGetDmaTag(struct VAdevice *board, dma_mem *pMem, const frameindex_t nSubbufNr);
SISO_HAL_API unsigned int SHalGetDmaTimeStamp(struct VAdevice *board, dma_mem *pMem, const frameindex_t nSubbufNr);
SISO_HAL_API uint64_t SHalGetDmaTimeStampLong(struct VAdevice *board, dma_mem *pMem, const frameindex_t nSubbufNr);
SISO_HAL_API int SHalGetDmaTimeStampLongFrequency(struct VAdevice *board, uint64_t * freq);
SISO_HAL_API size_t SHalGetDmaLength(struct VAdevice *board, dma_mem *pMem, const frameindex_t nSubbufNr);

SISO_HAL_API int SHalDataQueueReadBuffer(struct VAdevice *board, int nBufferNr, unsigned char *cpBuffer, int bufsize,
                                         int timeoutInMs);
SISO_HAL_API int SHalDataQueueReadBufferLong(struct VAdevice *board, int nBufferNr, uint32_t *cpBuffer, size_t bufsize,
                                             int timeoutInMs);
SISO_HAL_API int SHalDataQueueWriteBuffer(struct VAdevice *board, int nBufferNr, const unsigned char *cpBuffer, int bufsize,
    int timeoutInMs);
SISO_HAL_API int SHalDataQueueWriteBufferLong(struct VAdevice *board, int nBufferNr, const uint32_t *cpBuffer, int bufsize,
    int timeoutInMs);
SISO_HAL_API int SHalDataQueueGetNrOfWords(struct VAdevice *board, int nBufferNr);
SISO_HAL_API int SHalDataQueueGetNrOfLostWords(struct VAdevice *board, int nBufferNr);
SISO_HAL_API unsigned int SHalDataQueueGetStatus(struct VAdevice *board, int nBufferNr);
SISO_HAL_API int SHalDataQueueGetNrOfTransfers(struct VAdevice *board, int nBufferNr);
SISO_HAL_API int SHalDataQueueClear(struct VAdevice *board, int nBufferNr);
SISO_HAL_API int SHalDataQueueFlush(struct VAdevice *board, const unsigned int queue);
SISO_HAL_API int SHalDataQueuePoll(struct VAdevice *board, uint64_t *queues, const unsigned int timeout);

SISO_HAL_API void *SHalGetBufferByNumber(struct VAdevice *board, dma_mem *dm, const frameindex_t pic_nr);
SISO_HAL_API size_t SHalGetBufferSize(struct VAdevice *board, dma_mem *dm);
SISO_HAL_API frameindex_t SHalGetNrOfSubbuffer(struct VAdevice *board, dma_mem *dm);

SISO_HAL_API int SHalSetLicenseString(struct VAdevice *board, const char *license);

SISO_HAL_API int SHalGetBoardInfoInt(struct VAdevice *board, const char *featureName);
SISO_HAL_API const char *const SHalGetBoardInfoString(struct VAdevice *board, const char *featureName);
SISO_HAL_API int SHalSetBoardParamInt(struct VAdevice *board, const char *featureName, int val);

SISO_HAL_API int SHalGetCameraStatusExtended(struct VAdevice *board, int camPort);

// HAL-only functions (not in siso_hw)

SISO_HAL_API int SHalClSerInit(struct VAdevice *board, const unsigned int port, const unsigned int baud, unsigned int *readqueue,
                               unsigned int *writequeue);
SISO_HAL_API int SHalClSerSetParity(struct VAdevice *board, const unsigned int port, const int parity);
SISO_HAL_API int SHalClSerPortChange(struct VAdevice *board, const unsigned int port);
SISO_HAL_API int SHalClSerSetBaudrate(struct VAdevice *board, const unsigned int port, const unsigned int baud);
SISO_HAL_API int SHalClSerSetConfig(struct VAdevice *board, const unsigned int port, const unsigned long config);
SISO_HAL_API int SHalClSerSetFlowControlMode(struct VAdevice *board, const unsigned int port, const unsigned int portsec,
                                             const int mode);
SISO_HAL_API int SHalClSerSetFIFODepth(struct VAdevice *board, const unsigned int port, const unsigned long depth);

// board specific functions

SISO_HAL_API void *SHalMe3GetRombase(struct VAdevice *board);
SISO_HAL_API int SHalInvalidatePartition(struct VAdevice *board, const unsigned int partition);
SISO_HAL_API int SHalUpdateFirmware(struct VAdevice *board, const void *mem, const size_t len, const char *filePath, const size_t filePathLength, const unsigned int partition, const int flags);
SISO_HAL_API int SHalUpdateCpld(struct VAdevice *board, const void *buf, const unsigned int len, const int tag, const int flags);
SISO_HAL_API void SHalRegisterProgressCallback(struct VAdevice *board, void (*callback)(struct SISO_PROGRESS_CONTEXT *),
                                               void *user);
SISO_HAL_API void SHalMe4RegisterWriteSPICallback(struct VAdevice *board, void (*callback)(struct SISO_SPI_CONTEXT *), void *user);
SISO_HAL_API int SHalMe4WriteSPI(struct VAdevice *board, const void *buf, const size_t len, const unsigned int spi);
SISO_HAL_API int SHalMe4WriteSPIEx(struct VAdevice *board, const void *buf, const size_t len, const unsigned int spi,
                                   const int boardTypeCheck);
SISO_HAL_API int SHalMe4ReadSPI(struct VAdevice *board, unsigned char *buf, size_t *len, const unsigned int spi,
                                const unsigned int whatToReadflag);

SISO_HAL_API void *SHalMe4GetRuntimeBase(struct VAdevice *board);
SISO_HAL_API void *SHalMe4MapFpgaBase(struct VAdevice *board);
SISO_HAL_API void SHalMe4UnmapFpgaBase(struct VAdevice *board, void *fpga_base);

// Extensions
SISO_HAL_API int SHalDataQueueAcquireLock(struct VAdevice *board, const unsigned int queue);
SISO_HAL_API int SHalDataQueueReleaseLock(struct VAdevice *board, const unsigned int queue);

SISO_HAL_API int SHalGetPartitionList(struct VAdevice *board, void * partitionList, void * statusList, unsigned int * configCount, uint32_t permissions, uint32_t flags);
SISO_HAL_API int SHalGetPeripheralList(struct VAdevice *board, void * peripheralList, void * statusList, unsigned int * deviceCount, uint32_t flags);
SISO_HAL_API int SHalReadConfig(struct VAdevice *board, void *buffer, size_t size, unsigned long tag, unsigned long address);
SISO_HAL_API int SHalWriteConfig(struct VAdevice *board, void *buffer, size_t size, unsigned long tag, unsigned long address);
SISO_HAL_API int SHalEraseConfig(struct VAdevice *board, size_t size, unsigned long tag, unsigned long address);

SISO_HAL_API int SHalGetLEDs(struct VAdevice *board, unsigned short *status, unsigned short *present);
SISO_HAL_API int SHalSetLEDs(struct VAdevice *board, unsigned short status);

SISO_HAL_API int SHalDmaStopMode(struct VAdevice *board, const unsigned int chan, const int bSync, const int nTimeout);

SISO_HAL_API int SHalGetDmaParam(struct VAdevice *board, const unsigned int chan, const unsigned int param,
                                 unsigned long long *const value);
SISO_HAL_API int SHalSetDmaParam(struct VAdevice *board, const unsigned int chan, const unsigned int param,
                                 const unsigned long long value);

SISO_HAL_API int SHalAuxPortEnumeratePorts(struct VAdevice *board, const unsigned int port_num);
SISO_HAL_API int SHalAuxPortAcquireLock(struct VAdevice *board, const unsigned int port_num);
SISO_HAL_API int SHalAuxPortReleaseLock(struct VAdevice *board, const unsigned int port_num);
SISO_HAL_API int SHalAuxPortStartTransferBlock(struct VAdevice *board, const unsigned int port_num, const unsigned int timeout);
SISO_HAL_API int SHalAuxPortStopTransferBlock(struct VAdevice *board, const unsigned int port_num);
SISO_HAL_API int SHalAuxPortGetFrequency(struct VAdevice *board, const unsigned int port_num, unsigned int *freq);
SISO_HAL_API int SHalAuxPortSetFrequency(struct VAdevice *board, const unsigned int port_num, const unsigned int freq);
SISO_HAL_API int SHalAuxPortWriteData(struct VAdevice *board, const unsigned int port_num, const void *data, unsigned int length,
                                      unsigned int flags, unsigned int timeout);
SISO_HAL_API int SHalAuxPortReadData(struct VAdevice *board, const unsigned int port_num, void *data, unsigned int length,
                                     unsigned int flags, unsigned int timeout);
SISO_HAL_API int SHalAuxPortFlushData(struct VAdevice *board, const unsigned int port_num, const int flush_type);
SISO_HAL_API int SHalJTAGPortSetState(struct VAdevice *board, const unsigned int port_num, const JTAGStateEnum state);
SISO_HAL_API int SHalUiqPortSetTransmissionMode(struct VAdevice *board, const unsigned int port_num, const UiqTransmissionModeEnum transmissionMode);
SISO_HAL_API int SHalUiqPortGetTransmissionMode(struct VAdevice *board, const unsigned int port_num, UiqTransmissionModeEnum *transmissionMode);
SISO_HAL_API int SHalUiqPortSetSpiMode(struct VAdevice *board, const unsigned int port_num, const unsigned int spiMode);
SISO_HAL_API int SHalUiqPortGetSpiMode(struct VAdevice *board, const unsigned int port_num, unsigned int * spiMode);
SISO_HAL_API int SHalUiqPortGetNrOfWords(struct VAdevice *board, const unsigned int port_num, unsigned int * nWords);

SISO_HAL_API int SHalRegisterAsyncNotifyCallback(struct VAdevice *board, Fg_AsyncNotifyFunc_t func, void *context);
SISO_HAL_API int SHalUnregisterAsyncNotifyCallback(struct VAdevice *board, Fg_AsyncNotifyFunc_t func, void *context);
SISO_HAL_API int SHalGetAsyncNotify(struct VAdevice *board, unsigned long *evt, unsigned long *pl, unsigned long *ph);
SISO_HAL_API int SHalResetAsyncNotify(struct VAdevice *board, unsigned long evt, unsigned long pl, unsigned long ph);
SISO_HAL_API int SHalGetDeviceStatus(struct VAdevice *board, unsigned long *status);

SISO_HAL_API int SHalReconfigureFpga(struct VAdevice *board, int partition, int flags);
SISO_HAL_API int SHalGetHapConfigData(struct VAdevice *board, fpga_design * fwdes, const void **confData, size_t *conflen);

}  // extern "C"
