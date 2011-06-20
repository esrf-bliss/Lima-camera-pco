#ifndef PCOINTERFACE_H
#define PCOINTERFACE_H

#include "HwInterface.h"

namespace lima
{
  namespace Pco
  {
    class Camera;
    class DetInfoCtrlObj;
    class BufferCtrlObj;
    class VideoCtrlObj;
    class SyncCtrlObj;

    class Interface : public HwInterface
    {
      DEB_CLASS_NAMESPC(DebModCamera, "Interface", "Pco");

    public:
      Interface(Camera*);
      virtual ~Interface();

      virtual void getCapList(CapList &) const;

      virtual void reset(ResetLevel reset_level);
      virtual void prepareAcq();
      virtual void startAcq();
      virtual void stopAcq();
      virtual void getStatus(StatusType& status);

      virtual int getNbAcquiredFrames();
      virtual int getNbHwAcquiredFrames();
    private:
      Camera* 		m_cam;
      DetInfoCtrlObj* 	m_det_info;
      BufferCtrlObj* 	m_buffer;
      VideoCtrlObj* 	m_video;
      SyncCtrlObj* 	m_sync;
    };

  } // namespace Pco

} // namespace lima

#endif // PCOINTERFACE_H
