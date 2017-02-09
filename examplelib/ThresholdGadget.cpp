//ThresholdGadget.cpp

#include "ThresholdGadget.h"

using namespace Gadgetron;

int ThresholdGadget::process(GadgetContainerMessage< ISMRMRD::ImageHeader>* m1,
                             GadgetContainerMessage< hoNDArray< std::complex<float> > >* m2)
{

  std::complex<float>* d = 
    m2->getObjectPtr()->get_data_ptr();

  unsigned long int elements =  
    m2->getObjectPtr()->get_number_of_elements();

  //First find max
  float max = 0.0;
  for (unsigned long int i = 0; i < elements; i++) {
    if (abs(d[i]) > max) {
      max = abs(d[i]);
    }
  }

  //Now threshold
  for (unsigned long int i = 0; i < elements; i++) {
    if (abs(d[i]) < level.value()*max) {
      d[i] = std::complex<float>(0.0,0.0);
    }
  }

  //Now pass on image
  if (this->next()->putq(m1) < 0) {
    return GADGET_FAIL;
  }

  return GADGET_OK;
}

GADGET_FACTORY_DECLARE(ThresholdGadget)
