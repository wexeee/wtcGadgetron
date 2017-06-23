//SOSCombine.cpp

#include "SOSCombine.h"
#include "hoNDArray_math.h"

using namespace Gadgetron;

int SOSCombine::process(GadgetContainerMessage< ISMRMRD::ImageHeader>* m1,
                             GadgetContainerMessage< hoNDArray< std::complex<float> > >* m2)
{

  std::vector<size_t> dimensions, newDimensions;
  m2->getObjectPtr()->get_dimensions(dimensions);


      GDEBUG_STREAM("Image size = [" << dimensions[0] << " " << dimensions[1] << " " << dimensions[2] << "]" <<  std::endl
                    << "Channels = " << dimensions[3] << std::endl
                    << "N (sets) = " << dimensions[4] << std::endl
                    );

  newDimensions = dimensions;
  newDimensions[3] = 1;// Channels to 1

  GadgetContainerMessage< hoNDArray<std::complex<float>> >* cm2 = new GadgetContainerMessage< hoNDArray< std::complex<float>> >();

  try{cm2->getObjectPtr()->create(&newDimensions);}
  catch (std::runtime_error &err ){
    GEXCEPTION(err,"Unable to create combined image array\n");
    return GADGET_FAIL;
  }

   hoNDArray< std::complex<float> >* in = m2->getObjectPtr();
   hoNDArray< std::complex<float> >* out = cm2->getObjectPtr();
  size_t N = dimensions[4];
  size_t CHA = dimensions[3];


  //Compute d* d in place
  multiplyConj(in,in,in);

  sum_over_dimension(*in,*out,3);

  //Take the square root in place
  sqrt_inplace(out);


  // Place new image in the container of the image header
  m1->cont(NULL);
  m2->release();
  m1->cont(cm2);

  m1->getObjectPtr()->channels = 1;

  //Now pass on image
  if (this->next()->putq(m1) < 0) {
    return GADGET_FAIL;
  }

  return GADGET_OK;
}

GADGET_FACTORY_DECLARE(SOSCombine)
