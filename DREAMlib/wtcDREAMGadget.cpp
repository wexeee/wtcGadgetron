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

    GadgetContainerMessage< hoNDArray< std::complex<float> > >* m3 =
        new GadgetContainerMessage< hoNDArray< std::complex<float> > >();

      std::vector<size_t> newImg_dims(5);
      newImg_dims[0] = m1->getObjectPtr()->matrix_size[0];
      newImg_dims[1] = m1->getObjectPtr()->matrix_size[1];
      newImg_dims[2] = m1->getObjectPtr()->matrix_size[2];
      newImg_dims[3] = m1->getObjectPtr()->channels;
      newImg_dims[4] = m1->getObjectPtr()->repetition;

      try{m3->getObjectPtr()->create(&newImg_dims);}
      catch (std::runtime_error &err){
        GEXCEPTION(err,"wtcDREAMGadget, failed to allocate new array\n");
        return -1;
      }

//    Do calculation as in matlab
//      STE = squeeze(rxcmb(:,:,:,1,:)); % Des' code suggests this way around (105 of /Users/wclarke/Documents/MATLAB/pTxPulses/b0b1tools/helpers/Tse/txmap_dream.m)
//      FID = squeeze(rxcmb(:,:,:,2,:));
//      alphaImages =  atan( sqrt( 2.* abs( STE ./ FID ) )) .* 180./pi ;
      std::complex<float>* d1 = m2->getObjectPtr()->get_data_ptr();
      std::complex<float>* d2 = m3->getObjectPtr()->get_data_ptr();

      size_t txMaxMapPos = 0;
      if (newImg_dims[4] > 1)
      {
            //Read out the value needed for the restored phase
          // Restore the phase
    //      reltxphasemap = angle( STE );
    //      pos = round(size(txmaxmap)/2);
    //      relphs = reltxphasemap - repmat( reltxphasemap(:,:,:, txmaxmap( pos(1), pos(2), pos(3) ) ), [1 1 1  imgsiz(4)] );
    //      unshimmed = alphaImages .* exp(relphs.*1i);

            txMaxMapPos = m1->getObjectPtr()->user_int[0];
      }

      size_t elements = m3->getObjectPtr()->get_number_of_elements();
      size_t elementsInRep = newImg_dims[0] * newImg_dims[1] * newImg_dims[2];
      size_t elementsInRepCon = newImg_dims[0] * newImg_dims[1] * newImg_dims[2] * 2;
      for(size_t iDx = 0; iDx < elements; iDx++)
      {
          size_t currentRep = size_t(std::floor(double(iDx)/double(elementsInRep)));
          size_t posInMainImg = iDx - (currentRep*elementsInRep);
          size_t STEOffset = posInMainImg + (currentRep*elementsInRepCon);
          size_t FIDOffset = posInMainImg + elementsInRep + (currentRep*elementsInRepCon);

          size_t STEOffsetRelPhase = posInMainImg + (txMaxMapPos*elementsInRepCon);
          float relTxPhase = float(std::arg(d1[STEOffset])) - float(std::arg(d1[STEOffsetRelPhase]));

          float flipAngle = (180.0/M_PI) * atan(std::sqrt(2.0*abs(d1[STEOffset]/d1[FIDOffset])));

          d2[iDx] = std::complex<float>(flipAngle,0.0) * exp(sqrt(std::complex<float>(-1,0)) * relTxPhase);

      }

      //Now add the new array to the outgoing message
      m1->cont(m3);
      m2->release();

      //Modify header to match
      m1->getObjectPtr()->contrast = 1;

      return this->next()->putq(m1);

}

GADGET_FACTORY_DECLARE(wtcDREAMGadget)
}

