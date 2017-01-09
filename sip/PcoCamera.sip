namespace Pco
{
  class Camera
  {
%TypeHeaderCode
#include "lima/HwInterface.h"
#include <PcoCamera.h>
%End
  public:

        Camera(const char*);
        ~Camera();


        void 	startAcq();
        void	reset(int);

        void getMaxWidthHeight(unsigned int& width,unsigned int& height /Out/);
        void getBytesPerPixel(unsigned int& pixbytes /Out/);
        int getNbAcquiredFrames();
        char *talk(char *cmd);       

		void getAcqTimeoutRetry(int &val /Out/);
		void setAcqTimeoutRetry(int val);

        void getAdc(int & adc /Out/);
        void setAdc(int val);
        void getAdcMax(int & adc /Out/);

		void getCamInfo(std::string &o_sn /Out/) ;
		void getCamType(std::string &o_sn /Out/) ;
		void getVersion(std::string &o_sn /Out/) ;
		void getPixelRateInfo(std::string &o_sn /Out/) ;
		void getClTransferParam(std::string &o_sn /Out/) ;
		void getLastError(std::string &o_sn /Out/) ;
		void getPixelRateValidValues(std::string &o_sn /Out/) ;
		void getTraceAcq(std::string &o_sn /Out/) ;
		
		void getCocRunTime(double & coc /Out/);
		void getFrameRate(double & frameRate /Out/);
		

		void getLastImgRecorded(int & img /Out/);
		void getLastImgAcquired(int & img /Out/);
		void getMaxNbImages(int & nr /Out/);
		void getPcoLogsEnabled(int & enabled /Out/);

		void getPixelRate(int & val /Out/);
		void setPixelRate(int val);

		void getRollingShutter(int & val /Out/);
		void setRollingShutter(int val);


		

  };
};


