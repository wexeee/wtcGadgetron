#ifndef FATOB1GADGET_H
#define FATOB1GADGET_H

#include "Gadget.h"
#include "hoNDArray.h"
#include "gadgetron_mricore_export.h"

#include <ismrmrd/ismrmrd.h>
#include <ismrmrd/xml.h>
#include "mri_core_data.h"

namespace Gadgetron{

  class EXPORTGADGETSMRICORE faToB1Gadget :
  public Gadget2<ISMRMRD::ImageHeader, hoNDArray< std::complex<float> > >
    {
    public:
      GADGET_DECLARE(faToB1Gadget)
      faToB1Gadget();

    protected:
      virtual int process_config(ACE_Message_Block* mb);
      virtual int process(GadgetContainerMessage< ISMRMRD::ImageHeader>* m1,
                          GadgetContainerMessage< hoNDArray< std::complex<float> > >* m2);
      float rfvolt_;
    };
}
#endif //FATOB1GADGET_H
