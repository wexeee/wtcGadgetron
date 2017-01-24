//phasePreserveRxGadget.h

#ifndef PHASEPRESERVERXGADGET_H
#define PHASEPRESERVERXGADGET_H

#include "DREAMlib_export.h"
#include "Gadget.h"
#include "GadgetMRIHeaders.h"
#include "hoNDArray.h"
#include <complex>
#include <ismrmrd/ismrmrd.h>

namespace Gadgetron
{

class EXPORTGADGETSDREAM phasePreserveRxGadget : 
public Gadget2<ISMRMRD::ImageHeader, hoNDArray< std::complex<float> > >
{
  public:
    GADGET_DECLARE(phasePreserveRxGadget)

    GADGET_PROPERTY(level, double, "Threshold level", 1.0);

  protected:
    virtual int process( GadgetContainerMessage< ISMRMRD::ImageHeader>* m1,
                         GadgetContainerMessage< hoNDArray< std::complex<float> > >* m2);
};

}
#endif //PHASEPRESERVERXGADGET_H
