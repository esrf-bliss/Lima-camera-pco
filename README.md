[![License](https://img.shields.io/github/license/esrf-bliss/lima.svg?style=flat)](https://opensource.org/licenses/GPL-3.0)
[![Gitter](https://img.shields.io/gitter/room/esrf-bliss/lima.svg?style=flat)](https://gitter.im/esrf-bliss/LImA)
[![Conda](https://img.shields.io/conda/dn/esrf-bcu/lima-camera-pco.svg?style=flat)](https://anaconda.org/esrf-bcu)
[![Version](https://img.shields.io/conda/vn/esrf-bcu/lima-camera-pco.svg?style=flat)](https://anaconda.org/esrf-bcu)
[![Platform](https://img.shields.io/conda/pn/esrf-bcu/lima-camera-pco.svg?style=flat)](https://anaconda.org/esrf-bcu)

# LImA Pco Camera Plugin

This is the LImA plugin for Pco cameras.

## Install

### Camera python

PCO Tango device
================

This is the reference documentation of the PCO Tango device.

You can also find some useful information about the camera
models/prerequisite/installation/configuration/compilation in the
PCO camera plugin \<camera-pco\> section.

**Properties**

Property name | Mandatory | Default value | Description
------------- | --------- | ------------- | -----------
debug\_control | No | 0 | Enable/Disble the debug (0/1)    
debug\_module  | No  |  0  |  To set the debug module list (in hex format 0x....)
 . | . | . | - None = 0x001
 . | . | . | - Common = 0x002
 . | . | . | - Hardware = 0x004
 . | . | . | - HardwareSerial = 0x008
 . | . | . | - Control = 0x010
 . | . | . | - Espia = 0x020
 . | . | . | - EspiaSerial = 0x040
 . | . | . | - Focla = 0x080
 . | . | . | - Camera = 0x100
 . | . | . | - CameraCom = 0x200
 . | . | . | - Test = 0x400
 . | . | . | - Application = 0x800
debug\_format  | No  |  0  |  To set the debug format (in hex format 0x....)
 . | . | . | - DateTime = 0x001
 . | . | . | - Thread = 0x002
 . | . | . | - Module = 0x004
 . | . | . | - Obj = 0x008
 . | . | . | - Funct = 0x010
 . | . | . | - FileLine = 0x020
 . | . | . | - Type = 0x040
 . | . | . | - Indent = 0x080
 . | . | . | - Color = 0x100
debug\_type  | No  |  0  |  To set the debug type (in hex format 0x....)
 . | . | . | - Fatal = 0x001
 . | . | . | - Error = 0x002
 . | . | . | - Warning = 0x004
 . | . | . | - Trace = 0x008
 . | . | . | - Funct = 0x010
 . | . | . | - Param = 0x020
 . | . | . | - Return = 0x040
 . | . | . | - Always = 0x080
params | No | empty | List of parameters/options (one per line)
 . | . | . | - sn = \<camera serial number\> (if it is 0 or doesn't exist, the first camera found will be opened) (if the serial number is not found, OpenCam will fail)
 . | . | . | - trigSingleMulti = 1 (enable TriggerSingleMulti as TriggerMulti for compability with SPEC START)
 . | . | . | - xMinSize = 1 (enable correction for the X minimum size for the CLHS firmware bug)
 . | . | . | - bitAligment = \<MSB or LSB\> (bit aligment of the image data, i.e. for 12b: (MSB - xxxx xxxx xxxx 0000) (LSB - 0000 xxxx xxxx xxxx))



Attributes
==========

      Attribute name         RW   Type        Description
      ---------------------- ---- ----------- -----------------------------------------------------------------------------------
      acqTimeoutRetry        rw   DevLong     Maximum Timeout retries during acq (0 - infinite)
      adc                    rw   DevLong     Number of working ADC's
      adcMax                 ro   DevLong     Maximum number of ADC's
      binInfo                ro   DevLong     PCO hw binning info
      bitAlignment           rw   DevString   Bit alignment
                                              - MSB (0)
                                              - LSB (1)
      bytesPerPixel          ro   DevLong     Bytes per Pixel
      camerasFound           ro   DevString   List of cameras found during the Open search
      camInfo                ro   DevString   General camera parameters information
      camName                ro   DevString   Camera Name
      camNameBase            ro   DevString   Camera Name (Pco)
      camNameEx              ro   DevString   Camera Name, Interface, Sensor
      camType                ro   DevString   Camera Type
      cdiMode                rw   DevLong     Correlated Double Imaging Mode
                                              - enabled/disabled = 1/0 (rw)
                                              - not allowed = -1 (ro)
      clXferPar              ro   DevString   General CameraLink parameters
      cocRunTime             ro   DevDouble   cocRunTime (s) - only valid after the camera is armed
      coolingTemperature     ro   DevDouble   Cooling Temperature
      debugInt               rw   DevString   PCO plugin internal debug level (hex format: 0x....)
      debugIntTypes          r0   DevString   PCO plugin internal debug types
      doubleImageMode        rw   DevLong     Double Image Mode
                                              - enabled/disabled = 1/0 (rw)
                                              - not allowed = -1 (ro)
      firmwareInfo           ro   DevString   Firmware info
      frameRate              ro   DevDouble   Framerate, calculated as: 1/cocRunTime (1/s)
      info                   ro   DevString   General camera parameters information
      lastError              ro   DevString   The last PCO error message
      lastImgAcquired        ro   DevLong     Last image acquired (during recording)
      lastImgRecorded        ro   DevLong     Last image recorded (during recording)
      logMsg                 ro   DevString   Last Log msgs
      logPcoEnabled          ro   DevLong     PCO logs are enabled
      maxNbImages            ro   DevLong     The maximum number of images which can be acquired by the camera (recording mode)
      pixelRate              ro   DevLong     Actual Pixel Rate (Hz)
      pixelRateInfo          ro   DevString   Pixel Rate information
      pixelRateValidValues   ro   DevString   Allowed Pixel Rates
      recorderForcedFifo     rw   DevLong     Forced Fifo Mode (**only for recording cams**)
      roiInfo                ro   DevString   PCO ROI info
      roiLastFixed           ro   DevString   Last fixed ROI info
      rollingShutter         rw   DevLong     Rolling Shutter Mode (**only for some types of EDGE**)
                                              - 1 = ROLLING
                                              - 2 = GLOBAL
                                              - 4 = GLOBAL RESET
      rollingShutterInfo     ro   DevString   Rolling Shutter info
      temperatureInfo        ro   DevString   Temperature info
      timestampMode          rw   DevLong     Timestamp mode
                                              - 0 = none
                                              - 1 = BCD coded stamp in the first 14 pixel
                                              - 2 = BCD coded stamp in the first 14 pixel + ASCII text
                                              - 3 = ASCII text (**only for some cameras**)
      traceAcq               ro   DevString   Debug information for some types of acq
      version                ro   DevString   Version information of the plugin
      versionAtt             ro   DevString   Version of att file
      versionSdk             ro   DevString   PCO SDK Release

Commands
========

      Command name             Arg. in          Arg. out             Description
      ------------------------ ---------------- -------------------- ------------------------------------------------
      Init                     DevVoid          DevVoid              Do NOT use
      State                    DevVoid          DevLong              Return the device state
      Status                   DevVoid          DevString            Return the device state as a string
      getAttrStringValueList   DevString:       DevVarStringArray:   Return the authorized string value list for
                               Attribute name   String value list    a given attribute name

```
conda install -c esrf-bcu lima-camera-pco
```

### Camera tango device server

```
conda install -c tango-controls -c esrf-bcu lima-camera-pco-tango
```

# LImA

Lima ( **L** ibrary for **Im** age **A** cquisition) is a project for the unified control of 2D detectors. The aim is to clearly separate hardware specific code from common software configuration and features, like setting standard acquisition parameters (exposure time, external trigger), file saving and image processing.

Lima is a C++ library which can be used with many different cameras. The library also comes with a [Python](http://python.org) binding and provides a [PyTango](http://pytango.readthedocs.io/en/stable/) device server for remote control.

## Documentation

The documentation is available [here](https://lima1.readthedocs.org)