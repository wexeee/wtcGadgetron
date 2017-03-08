//ThresholdGadget.h

#ifndef PHASEDIFFCOMBINEGADGET_H
#define PHASEDIFFCOMBINEGADGET_H

#include "b0lib_export.h"
#include "Gadget.h"
#include "GadgetMRIHeaders.h"
#include "hoNDArray.h"
#include <complex>
#include <ismrmrd/ismrmrd.h>

namespace Gadgetron
{

class EXPORTGADGETSEXAMPLE PhaseDiffCombineGadget :
public Gadget2<ISMRMRD::ImageHeader, hoNDArray< std::complex<float> > >
{
  public:
    GADGET_DECLARE(PhaseDiffCombineGadget)

  protected:
    virtual int process( GadgetContainerMessage< ISMRMRD::ImageHeader>* m1,
                         GadgetContainerMessage< hoNDArray< std::complex<float> > >* m2);
};

}
#endif //PHASEDIFFCOMBINEGADGET_H
