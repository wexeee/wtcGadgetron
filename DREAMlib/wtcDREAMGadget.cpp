#include "wtcDREAMGadget.h"
#include "hoNDObjectArray.h"
#include "hoNDArray_elemwise.h"
#include "hoNDArray_utils.h"
namespace Gadgetron{

wtcDREAMGadget::wtcDREAMGadget()
{
}

/* Take the two contrast dimension calculate the flip angle from this and return with a reduced contrasts size.*/
int wtcDREAMGadget::process( GadgetContainerMessage< ISMRMRD::ImageHeader>* m1,
                             GadgetContainerMessage< hoNDArray< std::complex<float> > >* m2)
{

    // Keep this as complex float. TO DO turn the fatob1 function into a templated gadget so that it can take either float or complex float.
    GadgetContainerMessage< hoNDArray<std::complex<float>> >* cm2 =
        new GadgetContainerMessage< hoNDArray< std::complex<float> > >();

      std::vector<size_t> newImg_dims(4);
      newImg_dims[0] = m1->getObjectPtr()->matrix_size[0];
      newImg_dims[1] = m1->getObjectPtr()->matrix_size[1];
      newImg_dims[2] = m1->getObjectPtr()->matrix_size[2];
      newImg_dims[3] = m1->getObjectPtr()->channels;

      try{cm2->getObjectPtr()->create(&newImg_dims);}
      catch (std::runtime_error &err){
        GEXCEPTION(err,"wtcDREAMGadget, failed to allocate new array\n");
        return -1;
      }

//    Do calculation as in matlab
//      STE = squeeze(rxcmb(:,:,:,1,:)); % Des' code suggests this way around (105 of /Users/wclarke/Documents/MATLAB/pTxPulses/b0b1tools/helpers/Tse/txmap_dream.m)
//      FID = squeeze(rxcmb(:,:,:,2,:));
//      alphaImages =  atan( sqrt( 2.* abs( STE ./ FID ) )) .* 180./pi ;
      std::complex<float>* d1 = m2->getObjectPtr()->get_data_ptr();
      std::complex<float>* d2 = cm2->getObjectPtr()->get_data_ptr();

      size_t elements = cm2->getObjectPtr()->get_number_of_elements();
      size_t elementsInCon = newImg_dims[0] * newImg_dims[1] * newImg_dims[2];
      for(size_t iDx = 0; iDx < elements; iDx++)
      {
          size_t posInMainImg = iDx;
          size_t STEOffset = posInMainImg;
          size_t FIDOffset = posInMainImg + elementsInCon;

          float flipAngle = (180.0/M_PI) * atan(std::sqrt(2.0*abs(d1[STEOffset]/d1[FIDOffset])));

          d2[iDx] = std::complex<float>(flipAngle,0.0);
      }

      //Now add the new array to the outgoing message
      m1->cont(cm2);
      m2->release();

      //Modify header to match
      m1->getObjectPtr()->contrast = 1;
      m1->getObjectPtr()->repetition = 1;

      return this->next()->putq(m1);

}

GADGET_FACTORY_DECLARE(wtcDREAMGadget)
}

