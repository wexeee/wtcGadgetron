#ifndef FATOB1GADGET_H
#define FATOB1GADGET_H

#include "Gadget.h"
#include "hoNDArray.h"
#include "gadgetron_mricore_export.h"

#include <ismrmrd/ismrmrd.h>
#include <ismrmrd/xml.h>
#include "mri_core_data.h"
#include "mri_core_def.h"

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

      GADGET_PROPERTY(enableOutput,         bool,           "Switch on file output", false);
      GADGET_PROPERTY(folder,               std::string,    "Folder  for dump file", "/tmp/gadgetron");
      GADGET_PROPERTY(file_prefix,          std::string,    "Prefix for dump file", "pTxB1");

      private:
      std::string timestring_;

    };
}
#endif //FATOB1GADGET_H
