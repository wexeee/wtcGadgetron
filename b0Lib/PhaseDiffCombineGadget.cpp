//PhaseDiffCombineGadget.cpp

#include "PhaseDiffCombineGadget.h"
#include "robustunwrap.h"

using namespace Gadgetron;

int PhaseDiffCombineGadget::process(GadgetContainerMessage< ISMRMRD::ImageHeader>* m1,
                             GadgetContainerMessage< hoNDArray< std::complex<float> > >* m2)
{

  std::complex<float>* src =
    m2->getObjectPtr()->get_data_ptr();

  std::vector<size_t> dimensions, newDimensions;
  m2->getObjectPtr()->get_dimensions(dimensions);

  newDimensions = dimensions;
  newDimensions.pop_back(); // Remove the contrasts dimensions
  newDimensions.pop_back(); // Remove the channels dimensions

  GadgetContainerMessage< hoNDArray<float> >* cm2 = new GadgetContainerMessage< hoNDArray< float> >();

  try{cm2->getObjectPtr()->create(&newDimensions);}
  catch (std::runtime_error &err ){
    GEXCEPTION(err,"Unable to create combined image array\n");
    return GADGET_FAIL;
  }


  hoNDArray<float> phase;
  phase.create(&newDimensions);
  hoNDArray<float> magnitude;
  magnitude.create(&newDimensions);

  float* phs = phase.get_data_ptr();
  float* mag = magnitude.get_data_ptr();
  float* out = cm2->getObjectPtr()->get_data_ptr();

  size_t elements = dimensions[0]*dimensions[1]*dimensions[2]; // Loop over spatial points
  for (size_t iDx = 0; iDx < elements; iDx++)
  {
      std::complex<float> combined(0.0,0.0);
      for (size_t cDx = 0; cDx < dimensions[3]; cDx++) //Loop over coils
      {
        size_t firstEchoIndex = iDx+(elements*cDx);
        size_t secondEchoIndex = iDx +(elements*cDx) + (elements*dimensions[3]);

        combined += src[firstEchoIndex] * std::conj(src[secondEchoIndex]);

      }
    phs[iDx] = std::atan2(combined.imag(),combined.real());
    mag[iDx] = std::abs(std::sqrt(combined));
  }

  // Now do the robust unwrapping
  int numunwrapbins = 10000;
  robustUnwrapMain(newDimensions, phs, mag, out, numunwrapbins);

  m1->cont(NULL);
  m2->release();
  m1->cont(cm2);

  m1->getObjectPtr()->channels = 1;
  m1->getObjectPtr()->contrast = 1;
  m1->getObjectPtr()->data_type = ISMRMRD::ISMRMRD_FLOAT;
  m1->getObjectPtr()->image_type = ISMRMRD::ISMRMRD_IMTYPE_PHASE;

  //Now pass on image
  if (this->next()->putq(m1) < 0) {
    return GADGET_FAIL;
  }

  return GADGET_OK;
}

GADGET_FACTORY_DECLARE(PhaseDiffCombineGadget)
