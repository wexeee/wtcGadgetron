//AFIGadget.h

#ifndef SOSCombine_H
#define SOSCombine_H

#include "AFIlib_export.h"
#include "Gadget.h"
#include "GadgetMRIHeaders.h"
#include "hoNDArray.h"
#include <complex>
#include <ismrmrd/ismrmrd.h>

namespace Gadgetron
{

class EXPORTGADGETSAFI SOSCombine :
public Gadget2<ISMRMRD::ImageHeader, hoNDArray< std::complex<float> > >
{
  public:
    GADGET_DECLARE(SOSCombine)

  protected:
    virtual int process( GadgetContainerMessage< ISMRMRD::ImageHeader>* m1,
                         GadgetContainerMessage< hoNDArray< std::complex<float> > >* m2);
};

}
#endif //AFIGADGET_H
