//AFIGadget.h

#ifndef AFIGADGET_H
#define AFIGADGET_H

#include "AFIlib_export.h"
#include "Gadget.h"
#include "GadgetMRIHeaders.h"
#include "hoNDArray.h"
#include <complex>
#include <ismrmrd/ismrmrd.h>

namespace Gadgetron
{

class EXPORTGADGETSAFI AFIGadget :
public Gadget2<ISMRMRD::ImageHeader, hoNDArray< std::complex<float> > >
{
  public:
    GADGET_DECLARE(AFIGadget)

    GADGET_PROPERTY(AFI_N, double, "Ratio TR", 5.0);

  protected:
    virtual int process( GadgetContainerMessage< ISMRMRD::ImageHeader>* m1,
                         GadgetContainerMessage< hoNDArray< std::complex<float> > >* m2);
};

}
#endif //AFIGADGET_H
