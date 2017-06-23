//AFIGadget.cpp

#include "AFIGadget.h"

using namespace Gadgetron;

int AFIGadget::process(GadgetContainerMessage< ISMRMRD::ImageHeader>* m1,
                             GadgetContainerMessage< hoNDArray< std::complex<float> > >* m2)
{

  std::vector<size_t> dimensions, newDimensions;
  m2->getObjectPtr()->get_dimensions(dimensions);


      GDEBUG_STREAM("Image size = [" << dimensions[0] << " " << dimensions[1] << " " << dimensions[2] << "]" <<  std::endl
                    << "Channels = " << dimensions[3] << std::endl
                    << "N (sets) = " << dimensions[4] << std::endl
                    );

  newDimensions = dimensions;
  newDimensions.pop_back();// Remove the S dimension
  newDimensions.pop_back();// Remove the sets dimension

  GadgetContainerMessage< hoNDArray<std::complex<float>> >* cm2 = new GadgetContainerMessage< hoNDArray< std::complex<float>> >();

  try{cm2->getObjectPtr()->create(&newDimensions);}
  catch (std::runtime_error &err ){
    GEXCEPTION(err,"Unable to create combined image array\n");
    return GADGET_FAIL;
  }

  std::complex<float>* in = m2->getObjectPtr()->get_data_ptr();
  std::complex<float>* out = cm2->getObjectPtr()->get_data_ptr();

  float n = 125.0/25.0; // TO DO create this automatically
  size_t elements = dimensions[0]*dimensions[1]*dimensions[2]; // Loop over spatial points
  for (size_t iDx = 0; iDx < elements; iDx++)
  {
      size_t index1 = iDx ;
      size_t index2 = iDx + (elements*(dimensions[4]-1));

      float r = std::abs(in[index2])/std::abs(in[index1]);

      out[iDx] = std::real((180/M_PI)*std::acos((n*r-1)/(n-r)));
  }

  // Place new image in the container of the image header
  m1->cont(NULL);
  m2->release();
  m1->cont(cm2);

  m1->getObjectPtr()->contrast = 1; // TOuching this up as a leftover from the fft function
  m1->getObjectPtr()->set = 1;
  //m1->getObjectPtr()->data_type = ISMRMRD::ISMRMRD_FLOAT;

  //Now pass on image
  if (this->next()->putq(m1) < 0) {
    return GADGET_FAIL;
  }

  return GADGET_OK;
}

GADGET_FACTORY_DECLARE(AFIGadget)
