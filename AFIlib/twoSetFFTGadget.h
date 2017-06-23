#ifndef TWOSETFFTGADGET_H
#define TWOSETFFTGADGET_H

#include "Gadget.h"
#include "hoNDArray.h"
#include "gadgetron_mricore_export.h"

#include <ismrmrd/ismrmrd.h>
#include <ismrmrd/xml.h>
#include "mri_core_data.h"

namespace Gadgetron{

  class EXPORTGADGETSMRICORE twoSetFFTGadget :
  public Gadget1<IsmrmrdReconData>
    {
    public:
      GADGET_DECLARE(twoSetFFTGadget)
      twoSetFFTGadget();

    protected:
      virtual int process(GadgetContainerMessage<IsmrmrdReconData>* m1);
      long long image_counter_;
    };
}
#endif //TWOSETFFTGADGET_H
