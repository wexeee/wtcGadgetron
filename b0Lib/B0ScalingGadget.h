//B0ScalingGadget.h

#ifndef B0SCALINGGADGET_H
#define B0SCALINGGADGET_H

#include "b0lib_export.h"
#include "Gadget.h"
#include "GadgetMRIHeaders.h"
#include "hoNDArray.h"
#include <complex>
#include <ismrmrd/ismrmrd.h>

namespace Gadgetron
{

class EXPORTGADGETSEXAMPLE B0ScalingGadget :
public Gadget2<ISMRMRD::ImageHeader, hoNDArray< float > >
{
  public:
    GADGET_DECLARE(B0ScalingGadget)


  protected:
    GADGET_PROPERTY(enableOutput,         bool,           "Switch on file output", false);
    GADGET_PROPERTY(folder,               std::string,    "Folder  for dump file", "/tmp/gadgetron");
    GADGET_PROPERTY(file_prefix,          std::string,    "Prefix for dump file", "pTxB0");

    virtual int process_config(ACE_Message_Block* mb);
    virtual int process( GadgetContainerMessage< ISMRMRD::ImageHeader>* m1,
                         GadgetContainerMessage< hoNDArray< float > >* m2);

  private:
     std::vector<float> echoTimes_;
     float LarmorFrequencyHz_;
     float fieldStrength_;
};

}
#endif //B0SCALINGGADGET_H
