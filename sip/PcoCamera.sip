namespace Pco
{
  class Camera
  {
%TypeHeaderCode
#include "lima/HwInterface.h"
#include <PcoCamera.h>
%End
  public:

        Camera(const std::string&);
        ~Camera();


        void 	startAcq();
        void	reset(int);

        std::string talk(const std::string& cmd);

        int getNbAcquiredFrames();

        void getMaxWidthHeight(unsigned int& width,unsigned int& height /Out/);


//--------------------------------------------------------


		void getAcqTimeoutRetry(int &val /Out/);   // acqTimeoutRetry
		void setAcqTimeoutRetry(int val);

		void getAdc(int & adc /Out/);    // adc
		void setAdc(int val);

		void getAdcMax(int & adc /Out/);    // adcMax

		void getBinningInfo(std::string &o_sn /Out/);

		void getBytesPerPixel(unsigned int& pixbytes /Out/); // bytesPerPixel

		void getCamerasFound(std::string &o_sn /Out/) ;
		void getCamInfo(std::string &o_sn /Out/) ;    // camInfo
		void getCameraName(std::string &o_sn /Out/);
		void getCameraNameBase(std::string &o_sn /Out/);
		void getCameraNameEx(std::string &o_sn /Out/) ;
		void getCamType(std::string &o_sn /Out/) ;   // camType

		void getCDIMode(int & val /Out/);     // cdiMode
		void setCDIMode(int val);

		void getClTransferParam(std::string &o_sn /Out/) ;    // clXferPar

		void getCocRunTime(double & coc /Out/);    // CocRunTime

		void getCoolingTemperature(int & val /Out/);
		void setCoolingTemperature(int val);

		void getDoubleImageMode(int & val /Out/);
		void setDoubleImageMode(int val);

		void getFirmwareInfo(std::string &o_sn /Out/);

		void getFrameRate(double & frameRate /Out/);  // frameRate

		void getLastError(std::string &o_sn /Out/) ;   // last error
		void getLastImgAcquired(unsigned long & img /Out/);   // lastImgAcquired
		void getLastImgRecorded(unsigned long & img /Out/);   // lastImgRecorded
		void getMsgLog(std::string &o_sn /Out/);  // logMsg
		void getMsgLogEx(std::string &o_sn /Out/);  // logMsgEx
		void getPcoLogsEnabled(int & enabled /Out/);      // logPcoEnabled

		void getMaxNbImages(unsigned long & nr /Out/);     // maxNbImages

		void getPixelRate(int & val /Out/);   // pixelRate
		void setPixelRate(int val);
		void getPixelRateInfo(std::string &o_sn /Out/) ;       // pixelRateInfo
		void getPixelRateValidValues(std::string &o_sn /Out/) ;  // pixelRateValidValues

		void getRoiInfo(std::string &o_sn /Out/); 
		void getLastFixedRoi(std::string &o_sn /Out/) ;   // roiLastFixed

		void getRollingShutter(int & val /Out/);    // rollingShutter
		void setRollingShutter(int val);
		void getRollingShutterInfo(std::string &o_sn /Out/) ;       // rollingShutterInfo
        void setRollingShutterStr(std::string &i_sn); // rollingShutter string

		void getTemperatureInfo(std::string &o_sn /Out/);

		void getTraceAcq(std::string &o_sn /Out/) ;   // traceAcq

		void getVersion(std::string &o_sn /Out/) ;   // version

		void getSdkRelease(std::string &o_sn /Out/) ; // versionSdk

		void getDebugIntTypes(std::string &o_sn /Out/);
		void getDebugInt(std::string &o_sn /Out/);
		void setDebugInt(std::string &i_sn); 
		
		void setTest(std::string &i_sn) ;
		void getTest(std::string &o_sn /Out/) ;
 
		void setTimestampMode(int mode);
		void getTimestampMode(int &mode /Out/);

		void setBitAlignment(std::string &i_sn);
		void getBitAlignment(std::string &o_sn /Out/); 
		
		void getRecorderForcedFifo(int & val /Out/);
		void setRecorderForcedFifo(int val);

		void getNrEvents(int & val /Out/);
		void setNrEvents(int val);
		
		void getRecorderStopRequest(int & val /Out/);
		void setRecorderStopRequest(int val);
		
		void getLastImgFifo(int & val /Out/);
        
        void getGeneralCAPS1(std::string &o_sn  /Out/);
 
        void getParamsInfo(std::string &o_sn /Out/);


	private:
		Camera(const Pco::Camera&);
  };
};
