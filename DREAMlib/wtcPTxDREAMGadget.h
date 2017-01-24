#ifndef WTCPTXDREAMGADGET_H
#define WTCPTXDREAMGADGET_H

#include "Gadget.h"
#include "hoNDArray.h"
#include "gadgetron_mricore_export.h"

#include <ismrmrd/ismrmrd.h>
#include <ismrmrd/xml.h>
#include "mri_core_data.h"

namespace Gadgetron{

  class EXPORTGADGETSMRICORE wtcPTxDREAMGadget :
  public Gadget2<ISMRMRD::ImageHeader, hoNDArray< std::complex<float> > >
    {
    public:
      GADGET_DECLARE(wtcPTxDREAMGadget)
      wtcPTxDREAMGadget();

    protected:
      virtual int process(GadgetContainerMessage< ISMRMRD::ImageHeader>* m1,
                          GadgetContainerMessage< hoNDArray< std::complex<float> > >* m2);

    };
}
#endif //WTCPTXDREAMGADGET_H
