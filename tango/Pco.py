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

from tango import AttrWriteType, DevState
from tango.server import Device, attribute, command, device_property

from lima import core
from lima.limapco import Pco as PcoAcq
from lima.server import AttrHelper

RESET_CLOSE_INTERFACE = 100


def _make_pco_fget_fset(function_base, interface):
    """Local helper: dispatch via an explicit Get<function_base>/Set<function_base>
    method pair (mirrors the legacy __Attribute2FunctionBase override table).
    AttrHelper.make_fget_fset()'s naming convention (title-casing each
    underscore-separated word) can't reproduce PCO's mixed-case method names
    (e.g. CDIMode, ClTransferParam) from a snake_case attribute identifier,
    so PCO keeps its own explicit function-name table instead.
    """
    get_name, set_name = "get" + function_base, "set" + function_base

    def fget(self):
        return getattr(interface(self), get_name)()

    def fset(self, value):
        getattr(interface(self), set_name)(value)

    return fget, fset


class Pco(Device):

    core.DEB_CLASS(core.DebModule.DebModApplication, "LimaCCDs")

    # ------------------------------------------------------------------
    #    Static properties
    # ------------------------------------------------------------------
    _PcoCam = None
    """Reference to Lima pco camera binding"""
    _PcoInterface = None
    """Reference to Lima pco interface binding"""

    # ------------------------------------------------------------------
    #    Device properties
    # ------------------------------------------------------------------
    sn = device_property(dtype=str, doc="camera serial number")
    params = device_property(dtype=str, doc="general parameters")
    acq_timeout = device_property(dtype=float, doc="acq trigger timeout")
    acq_thread_numa_node = device_property(
        dtype=int, default_value=0, doc="acq thread numa node"
    )
    acq_thread_rt_priority = device_property(
        dtype=int,
        default_value=10,
        doc="acq thread SCHED_FIFO real-time priority (1-99)",
    )
    acq_thread_irq_pci_addr = device_property(
        dtype=str,
        doc="PCI address of this camera's frame grabber (e.g. '86:00.0'), used to "
        "keep its IRQ CPU out of the acq thread's cpuset (empty disables this)",
    )

    # ------------------------------------------------------------------
    #    Device initialization
    # ------------------------------------------------------------------
    @core.DEB_MEMBER_FUNCT
    def init_device(self):
        Device.init_device(self)
        self.set_state(DevState.ON)

        if self.acq_timeout:
            self._PcoCam.setAcqTimeout(self.acq_timeout)

    # ------------------------------------------------------------------
    #    Commands
    # ------------------------------------------------------------------
    @command(
        dtype_in=str,
        doc_in="Attribute name",
        dtype_out=(str,),
        doc_out="Authorized String value list",
    )
    @core.DEB_MEMBER_FUNCT
    def getAttrStringValueList(self, attr_name):
        return AttrHelper.get_attr_string_value_list(self, attr_name)

    # ------------------------------------------------------------------
    #    Attribute with a name colliding with a device_property
    # ------------------------------------------------------------------
    _acq_timeout_fget, _acq_timeout_fset = _make_pco_fget_fset(
        "AcqTimeout", lambda self: self._PcoCam
    )
    acq_timeout_attr = attribute(
        name="acq_timeout",
        dtype=float,
        access=AttrWriteType.READ_WRITE,
        label="Acquisition timeout",
        unit="s",
        format="%f",
        doc="Acquisition timeout in s",
        fget=_acq_timeout_fget,
        fset=_acq_timeout_fset,
    )

    # ------------------------------------------------------------------
    #    Attributes dispatched via the explicit function-name table
    # ------------------------------------------------------------------
    _acq_timeout_retry_fget, _acq_timeout_retry_fset = _make_pco_fget_fset("AcqTimeoutRetry", lambda self: self._PcoCam)
    acqTimeoutRetry = attribute(
        dtype=int,
        access=AttrWriteType.READ_WRITE,
        unit="number",
        format="%d",
        doc="Max Timeout retries during acq (0 - infinite)",
        fget=_acq_timeout_retry_fget,
        fset=_acq_timeout_retry_fset,
    )

    _adc_fget, _adc_fset = _make_pco_fget_fset("Adc", lambda self: self._PcoCam)
    adc = attribute(
        dtype=int,
        access=AttrWriteType.READ_WRITE,
        unit="number",
        format="%d",
        doc="Number of working ADC",
        fget=_adc_fget,
        fset=_adc_fset,
    )

    _adc_max_fget, _ = _make_pco_fget_fset("AdcMax", lambda self: self._PcoCam)
    adcMax = attribute(
        dtype=int,
        access=AttrWriteType.READ,
        unit="number",
        format="%d",
        doc="Max number of ADC",
        fget=_adc_max_fget,
    )

    _bin_info_fget, _ = _make_pco_fget_fset("BinningInfo", lambda self: self._PcoCam)
    binInfo = attribute(
        dtype=str,
        access=AttrWriteType.READ,
        format="%s",
        doc="Pco binning info",
        fget=_bin_info_fget,
    )

    _bit_alignment_fget, _bit_alignment_fset = _make_pco_fget_fset("BitAlignment", lambda self: self._PcoCam)
    bitAlignment = attribute(
        dtype=str,
        access=AttrWriteType.READ_WRITE,
        format="%s",
        doc="Bit alignment (MSB/LSB)",
        fget=_bit_alignment_fget,
        fset=_bit_alignment_fset,
    )

    _bytes_per_pixel_fget, _ = _make_pco_fget_fset("BytesPerPixel", lambda self: self._PcoCam)
    bytesPerPixel = attribute(
        dtype=int,
        access=AttrWriteType.READ,
        unit="byte",
        format="%d",
        doc="Bytes per pixel",
        fget=_bytes_per_pixel_fget,
    )

    _cam_info_fget, _ = _make_pco_fget_fset("CamInfo", lambda self: self._PcoCam)
    camInfo = attribute(
        dtype=str,
        access=AttrWriteType.READ,
        format="%s",
        doc="General cam information",
        fget=_cam_info_fget,
    )

    _cam_name_fget, _ = _make_pco_fget_fset("CameraName", lambda self: self._PcoCam)
    camName = attribute(
        dtype=str,
        access=AttrWriteType.READ,
        format="%s",
        doc="Camera name",
        fget=_cam_name_fget,
    )

    _cam_name_base_fget, _ = _make_pco_fget_fset("CameraNameBase", lambda self: self._PcoCam)
    camNameBase = attribute(
        dtype=str,
        access=AttrWriteType.READ,
        format="%s",
        doc="Camera name (pco)",
        fget=_cam_name_base_fget,
    )

    _cam_name_ex_fget, _ = _make_pco_fget_fset("CameraNameEx", lambda self: self._PcoCam)
    camNameEx = attribute(
        dtype=str,
        access=AttrWriteType.READ,
        format="%s",
        doc="Camera name, interface, sensor",
        fget=_cam_name_ex_fget,
    )

    _cam_type_fget, _ = _make_pco_fget_fset("CamType", lambda self: self._PcoCam)
    camType = attribute(
        dtype=str,
        access=AttrWriteType.READ,
        format="%s",
        doc="Camera type",
        fget=_cam_type_fget,
    )

    _cdi_mode_fget, _cdi_mode_fset = _make_pco_fget_fset("CDIMode", lambda self: self._PcoCam)
    cdiMode = attribute(
        dtype=int,
        access=AttrWriteType.READ_WRITE,
        format="%d",
        doc="CDI Mode",
        fget=_cdi_mode_fget,
        fset=_cdi_mode_fset,
    )

    _double_image_mode_fget, _double_image_mode_fset = _make_pco_fget_fset("DoubleImageMode", lambda self: self._PcoCam)
    doubleImageMode = attribute(
        dtype=int,
        access=AttrWriteType.READ_WRITE,
        format="%d",
        doc="Double Image Mode",
        fget=_double_image_mode_fget,
        fset=_double_image_mode_fset,
    )

    _cl_xfer_par_fget, _ = _make_pco_fget_fset("ClTransferParam", lambda self: self._PcoCam)
    clXferPar = attribute(
        dtype=str,
        access=AttrWriteType.READ,
        format="%s",
        doc="Cameralink transfere parameters",
        fget=_cl_xfer_par_fget,
    )

    _coc_run_time_fget, _ = _make_pco_fget_fset("CocRunTime", lambda self: self._PcoCam)
    cocRunTime = attribute(
        dtype=float,
        access=AttrWriteType.READ,
        unit="s",
        format="%g",
        doc="Coc Runtime",
        fget=_coc_run_time_fget,
    )

    _cooling_temperature_fget, _cooling_temperature_fset = _make_pco_fget_fset("CoolingTemperature", lambda self: self._PcoCam)
    coolingTemperature = attribute(
        dtype=int,
        access=AttrWriteType.READ_WRITE,
        unit="degrees",
        format="%d",
        doc="Cooling temperature",
        fget=_cooling_temperature_fget,
        fset=_cooling_temperature_fset,
    )

    _firmware_info_fget, _ = _make_pco_fget_fset("FirmwareInfo", lambda self: self._PcoCam)
    firmwareInfo = attribute(
        dtype=str,
        access=AttrWriteType.READ,
        format="%s",
        doc="Firmware info",
        fget=_firmware_info_fget,
    )

    _frame_rate_fget, _ = _make_pco_fget_fset("FrameRate", lambda self: self._PcoCam)
    frameRate = attribute(
        dtype=float,
        access=AttrWriteType.READ,
        unit="frames/s",
        format="%g",
        doc="Frames per second (= 1/cocRuntime)",
        fget=_frame_rate_fget,
    )

    _info_fget, _ = _make_pco_fget_fset("CamInfo", lambda self: self._PcoCam)
    info = attribute(
        dtype=str,
        access=AttrWriteType.READ,
        format="%s",
        doc="General cam information",
        fget=_info_fget,
    )

    _last_error_fget, _ = _make_pco_fget_fset("LastError", lambda self: self._PcoCam)
    lastError = attribute(
        dtype=str,
        access=AttrWriteType.READ,
        format="%s",
        doc="Last PCO error",
        fget=_last_error_fget,
    )

    _last_img_acquired_fget, _ = _make_pco_fget_fset("LastImgAcquired", lambda self: self._PcoCam)
    lastImgAcquired = attribute(
        dtype=int,
        access=AttrWriteType.READ,
        format="%ld",
        doc="Last image acquired",
        fget=_last_img_acquired_fget,
    )

    _last_img_recorded_fget, _ = _make_pco_fget_fset("LastImgRecorded", lambda self: self._PcoCam)
    lastImgRecorded = attribute(
        dtype=int,
        access=AttrWriteType.READ,
        format="%ld",
        doc="Last image recorded in camera RAM (not for all cams)",
        fget=_last_img_recorded_fget,
    )

    _log_msg_fget, _ = _make_pco_fget_fset("MsgLog", lambda self: self._PcoCam)
    logMsg = attribute(
        dtype=str,
        access=AttrWriteType.READ,
        format="%s",
        doc="Print the log msgs",
        fget=_log_msg_fget,
    )

    _max_nb_images_fget, _ = _make_pco_fget_fset("MaxNbImages", lambda self: self._PcoCam)
    maxNbImages = attribute(
        dtype=int,
        access=AttrWriteType.READ,
        format="%ld",
        doc="Max nr of images in camera RAM (not for all cams)",
        fget=_max_nb_images_fget,
    )

    _pixel_rate_fget, _pixel_rate_fset = _make_pco_fget_fset("PixelRate", lambda self: self._PcoCam)
    pixelRate = attribute(
        dtype=int,
        access=AttrWriteType.READ_WRITE,
        unit="Hz",
        format="%ld",
        doc="Pixel rate in Hz",
        fget=_pixel_rate_fget,
        fset=_pixel_rate_fset,
    )

    _pixel_rate_info_fget, _ = _make_pco_fget_fset("PixelRateInfo", lambda self: self._PcoCam)
    pixelRateInfo = attribute(
        dtype=str,
        access=AttrWriteType.READ,
        format="%s",
        doc="Pixel rate info",
        fget=_pixel_rate_info_fget,
    )

    _pixel_rate_valid_values_fget, _ = _make_pco_fget_fset("PixelRateValidValues", lambda self: self._PcoCam)
    pixelRateValidValues = attribute(
        dtype=str,
        access=AttrWriteType.READ,
        unit="Hz",
        format="%s",
        doc="Pixel rate valid values in Hz",
        fget=_pixel_rate_valid_values_fget,
    )

    _recorder_forced_fifo_fget, _recorder_forced_fifo_fset = _make_pco_fget_fset("RecorderForcedFifo", lambda self: self._PcoCam)
    recorderForcedFifo = attribute(
        dtype=int,
        access=AttrWriteType.READ_WRITE,
        format="%ld",
        doc="Forced fifo mode for recording cams",
        fget=_recorder_forced_fifo_fget,
        fset=_recorder_forced_fifo_fset,
    )

    _roi_info_fget, _ = _make_pco_fget_fset("RoiInfo", lambda self: self._PcoCam)
    roiInfo = attribute(
        dtype=str,
        access=AttrWriteType.READ,
        format="%s",
        doc="Pco roi info",
        fget=_roi_info_fget,
    )

    _roi_last_fixed_fget, _ = _make_pco_fget_fset("LastFixedRoi", lambda self: self._PcoCam)
    roiLastFixed = attribute(
        dtype=str,
        access=AttrWriteType.READ,
        format="%s",
        doc="Last fixed roi info",
        fget=_roi_last_fixed_fget,
    )

    _rolling_shutter_fget, _rolling_shutter_fset = _make_pco_fget_fset("RollingShutter", lambda self: self._PcoCam)
    rollingShutter = attribute(
        dtype=int,
        access=AttrWriteType.READ_WRITE,
        format="%d",
        doc="1(Rolling), 2(Global), 4(Global Reset)",
        fget=_rolling_shutter_fget,
        fset=_rolling_shutter_fset,
    )

    _rolling_shutter_info_fget, _ = _make_pco_fget_fset("RollingShutterInfo", lambda self: self._PcoCam)
    rollingShutterInfo = attribute(
        dtype=str,
        access=AttrWriteType.READ,
        format="%s",
        doc="Rolling shutter info",
        fget=_rolling_shutter_info_fget,
    )

    _temperature_info_fget, _ = _make_pco_fget_fset("TemperatureInfo", lambda self: self._PcoCam)
    temperatureInfo = attribute(
        dtype=str,
        access=AttrWriteType.READ,
        format="%s",
        doc="Teperature info",
        fget=_temperature_info_fget,
    )

    _trace_acq_fget, _ = _make_pco_fget_fset("TraceAcq", lambda self: self._PcoCam)
    traceAcq = attribute(
        dtype=str,
        access=AttrWriteType.READ,
        format="%s",
        doc="Trace info during acq for some cameras",
        fget=_trace_acq_fget,
    )

    _version_fget, _ = _make_pco_fget_fset("Version", lambda self: self._PcoCam)
    version = attribute(
        dtype=str,
        access=AttrWriteType.READ,
        format="%s",
        doc="Complete version info",
        fget=_version_fget,
    )

    # versionAtt has no entry in the legacy __Attribute2FunctionBase table,
    # so it resolves to the same (non-existent) getVersionatt/setVersionatt
    # names as before - pre-existing dead attribute, faithfully reproduced.
    _version_att_fget, _ = _make_pco_fget_fset("Versionatt", lambda self: self._PcoCam)
    versionAtt = attribute(
        dtype=str,
        access=AttrWriteType.READ,
        format="%s",
        doc="Att file version",
        fget=_version_att_fget,
    )

    _version_sdk_fget, _ = _make_pco_fget_fset("SdkRelease", lambda self: self._PcoCam)
    versionSdk = attribute(
        dtype=str,
        access=AttrWriteType.READ,
        format="%s",
        doc="Pco sdk release",
        fget=_version_sdk_fget,
    )

    _cameras_found_fget, _ = _make_pco_fget_fset("CamerasFound", lambda self: self._PcoCam)
    camerasFound = attribute(
        dtype=str,
        access=AttrWriteType.READ,
        format="%s",
        doc="Cameras found during the Open search",
        fget=_cameras_found_fget,
    )

    _test_fget, _test_fset = _make_pco_fget_fset("Test", lambda self: self._PcoCam)
    test = attribute(
        dtype=str,
        access=AttrWriteType.READ_WRITE,
        format="%s",
        doc="Test",
        fget=_test_fget,
        fset=_test_fset,
    )

    _debug_int_fget, _debug_int_fset = _make_pco_fget_fset("DebugInt", lambda self: self._PcoCam)
    debugInt = attribute(
        dtype=str,
        access=AttrWriteType.READ_WRITE,
        format="%s",
        doc="Internal debug level in hex format (0x....)",
        fget=_debug_int_fget,
        fset=_debug_int_fset,
    )

    _debug_int_types_fget, _ = _make_pco_fget_fset("DebugIntTypes", lambda self: self._PcoCam)
    debugIntTypes = attribute(
        dtype=str,
        access=AttrWriteType.READ,
        format="%s",
        doc="Internal debug level types",
        fget=_debug_int_types_fget,
    )

    _timestamp_mode_fget, _timestamp_mode_fset = _make_pco_fget_fset("TimestampMode", lambda self: self._PcoCam)
    timestampMode = attribute(
        dtype=int,
        access=AttrWriteType.READ_WRITE,
        format="%d",
        doc="Timestamp mode",
        fget=_timestamp_mode_fget,
        fset=_timestamp_mode_fset,
    )

    _general_caps1_fget, _ = _make_pco_fget_fset("GeneralCAPS1", lambda self: self._PcoCam)
    generalCAPS1 = attribute(
        dtype=str,
        access=AttrWriteType.READ,
        format="%s",
        doc="General CAPS1 bits",
        fget=_general_caps1_fget,
    )

    _, _rolling_shutter_str_fset = _make_pco_fget_fset("RollingShutterStr", lambda self: self._PcoCam)
    rollingShutterStr = attribute(
        dtype=str,
        access=AttrWriteType.WRITE,
        format="%s",
        doc="Set rolling shutter",
        fset=_rolling_shutter_str_fset,
    )

    _params_info_fget, _ = _make_pco_fget_fset("ParamsInfo", lambda self: self._PcoCam)
    paramsInfo = attribute(
        dtype=str,
        access=AttrWriteType.READ,
        format="%s",
        doc="Params info",
        fget=_params_info_fget,
    )




# ----------------------------------------------------------------------------
# Plugins
# ----------------------------------------------------------------------------
_PcoControl = None


def get_control(
    sn: str = "",
    acq_thread_numa_node: int = 0,
    acq_thread_rt_priority: int = 10,
    acq_thread_irq_pci_addr: str = "",
    _Pco=Pco,
    _Camera=PcoAcq.Camera,
    _Interface=PcoAcq.Interface,
    **keys
):
    global _PcoControl

    print("============= Properties =============")
    print("           sn:", sn)
    print("    numa_node:", acq_thread_numa_node)
    print("  rt_priority:", acq_thread_rt_priority)
    print(" irq_pci_addr:", acq_thread_irq_pci_addr)
    print("         keys:", keys)
    print("======================================")

    camera = _Pco._PcoCam
    interface = _Pco._PcoInterface

    if camera is None:
        camera = _Camera(sn, int(acq_thread_numa_node), int(acq_thread_rt_priority), acq_thread_irq_pci_addr)
        interface = _Interface(camera)
        _Pco._PcoCam = camera
        _Pco._PcoInterface = interface
        _PcoControl = core.CtControl(interface)

    return _PcoControl


def get_tango_specific_class_n_device():
    return Pco.TangoClassClass, Pco


# ==================================================================================
# ==================================================================================
# called by ->  delete_device (LimaCCDs.py:352)   [LimaCCDs(id00/limaccds/pco2k1)]
# requiered to close properly the camera / sdk
#   because the cam, interface destructors are NOT called (?)
# ==================================================================================
def close_interface():
    global _PcoControl

    try:
        Pco._PcoInterface.cleanup()

        _PcoControl = None
        Pco._PcoInterface = None
        Pco._PcoCam = None
    except BaseException as e:
        print("close_interface: BaseException", e)
