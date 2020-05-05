/**
 * Copyright 2008,2009 Silicon Software GmbH
 */
#pragma once

#include <stdlib.h> /* for NULL */
#include <string>   /* for std::string */
#include <sstream>  /* for std::ostringstream */

#ifdef __GNUC__
#define HAL_UNUSED __attribute__ ((unused))
#else
#define HAL_UNUSED
#endif

template <typename T> static std::string to_string(const T& val)
{
    std::ostringstream stream;
    stream << val;
    return stream.str();
}

/*
 * This defines all currently supported feature strings. They may be used with
 * GetBoardInfoInt().
 */

/* Returns the board type (e.g. 0xa44 for FullX4) */
#define BOARD_FEATURE_INT_BOARDTYPE "board/type"

/* Returns the board index (i.e. as used by fglib) */
#define BOARD_FEATURE_INT_BOARDINDEX    "board/index"

/* Returns the board driver index */
#define BOARD_FEATURE_INT_BOARDDRIVERINDEX  "board/driverindex"

/* Returns the board physical index */
#define BOARD_FEATURE_INT_BOARDPHYSINDEX    "board/physindex"

/* Returns the board hardware version */
#define BOARD_FEATURE_INT_BOARDVERSION  "board/version"

/* Returns the board hardware revision */
#define BOARD_FEATURE_INT_BOARDREVISION "board/revision"

/* Returns the board serial number */
#define BOARD_FEATURE_INT_BOARDSERIAL   "board/serial"

/* Returns the board error counter */
#define BOARD_FEATURE_INT_BOARDERRORS "board/errors"

/* Returns the board status */
#define BOARD_FEATURE_INT_BOARDSTATUS "board/status"
#define BOARD_STATUS_CONFIGURED         0x00001
#define BOARD_STATUS_LOCKED             0x00002
#define BOARD_STATUS_DRIVER_MISMATCH    0x10000
#define BOARD_STATUS_DEAD               0x08000

/* Returns number of partitions available for the board */
#define BOARD_FEATURE_INT_PARTITIONCOUNT            "board/partitions/count"

/* Returns the index of the current "cold" booted partition */
#define BOARD_FEATURE_INT_CURRENT_PARTITION_INDEX   "board/partitions/current"
/* Returns the index of the cold boot partition */
#define BOARD_FEATURE_INT_COLDBOOT_PARTITION_INDEX  "board/partitions/coldboot"
/* Returns the index of the warm boot partition */
#define BOARD_FEATURE_INT_WARMBOOT_PARTITION_INDEX  "board/partitions/warmboot"
/* Returns 0 if the board does not support safe configuration, 1 otherwise */
#define BOARD_FEATURE_INT_SAFE_PARTITON_AVAILABLE   "board/partitions/hassafeconfig"
/* Returns the index of the safe configuration partition */
#define BOARD_FEATURE_INT_SAFE_PARTITION_INDEX      "board/partitions/safeconfig"
/* Returns 0 if the board does not support reconfiguration, 1 otherwise */
#define BOARD_FEATURE_INT_SUPPORTS_RECONFIGURATION  "board/reconfigurable"

/* Returns MDIO version (-1 if not available) */
#define BOARD_FEATURE_INT_MDIO_TYPE            "board/peripherals/mdio/type"
/* Returns SPI Interface version (-1 if not available) */
#define BOARD_FEATURE_INT_SPI_IF_VERSION       "board/peripherals/spi/interface/version"

/* Returns the board flash count, protection count and status */
#define BOARD_FEATURE_INT_FLASHCOUNT           "board/flashcount"
#define BOARD_FEATURE_INT_PROTECTIONSTATUS     "board/protectionstatus"
#define BOARD_FEATURE_INT_PROTECTIONSTATUSINIT "board/protectionstatusinit"

/* Returns the design version number of the flashed design in the specified partition */
#define _BOARD_FEATURE_INT_DESIGN_VERSION     "board/partitions/version/"
#define BOARD_FEATURE_INT_DESIGN_VERSION(N)  (_BOARD_FEATURE_INT_DESIGN_VERSION + to_string((unsigned long long)N)).c_str()
/* Returns the design revision number of the flashed design in the specified partition */
#define _BOARD_FEATURE_INT_DESIGN_REVISION     "board/partitions/revision/"
#define BOARD_FEATURE_INT_DESIGN_REVISION(N)  (_BOARD_FEATURE_INT_DESIGN_REVISION + to_string((unsigned long long)N)).c_str()

/* CL Specific Features */
#define BOARD_FEATURE_INT_ALTERNATIVE_BOARD_DETECTION_FEATURE_ENABLED   "board/marathon/abdfeatureenabled"
#define BOARD_FEATURE_INT_POCL_DETECTION_ENABLED                        "board/marathon/cl/pocldetectionenabled"
#define BOARD_FEATURE_INT_POCL_DETECTION_OVERRIDE                       "board/marathon/cl/pocldetectionoverride"

#define SPI_PROTECTION_STANDARD         0x001C /* SPI on-chip protection feature */
#define SPI_PROTECTION_EXTENDED         0x007C /* SPI on-chip protection feature extension for larger chips (mE5) */

/* Board has expansion board installed (e.g. PixelPlant) */
#define BOARD_FEATURE_INT_EXPBOARD  "expb"

/* Returns the type of the first expansion board (e.g. 0xc42 for PX200) */
#define BOARD_FEATURE_INT_EXP0_TYPE "exp0/type"

/* Returns the serial number of the first expansion board */
#define BOARD_FEATURE_INT_EXP0_SERIAL   "exp0/serial"

/* Board supports Power over CameraLink (PoCL) */
#define BOARD_FEATURE_INT_POCL      "pocl"

/* Number of CameraLink interfaces */
#define BOARD_FEATURE_INT_CL        "cl"
#define BOARD_FEATURE_INT_PORTS_CL  "ports/cl"

/* Number of serial interfaces */
#define BOARD_FEATURE_INT_PORTS_SER "ports/serial"

/* Number of supported streams */
#define BOARD_FEATURE_INT_STREAMS   "streams"

/* Number of GigE ports */
#define BOARD_FEATURE_INT_PORTS_GIGE    "ports/gige"

/* Number of CXP ports */
#define BOARD_FEATURE_INT_PORTS_CXP "ports/cxp"

/* Number of CLHS ports */
#define BOARD_FEATURE_INT_PORTS_CLHS    "ports/clhs"

/* Firmware can be upgraded */
#define BOARD_FEATURE_INT_FIRMWARE_UPGRADEABLE  "firmware/upgradeable"

/* Firmware version major */
#define BOARD_FEATURE_INT_FIRMWARE_MAJOR    "firmware/major"

/* Firmware version major */
#define BOARD_FEATURE_INT_FIRMWARE_MINOR    "firmware/minor"

/* License group code */
#define BOARD_FEATURE_INT_LICENSE_GROUP "license/group"

/* License group code */
#define BOARD_FEATURE_INT_LICENSE_USER  "license/user"

/* PCI Express Link Speed (1, 2, 3) */
#define BOARD_FEATURE_INT_PCIE_LINK_SPEED "board/pcieSpeed"

/* PCI Express Link Width (1, 4, 8, 12, 16, 32) */
#define BOARD_FEATURE_INT_PCIE_LINK_WIDTH "board/pcieWidth"

/* PCI Express Link Payload Mode (0: 128, 1: 256) */
#define BOARD_FEATURE_INT_PCIE_PAYLOAD_MODE "board/pciePlMode"

/* PCI Express Link Payload Mode (0: 128, 1: 256) */
#define BOARD_FEATURE_INT_PCIE_REQUEST_MODE "board/pcieRqMode"

/* PCI Express Expected Link Speed (1, 2, 3) */
#define BOARD_FEATURE_INT_PCIE_EXPECTED_LINK_SPEED "board/pcieExpSpeed"

/* PCI Express Expected Link Width (1, 4, 8, 12, 16, 32) */
#define BOARD_FEATURE_INT_PCIE_EXPECTED_LINK_WIDTH "board/pcieExpWidth"

/* PCI Express Native Link Speed (Slot; 1, 2, 3) */
#define BOARD_FEATURE_INT_PCIE_NATIVE_LINK_SPEED "board/pcieNatSpeed"

/* PCI Express 8b/10b Invalid Code Errors */
#define BOARD_FEATURE_INT_PCIE_8B10B_INVALID_CODE_ERRORS "board/pcie8b10bInvCd"

/* PCI Express 8b/10b Disparity Errors */
#define BOARD_FEATURE_INT_PCIE_8B10B_DISPARITY_ERRORS "board/pcie8b10bDisp"

/* PCI Express Receiver Detection (0: default, 1: alternative) */
#define BOARD_FEATURE_INT_PCIE_RECEIVER_DETECTION "board/pcieRecvDet"

/* SiSo FPGA type of FPGA 0 */
#define BOARD_FEATURE_INT_FPGA0     "board/fpga0"

/* SiSo FPGA type of FPGA 1 */
#define BOARD_FEATURE_INT_FPGA1     "board/fpga1"

/* Board specific information */
#define BOARD_FEATURE_INT_ME3_HWSTATUS  "#me3/hwstatus"

/* NUMA support and interrupt affinity */
#define BOARD_FEATURE_INT_BOARD_NODE_NUMBER     "board/nodenumber"
#define BOARD_FEATURE_INT_DRIVER_GROUP_AFFINITY "driver/groupaffinity"
#define BOARD_FEATURE_INT_DRIVER_AFFINITY_HIGH  "driver/affinityhigh"
#define BOARD_FEATURE_INT_DRIVER_AFFINITY_LOW   "driver/affinitylow"
#define BOARD_FEATURE_INT_PCIE_DSN_HIGH         "pcie/dsnhigh"
#define BOARD_FEATURE_INT_PCIE_DSN_LOW          "pcie/dsnlow"
#define BOARD_FEATURE_INT_PCI_VENDOR_ID         "pci/vendor"
#define BOARD_FEATURE_INT_PCI_DEVICE_ID         "pci/device"
#define BOARD_FEATURE_INT_PCI_SUBSYS_VENDOR_ID  "pci/subsys/vendor"
#define BOARD_FEATURE_INT_PCI_SUBSYS_DEVICE_ID  "pci/subsys/device"

/* A list of all feature strings defined above. To get a list of all supported
 * features simply iterate over this array. */
static const char *me4_feature_strings[] HAL_UNUSED = {
    BOARD_FEATURE_INT_BOARDTYPE,
    BOARD_FEATURE_INT_EXPBOARD,
    BOARD_FEATURE_INT_POCL,
    BOARD_FEATURE_INT_CL,
    BOARD_FEATURE_INT_FIRMWARE_MAJOR,
    BOARD_FEATURE_INT_FIRMWARE_MINOR,
    BOARD_FEATURE_INT_LICENSE_GROUP,
    BOARD_FEATURE_INT_LICENSE_USER,
    NULL
};

/*
 * This defines all currently supported board feature strings that will return a
 * string.
 */

/* Returns the board description (e.g. "microEnable IV-FULL x1") */
#define BOARD_FEATURE_STR_BOARDNAME "board/name"

/* Returns the FPGA name with index 0 (e.g. "3s1600efg484") */
#define BOARD_FEATURE_STR_FPGA0     BOARD_FEATURE_INT_FPGA0

/* Returns the FPGA name with index 1 (e.g. "3s1600efg484") */
#define BOARD_FEATURE_STR_FPGA1     BOARD_FEATURE_INT_FPGA1

/* Returns the license string written to the board */
#define BOARD_FEATURE_STR_BOARDLICENSE  "board/license"

/* Returns the board (PCB) layout string */
#define BOARD_FEATURE_STR_BOARDLAYOUT   "board/layout"

/* Returns the board hardware serial number */
#define BOARD_FEATURE_STR_BOARDHWSERIAL "board/hwserial"

/* Returns the description of the first expansion board (e.g. "PixelPlant PX200") */
#define BOARD_FEATURE_STR_EXP0_NAME     "exp0/name"

/* Returns the hardware serial number of the first expansion board */
#define BOARD_FEATURE_STR_EXP0_HWSERIAL "exp0/hwserial"

/* Returns the board (PCB) layout string of the first expansion board */
#define BOARD_FEATURE_STR_EXP0_LAYOUT   "exp0/layout"

/* Returns the name of the currently loaded design */
#define BOARD_FEATURE_STR_DESIGN_NAME   "design/name"

/* Physical camera interface (e.g. "CameraLink", "GigE") */
#define BOARD_FEATURE_STR_CAMINTERFACE  "board/caminterface"

/* Returns the name of the design flashed in the specified partition */
#define _BOARD_FEATURE_STR_DESIGN_NAME_PARTITION     "board/partitions/dname/"
#define BOARD_FEATURE_STR_DESIGN_NAME_PARTITION(N)  (_BOARD_FEATURE_STR_DESIGN_NAME_PARTITION + to_string((unsigned long long)N)).c_str()
/* Returns the fpga uuid of the design flashed in the specified partition */
#define _BOARD_FEATURE_STR_DESIGN_UUID_PARTITION     "board/partitions/uuid/"
#define BOARD_FEATURE_STR_DESIGN_UUID_PARTITION(N)  (_BOARD_FEATURE_STR_DESIGN_UUID_PARTITION + to_string((unsigned long long)N)).c_str()
/* Returns the applet uuid of the design flashed in the specified partition */
#define _BOARD_FEATURE_STR_DESIGN_APPLET_UUID_PARTITION     "board/partitions/appuuid/"
#define BOARD_FEATURE_STR_DESIGN_APPLET_UUID_PARTITION(N)  (_BOARD_FEATURE_STR_DESIGN_APPLET_UUID_PARTITION + to_string((unsigned long long)N)).c_str()

#define BOARD_CAMINTERFACE_CAMERALINK   "CameraLink"
#define BOARD_CAMINTERFACE_CL           BOARD_CAMINTERFACE_CAMERALINK
#define BOARD_CAMINTERFACE_CAMERALINKHS "CameraLinkHS"
#define BOARD_CAMINTERFACE_CLHS         BOARD_CAMINTERFACE_CAMERALINKHS
#define BOARD_CAMINTERFACE_GIGE         "GigE"
#define BOARD_CAMINTERFACE_CXP          "CXP"

static const char *me4_caminterface_strings[] HAL_UNUSED = {
    BOARD_CAMINTERFACE_CAMERALINK,
    BOARD_CAMINTERFACE_CAMERALINKHS,
    BOARD_CAMINTERFACE_GIGE,
    BOARD_CAMINTERFACE_CXP
};

/* version string of the driver */
#define BOARD_FEATURE_STR_DRIVER_VERSION        "driver/version"
#define BOARD_FEATURE_STR_DRIVER_ARCHITECTURE   "driver/architecture"
#define BOARD_FEATURE_STR_DRIVER_FULL_VERSION   "driver/fullversion"

/* driver id and driver device number of the board */
#define BOARD_FEATURE_STR_DRIVER_ID             "driver/id"

#define BOARD_FEATURE_STR_APPLET_UUID           "applet/uuid"
#define BOARD_FEATURE_INT_APPLET_TIMESTAMP      "applet/timestamp"
#define BOARD_FEATURE_INT_APPLET_NAME_HASH      "applet/hash"

#define BOARD_FEATURE_INT_PCI_TRAINING_STATUS "firmaware/PCITrainingStatus"

static const char *me4_feature_strings_str[] HAL_UNUSED = {
    BOARD_FEATURE_STR_BOARDNAME,
    BOARD_FEATURE_STR_FPGA0,
    BOARD_FEATURE_STR_FPGA1,
    BOARD_FEATURE_STR_BOARDLICENSE,
    BOARD_FEATURE_STR_EXP0_NAME,
    BOARD_FEATURE_STR_CAMINTERFACE
};
