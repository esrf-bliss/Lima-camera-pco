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

  };
};


