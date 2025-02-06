PCO Tango device
================

This is the reference documentation of the PCO Tango device.

You can also find some useful information about the camera models/prerequisite/installation/configuration/compilation in the :ref:`PCO camera plugin <camera-pco>` section.


Properties
----------

=============== =============== =============== ==============================================================
Property name   Mandatory       Default value   Description
=============== =============== =============== ==============================================================
sn              No              empty           Camera serial number
                                                if it is 0 or doesn't exist, the first camera found will be opened
                                                if the serial number is not found, OpenCam will fail
params          No              empty           List of parameters/options (one per line)
                                                  - trigSingleMulti = 1 
                                                    (
                                                    enable TriggerSingleMulti as TriggerMulti for compability
                                                    with SPEC START
                                                    )  
                                                  - xMinSize = 1
                                                    (
                                                    enable correction for the X minimum size for the CLHS firmware bug
                                                    ) 
                                                  - bitAligment = <MSB | LSB>
                                                    (
                                                    bit aligment of the image data, i.e. for 12b: 
                                                    [MSB - xxxx xxxx xxxx 0000]
                                                    [LSB - 0000 xxxx xxxx xxxx]
                                                    )
=============== =============== =============== ==============================================================


Attributes
----------

======================= ======= ======================= ======================================================================
Attribute name          RW      Type                    Description
======================= ======= ======================= ======================================================================
acqTimeoutRetry         rw      DevLong                 Maximum Timeout retries during acq (0 - infinite)
acq_timeout             rw      DevLong                 Timeout during acq in second
adc                     rw      DevLong                 Number of working ADC's
adcMax                  ro      DevLong                 Maximum number of ADC's
binInfo                 ro      DevLong                 PCO hw binning info
bitAlignment            rw      DevString               Bit alignment
                                                         - MSB (0)
                                                         - LSB (1)
bytesPerPixel           ro      DevLong                 Bytes per Pixel
camerasFound            ro      DevString               List of cameras found during the Open search
camInfo                 ro      DevString               General camera parameters information
camName                 ro      DevString               Camera Name
camNameBase             ro      DevString               Camera Name (Pco)
camNameEx               ro      DevString               Camera Name, Interface, Sensor
camType                 ro      DevString               Camera Type
cdiMode                 rw      DevLong                 Correlated Double Imaging Mode
                                                         - enabled/disabled = 1/0 (rw)
                                                         - not allowed = -1 (ro)
clXferPar               ro      DevString               General CameraLink parameters
cocRunTime              ro      DevDouble               cocRunTime (s) - only valid after the camera is armed
coolingTemperature      ro      DevDouble               Cooling Temperature
debugInt                rw      DevString               PCO plugin internal debug level (hex format: 0x....)
debugIntTypes           ro      DevString               PCO plugin internal debug types
doubleImageMode         rw      DevLong                 Double Image Mode
                                                         - enabled/disabled = 1/0 (rw)
                                                         - not allowed = -1 (ro)
firmwareInfo            ro      DevString               Firmware info
frameRate               ro      DevDouble               Framerate, calculated as: 1/cocRunTime (1/s)
generalCAPS1            ro      DevString               General PCO CAPS1 value (hex and bin)
info                    ro      DevString               General camera parameters information
lastError               ro      DevString               The last PCO error message
lastImgAcquired         ro      DevLong                 Last image acquired (during recording)
lastImgRecorded         ro      DevLong                 Last image recorded (during recording)
logMsg                  ro      DevString               Last Log msgs
logPcoEnabled           ro      DevLong                 PCO logs are enabled
maxNbImages             ro      DevLong                 The maximum number of images which can be acquired by the camera (recording mode)
paramsInfo              ro      DevString               Values of the PCO properties **params**
pixelRate               ro      DevLong                 Actual Pixel Rate (Hz)
pixelRateInfo           ro      DevString               Pixel Rate information
pixelRateValidValues    ro      DevString               Allowed Pixel Rates
recorderForcedFifo      rw      DevLong                 Forced Fifo Mode (**only for recording cams**)
roiInfo                 ro      DevString               PCO ROI info
roiLastFixed            ro      DevString               Last fixed ROI info
rollingShutter          rw      DevLong                 Rolling Shutter Mode as int (**only for some types of EDGE**)
                                                         - 1 = ROLLING
                                                         - 2 = GLOBAL
                                                         - 4 = GLOBAL RESET
rollingShutterInfo      ro      DevString               Rolling Shutter info
rollingShutterStr       rw      DevLong                 Rolling Shutter Mode as str (**only for some types of EDGE**)
temperatureInfo         ro      DevString               Temperature info
test                    rw      DevString               Debug test function (**do not use it**)
timestampMode           rw      DevLong                 Timestamp mode
                                                         - 0 = none
                                                         - 1 = BCD coded stamp in the first 14 pixel
                                                         - 2 = BCD coded stamp in the first 14 pixel + ASCII text
                                                         - 3 = ASCII text (**only for some cameras**)
traceAcq                ro      DevString               Debug information for some types of acq
version                 ro      DevString               Version information of the plugin
versionAtt              ro      DevString               Version of att file
versionSdk              ro      DevString               PCO SDK Release
======================= ======= ======================= ======================================================================


Commands
--------

======================= =============== ======================= ===========================================
Command name            Arg. in         Arg. out                Description
======================= =============== ======================= ===========================================
Init                    DevVoid         DevVoid                 Do NOT use
State                   DevVoid         DevLong                 Return the device state
Status                  DevVoid         DevString               Return the device state as a string
getAttrStringValueList  DevString:      DevVarStringArray:      Return the authorized string value list for
                        Attribute name  String value list       a given attribute name
======================= =============== ======================= ===========================================
