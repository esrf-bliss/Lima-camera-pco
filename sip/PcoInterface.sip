namespace Pco
{
  class Interface : HwInterface
  {
%TypeHeaderCode
#include <PcoInterface.h>
%End
  
  public:

    Interface(Pco::Camera* /KeepReference/);
    virtual ~Interface();

    virtual void getCapList(std::vector<HwCap> &cap_list /Out/) const;
    virtual void reset(ResetLevel reset_level);
    virtual void prepareAcq();
    virtual void startAcq();
    virtual void stopAcq();

    virtual void getStatus(StatusType& status /Out/);

    virtual int getNbAcquiredFrames();
    virtual int getNbHwAcquiredFrames();
    
  private:
     Interface(const Interface&);
  };
  
  
  class RoiCtrlObj : HwRoiCtrlObj
  {

  %TypeHeaderCode
  #include "PcoInterface.h"
  #include "PcoRoiCtrlObj.h"
  %End

  public:
    RoiCtrlObj(Pco::Camera *cam);
    virtual ~RoiCtrlObj();

    virtual void checkRoi(const Roi& set_roi, Roi& hw_roi /Out/);
    virtual void setRoi(const Roi& set_roi);
    virtual void getRoi(Roi& hw_roi /Out/);
    
  private:
     RoiCtrlObj(const RoiCtrlObj&);

  };
};
