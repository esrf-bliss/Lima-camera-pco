############################################################################
# This file is part of LImA, a Library for Image Acquisition
#
# Copyright (C) : 2009-2011
# European Synchrotron Radiation Facility
# BP 220, Grenoble 38043
# FRANCE
#
# This is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <http://www.gnu.org/licenses/>.
############################################################################

import tango

from Lima import Core
from Lima import Pco as PcoAcq
from Lima.Server import AttrHelper

RESET_CLOSE_INTERFACE = 100


class Pco(tango.LatestDeviceImpl):

    Core.DEB_CLASS(Core.DebModApplication, "LimaCCDs")

    # ------------------------------------------------------------------
    #    Device constructor
    # ------------------------------------------------------------------
    def __init__(self, *args):
        tango.LatestDeviceImpl.__init__(self, *args)

        self.__Attribute2FunctionBase = {
            "acqTimeoutRetry": "AcqTimeoutRetry",
            "adc": "Adc",
            "adcMax": "AdcMax",
            "binInfo": "BinningInfo",
            "bitAlignment": "BitAlignment",
            "bytesPerPixel": "BytesPerPixel",
            "camerasFound": "CamerasFound",
            "camInfo": "CamInfo",
            "camName": "CameraName",
            "camNameBase": "CameraNameBase",
            "camNameEx": "CameraNameEx",
            "camType": "CamType",
            "cdiMode": "CDIMode",
            "clXferPar": "ClTransferParam",
            "cocRunTime": "CocRunTime",
            "coolingTemperature": "CoolingTemperature",
            "debugInt": "DebugInt",
            "debugIntTypes": "DebugIntTypes",
            "doubleImageMode": "DoubleImageMode",
            "firmwareInfo": "FirmwareInfo",
            "frameRate": "FrameRate",
            "generalCAPS1": "GeneralCAPS1",
            "info": "CamInfo",
            "lastError": "LastError",
            "lastImgAcquired": "LastImgAcquired",
            "lastImgRecorded": "LastImgRecorded",
            "logMsg": "MsgLog",
            "logPcoEnabled": "PcoLogsEnabled",
            "maxNbImages": "MaxNbImages",
            "paramsInfo": "ParamsInfo",
            "pixelRate": "PixelRate",
            "pixelRateInfo": "PixelRateInfo",
            "pixelRateValidValues": "PixelRateValidValues",
            "recorderForcedFifo": "RecorderForcedFifo",
            "roiInfo": "RoiInfo",
            "roiLastFixed": "LastFixedRoi",
            "rollingShutter": "RollingShutter",
            "rollingShutterInfo": "RollingShutterInfo",
            "rollingShutterStr": "RollingShutterStr",
            "temperatureInfo": "TemperatureInfo",
            "test": "Test",
            "timestampMode": "TimestampMode",
            "traceAcq": "TraceAcq",
            "version": "Version",
            "versionSdk": "SdkRelease",
        }

        self.init_device()

    # ------------------------------------------------------------------
    #    Device destructor
    # ------------------------------------------------------------------
    def delete_device(self):
        pass

    # ------------------------------------------------------------------
    #    Device initialization
    # ------------------------------------------------------------------
    @Core.DEB_MEMBER_FUNCT
    def init_device(self):
        self.set_state(tango.DevState.ON)
        self.get_device_properties(self.get_device_class())

    # ==================================================================
    #
    #    Pco read/write attribute methods
    #
    # ==================================================================
    def __getattr__(self, name):
        return AttrHelper.get_attr_4u(self, name, _PcoCam)

    # ==================================================================
    #
    #    Pco command methods
    #
    # ==================================================================
    @Core.DEB_MEMBER_FUNCT
    def getAttrStringValueList(self, attr_name):
        return AttrHelper.get_attr_string_value_list(self, attr_name)


# ==================================================================
#
#    PcoClass class definition
#
# ==================================================================
class PcoClass(tango.DeviceClass):

    #    Class Properties
    class_property_list = {}

    #    Device Properties

    device_property_list = {
        "params": [tango.DevString, "general parameters", []],
        "dummy": [tango.DevString, "dummy string", []],
        "debug_control": [tango.DevString, "general debug", []],
        "debug_module": [tango.DevString, "debug module flags", []],
        "debug_format": [tango.DevString, "debug format flags", []],
        "debug_type": [tango.DevString, "debug flags flags", []],
    }

    #    Command definitions
    cmd_list = {
        "getAttrStringValueList": [
            [tango.DevString, "Attribute name"],
            [tango.DevVarStringArray, "Authorized String value list"],
        ],
    }

    #    Attribute definitions
    attr_list = {
        "acqTimeoutRetry": [
            [tango.DevLong, tango.SCALAR, tango.READ_WRITE],
            {
                "unit": "number",
                "format": "%d",
                "description": "max Timeout retries during acq (0 - infinite)",
            },
        ],
        "adc": [
            [tango.DevLong, tango.SCALAR, tango.READ_WRITE],
            {"unit": "number", "format": "%d", "description": "number of working ADC"},
        ],
        "adcMax": [
            [tango.DevLong, tango.SCALAR, tango.READ],
            {"unit": "number", "format": "%d", "description": "max number of ADC"},
        ],
        "binInfo": [
            [tango.DevString, tango.SCALAR, tango.READ],
            {"unit": "N/A", "format": "%s", "description": "pco binning info"},
        ],
        "bitAlignment": [
            [tango.DevString, tango.SCALAR, tango.READ_WRITE],
            {"unit": "N/A", "format": "%s", "description": "bit alignment (MSB/LSB)"},
        ],
        "bytesPerPixel": [
            [tango.DevLong, tango.SCALAR, tango.READ],
            {"unit": "byte", "format": "%d", "description": "bytes per pixel"},
        ],
        "camInfo": [
            [tango.DevString, tango.SCALAR, tango.READ],
            {"unit": "N/A", "format": "%s", "description": "general cam information"},
        ],
        "camName": [
            [tango.DevString, tango.SCALAR, tango.READ],
            {"unit": "N/A", "format": "%s", "description": "camera name"},
        ],
        "camNameBase": [
            [tango.DevString, tango.SCALAR, tango.READ],
            {"unit": "N/A", "format": "%s", "description": "camera name (pco)"},
        ],
        "camNameEx": [
            [tango.DevString, tango.SCALAR, tango.READ],
            {
                "unit": "N/A",
                "format": "%s",
                "description": "camera name, interface, sensor",
            },
        ],
        "camType": [
            [tango.DevString, tango.SCALAR, tango.READ],
            {"unit": "N/A", "format": "%s", "description": "camera type"},
        ],
        "cdiMode": [
            [tango.DevLong, tango.SCALAR, tango.READ_WRITE],
            {"unit": "N/A", "format": "%d", "description": "CDI Mode"},
        ],
        "doubleImageMode": [
            [tango.DevLong, tango.SCALAR, tango.READ_WRITE],
            {"unit": "N/A", "format": "%d", "description": "Double Image Mode"},
        ],
        "clXferPar": [
            [tango.DevString, tango.SCALAR, tango.READ],
            {
                "unit": "N/A",
                "format": "%s",
                "description": "cameralink transfere parameters",
            },
        ],
        "cocRunTime": [
            [tango.DevDouble, tango.SCALAR, tango.READ],
            {"unit": "s", "format": "%g", "description": "coc Runtime"},
        ],
        "coolingTemperature": [
            [tango.DevLong, tango.SCALAR, tango.READ_WRITE],
            {"unit": "degrees", "format": "%d", "description": "cooling temperature"},
        ],
        "firmwareInfo": [
            [tango.DevString, tango.SCALAR, tango.READ],
            {"unit": "N/A", "format": "%s", "description": "firmware info"},
        ],
        "frameRate": [
            [tango.DevDouble, tango.SCALAR, tango.READ],
            {
                "unit": "frames/s",
                "format": "%g",
                "description": "frames per second (= 1/cocRuntime)",
            },
        ],
        "info": [
            [tango.DevString, tango.SCALAR, tango.READ],
            {"unit": "N/A", "format": "%s", "description": "general cam information"},
        ],
        "lastError": [
            [tango.DevString, tango.SCALAR, tango.READ],
            {"unit": "N/A", "format": "%s", "description": "last PCO error"},
        ],
        "lastImgAcquired": [
            [tango.DevLong, tango.SCALAR, tango.READ],
            {"unit": "N/A", "format": "%ld", "description": "last image acquired"},
        ],
        "lastImgRecorded": [
            [tango.DevLong, tango.SCALAR, tango.READ],
            {
                "unit": "N/A",
                "format": "%ld",
                "description": "last image recorded in camera RAM (not for all cams)",
            },
        ],
        "logMsg": [
            [tango.DevString, tango.SCALAR, tango.READ],
            {"unit": "N/A", "format": "%s", "description": "print the log msgs"},
        ],
        "logPcoEnabled": [
            [tango.DevLong, tango.SCALAR, tango.READ],
            {"unit": "N/A", "format": "%d", "description": "PCO logs are enabled"},
        ],
        "maxNbImages": [
            [tango.DevLong, tango.SCALAR, tango.READ],
            {
                "unit": "N/A",
                "format": "%ld",
                "description": "max nr of images in camera RAM (not for all cams)",
            },
        ],
        "pixelRate": [
            [tango.DevLong, tango.SCALAR, tango.READ_WRITE],
            {"unit": "Hz", "format": "%ld", "description": "pixel rate in Hz"},
        ],
        "pixelRateInfo": [
            [tango.DevString, tango.SCALAR, tango.READ],
            {"unit": "N/A", "format": "%s", "description": "pixel rate info"},
        ],
        "pixelRateValidValues": [
            [tango.DevString, tango.SCALAR, tango.READ],
            {
                "unit": "Hz",
                "format": "%s",
                "description": "pixel rate valid values in Hz",
            },
        ],
        "recorderForcedFifo": [
            [tango.DevLong, tango.SCALAR, tango.READ_WRITE],
            {
                "unit": "N/A",
                "format": "%ld",
                "description": "forced fifo mode for recording cams",
            },
        ],
        "roiInfo": [
            [tango.DevString, tango.SCALAR, tango.READ],
            {"unit": "N/A", "format": "%s", "description": "pco roi info"},
        ],
        "roiLastFixed": [
            [tango.DevString, tango.SCALAR, tango.READ],
            {"unit": "N/A", "format": "%s", "description": "last fixed roi info"},
        ],
        "rollingShutter": [
            [tango.DevLong, tango.SCALAR, tango.READ_WRITE],
            {
                "unit": "N/A",
                "format": "%d",
                "description": "1(Rolling), 2(Global), 4(Global Reset)",
            },
        ],
        "rollingShutterInfo": [
            [tango.DevString, tango.SCALAR, tango.READ],
            {"unit": "N/A", "format": "%s", "description": "rolling shutter info"},
        ],
        "temperatureInfo": [
            [tango.DevString, tango.SCALAR, tango.READ],
            {"unit": "N/A", "format": "%s", "description": "teperature info"},
        ],
        "traceAcq": [
            [tango.DevString, tango.SCALAR, tango.READ],
            {
                "unit": "N/A",
                "format": "%s",
                "description": "trace info during acq for some cameras",
            },
        ],
        "version": [
            [tango.DevString, tango.SCALAR, tango.READ],
            {"unit": "N/A", "format": "%s", "description": "complete version info"},
        ],
        "versionAtt": [
            [tango.DevString, tango.SCALAR, tango.READ],
            {"unit": "N/A", "format": "%s", "description": "att file version"},
        ],
        "versionSdk": [
            [tango.DevString, tango.SCALAR, tango.READ],
            {"unit": "N/A", "format": "%s", "description": "pco sdk release"},
        ],
        "camerasFound": [
            [tango.DevString, tango.SCALAR, tango.READ],
            {
                "unit": "N/A",
                "format": "%s",
                "description": "cameras found during the Open search",
            },
        ],
        "test": [
            [tango.DevString, tango.SCALAR, tango.READ_WRITE],
            {"unit": "N/A", "format": "%s", "description": "test"},
        ],
        "debugInt": [
            [tango.DevString, tango.SCALAR, tango.READ_WRITE],
            {
                "unit": "N/A",
                "format": "%s",
                "description": "internal debug level in hex format (0x....)",
            },
        ],
        "debugIntTypes": [
            [tango.DevString, tango.SCALAR, tango.READ],
            {
                "unit": "N/A",
                "format": "%s",
                "description": "internal debug level types",
            },
        ],
        "timestampMode": [
            [tango.DevLong, tango.SCALAR, tango.READ_WRITE],
            {"unit": "N/A", "format": "%d", "description": "timestamp mode"},
        ],
        "generalCAPS1": [
            [tango.DevString, tango.SCALAR, tango.READ],
            {"unit": "N/A", "format": "%s", "description": "general CAPS1 bits"},
        ],
        "rollingShutterStr": [
            [tango.DevString, tango.SCALAR, tango.WRITE],
            {"unit": "N/A", "format": "%s", "description": "set rolling shutter"},
        ],
        "paramsInfo": [
            [tango.DevString, tango.SCALAR, tango.READ],
            {"unit": "N/A", "format": "%s", "description": "params info"},
        ],
    }

    # ------------------------------------------------------------------
    #    PcoClass Constructor
    # ------------------------------------------------------------------
    def __init__(self, name):
        tango.DeviceClass.__init__(self, name)
        self.set_type(name)


# ----------------------------------------------------------------------------
# Plugins
# ----------------------------------------------------------------------------
_PcoCam = None
_PcoInterface = None
_PcoControl = None


def get_control(
    debug_control="0",
    debug_module="0",
    debug_type="0",
    mem_factor="0",
    debug_format="0x31",
    params=[],
    **keys
):

    global _PcoCam
    global _PcoInterface
    global _PcoControl

    debControl = int(debug_control, 0)
    debModule = int(debug_module, 0)
    debType = int(debug_type, 0)
    debFormat = int(debug_format, 0)
    memFactor = int(mem_factor, 0)

    if type(params) == str:
        # <type 'str'>
        paramsIn = params
    else:
        # <class 'tango._tango.StdStringVector'>
        paramsIn = "".join("%s;" % (x,) for x in params)

    bParamsIn = paramsIn.encode()
    # bParamsIn -> bytes
    #   <class 'bytes'>
    #   b'bitAlignment = MSB;trigSingleMulti=1;'

    print("============= Properties =============")
    print("         keys:", keys)
    print("       params:", params)
    print("     paramsIn:", paramsIn)
    print("%s [%s] [0x%x]" % ("debug_control:", debug_control, debControl))
    print("%s [%s] [0x%x]" % (" debug_module:", debug_module, debModule))
    print("%s [%s] [0x%x]" % (" debug_format:", debug_format, debFormat))
    print("%s [%s] [0x%x]" % ("   debug_type:", debug_type, debType))
    print("%s [%s] [0x%x]" % ("   mem_factor:", mem_factor, memFactor))
    print("======================================")

    if debControl:
        Core.DebParams.setModuleFlags(debModule)
        Core.DebParams.setTypeFlags(debType)
    else:
        Core.DebParams.setTypeFlags(0)
        Core.DebParams.setModuleFlags(0)

    Core.DebParams.setFormatFlags(debFormat)

    if _PcoCam is None:
        _PcoCam = PcoAcq.Camera(paramsIn)
        _PcoInterface = PcoAcq.Interface(_PcoCam)
        _PcoControl = Core.CtControl(_PcoInterface)

    return _PcoControl


# ----------------------------------------------------------------------------
# ----------------------------------------------------------------------------
def get_tango_specific_class_n_device():
    return PcoClass, Pco


# ==================================================================================
# ==================================================================================
# called by ->  delete_device (LimaCCDs.py:352)   [LimaCCDs(id00/limaccds/pco2k1)]
# requiered to close properly the camera / sdk
#   because the cam, interface destructors are NOT called (?)
# ==================================================================================
def close_interface():
    print("... close_interface()")
    _PcoInterface.reset(RESET_CLOSE_INTERFACE)
