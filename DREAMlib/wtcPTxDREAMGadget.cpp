#include "wtcPTxDREAMGadget.h"
#include "hoNDObjectArray.h"
#include "hoNDArray_elemwise.h"
#include "hoNDArray_linalg.h"
#include "hoNDArray_reductions.h"
#include "ImageIOAnalyze.h"
namespace Gadgetron{

wtcPTxDREAMGadget::wtcPTxDREAMGadget()
{
}

/* In this gadget take the fourier encoded Tx channels and unmix them*/
int wtcPTxDREAMGadget::process( GadgetContainerMessage< ISMRMRD::ImageHeader>* m1,
                             GadgetContainerMessage< hoNDArray< std::complex<float> > >* m2,
                                 GadgetContainerMessage< std::vector<ISMRMRD::ImageHeader> >* m3)
{

      std::vector<size_t> newImg_dims(6);
      newImg_dims[0] = m1->getObjectPtr()->matrix_size[0];
      newImg_dims[1] = m1->getObjectPtr()->matrix_size[1];
      newImg_dims[2] = m1->getObjectPtr()->matrix_size[2];
      newImg_dims[3] = m1->getObjectPtr()->slice;
      newImg_dims[4] = m1->getObjectPtr()->channels;
      newImg_dims[5] = m1->getObjectPtr()->repetition;

      size_t txMaxMapPos = 0;
      if (newImg_dims[5] > 1)
      {
            //Read out the value needed for the restored phase
            txMaxMapPos = m1->getObjectPtr()->user_int[0];
      }

      hoNDArray<std::complex<float>> complexFlipAngles;
      complexFlipAngles.create(newImg_dims);

      hoNDArray<float> absFlipAngles;
      absFlipAngles.create(newImg_dims);

      hoNDArray<float> weightingArray;
      weightingArray.create(newImg_dims);

//    Do flip angle calculationcalculation as in matlab
//      STE = squeeze(rxcmb(:,:,:,1,:)); % Des' code suggests this way around (105 of /Users/wclarke/Documents/MATLAB/pTxPulses/b0b1tools/helpers/Tse/txmap_dream.m)
//      FID = squeeze(rxcmb(:,:,:,2,:));
//      alphaImages =  atan( sqrt( 2.* abs( STE ./ FID ) )) .* 180./pi ;
      std::complex<float>* d1 = m2->getObjectPtr()->get_data_ptr();
      std::complex<float>* d2 = complexFlipAngles.get_data_ptr();
      float* d2_abs = absFlipAngles.get_data_ptr();

      float* w = weightingArray.get_data_ptr(); // this is needed later to do the unwrapping

      size_t elements = complexFlipAngles.get_number_of_elements();
      size_t elementsInRep = newImg_dims[0] * newImg_dims[1] * newImg_dims[2]* newImg_dims[3];
      size_t elementsInRepCon = newImg_dims[0] * newImg_dims[1] * newImg_dims[2]* newImg_dims[3] * 2;
      for(size_t iDx = 0; iDx < elements; iDx++)
      {
          size_t currentRep = size_t(std::floor(double(iDx)/double(elementsInRep)));
          size_t posInMainImg = iDx - (currentRep*elementsInRep);
          size_t STEOffset = posInMainImg + (currentRep*elementsInRepCon);
          size_t FIDOffset = posInMainImg + elementsInRep + (currentRep*elementsInRepCon);

          // Restore the phase
    //      reltxphasemap = angle( STE );
    //      pos = round(size(txmaxmap)/2);
    //      relphs = reltxphasemap - repmat( reltxphasemap(:,:,:, txmaxmap( pos(1), pos(2), pos(3) ) ), [1 1 1  imgsiz(4)] );
    //      unshimmed = alphaImages .* exp(relphs.*1i);
          size_t STEOffsetRelPhase = posInMainImg + (txMaxMapPos*elementsInRepCon);
          float relTxPhase = float(std::arg(d1[STEOffset])) - float(std::arg(d1[STEOffsetRelPhase]));

          float flipAngle = (180.0/M_PI) * atan(std::sqrt(2.0*abs(d1[STEOffset]/d1[FIDOffset])));

          d2[iDx] = std::complex<float>(flipAngle,0.0) * exp(std::complex<float>(0.0,1.0) * relTxPhase);
          d2_abs[iDx] = flipAngle;

          w[iDx] = std::abs(d1[FIDOffset]);
      }

//      // exporter
//      if (m1->getObjectPtr()->slice==2)
//      {
//          hoNDArray<std::complex<float>> complexFASqueeze(complexFlipAngles);
//          complexFASqueeze.squeeze();
//          Gadgetron::ImageIOAnalyze gt_exporter;
//          gt_exporter.export_array_complex(complexFASqueeze,"/home/wtc/Documents/temp/complexFAOut");
//      }

      // Now do the channel unmixing

      hoNDArray< std::complex<float>> unmixedArray;
      size_t channelCount = 8; // To do, calculate this automatically
      std::vector<size_t> newImg_dims2(newImg_dims);
      newImg_dims2.at(5) = channelCount;

      size_t repetitions = m1->getObjectPtr()->repetition;

      try{unmixedArray.create(&newImg_dims2);}
      catch (std::runtime_error &err){
        GEXCEPTION(err,"wtcPTxDREAMGadget, failed to allocate new array\n");
        return -1;
      }

      std::complex<float>* d3 = unmixedArray.get_data_ptr();

      //Replicate the phase cycling in the dt_DREAM sequence as read from the sequence source code.
      hoNDArray<float> phaseCycling;
      phaseCycling.create(repetitions,channelCount);

      hoNDArray<std::complex<float>> phaseCyclingComplex;
      phaseCyclingComplex.create(repetitions,channelCount);
      const std::complex<float> i(0.0, 1.0);
      const std::complex<float> piDeg(M_PI/180.0,0.0);

      // Remeber the index is the other way around from matlab! But confirmed that this is the correct calculations
      for (size_t cDx =0; cDx < channelCount; cDx++)
      {
          for (size_t rDx =0; rDx < repetitions; rDx++) // Usually 0 to 15
          {
              unsigned long dRFTxCh = cDx%channelCount;
              float dRFPhaseCycle = ( 360.0*(float(dRFTxCh)+1.0)*float(rDx)/float(repetitions) );
              size_t currIndex = rDx +  (cDx* repetitions);
              phaseCycling[currIndex] = dRFPhaseCycle - std::floor( dRFPhaseCycle / 360.0 ) * 360.0;
              phaseCyclingComplex[currIndex]
                      = std::exp( std::complex<float>(phaseCycling[currIndex],0.0) * i * piDeg);
          }
      }

      // Work out Des' weighting scheme
//      pinvWeight = dreamWeight(image_data_rOS,[8 5]);
//      p = [8 5];
//      w = abs(FID);
//      w = w./max(w(:));
//      pinvWeightRxComb = 1./(1 + exp(-p(2).*p(1).*(w-1./p(1))));
      float p1 = weight1.value();
      float p2 = weight2.value();
      hoNDArray<float> pinvWeightRxComb;
      pinvWeightRxComb.create(newImg_dims); // Has image x repetitions dimensions
      float wMax;
      maxValue(weightingArray,wMax);
      for(size_t iDx = 0; iDx < pinvWeightRxComb.get_number_of_elements(); iDx++)
      {
        float wNorm = w[iDx]/wMax;
        pinvWeightRxComb[iDx] = 1.0/(1.0 + std::exp(-1*p2*p1*(wNorm-1.0/p1)));
      }

      hoNDArray<std::complex<float>> txmod;
      txmod.create(repetitions,channelCount); // txmod is 16 x 8
     // r = conj(x)
      conjugate(phaseCyclingComplex,txmod);

//      % imgsiz = size(alphaImages);
//      % tximg=zeros([prod(imgsiz(1:3)) size(txmod,2)])';
//      % c = ones([prod(imgsiz(1:3)) 1]);
//      % img = reshape(alphaImages, prod(imgsiz(1:3)), imgsiz(4))';
//      % w   = reshape(pinvWeight,   prod(imgsiz(1:3)), imgsiz(4));
//      %
//      %  for iDx = 1:size(tximg,2)
//      %     tximg(:,iDx) = inv(txmod'*diag(w(iDx,:))*txmod)*txmod'*diag(w(iDx,:))*img(:,iDx);
//      %     c(iDx) = cond(inv(txmod'*diag(w(iDx,:))*txmod)*txmod'*diag(w(iDx,:)));
//      %  end
//      %
//      %  tximg = tximg';
//      % tximg = reshape(tximg,[imgsiz(1:3) size(txmod,2)]);

      // Loop over all elements excluding the repetition dimension
      elements = newImg_dims2[0]*newImg_dims2[1]*newImg_dims2[2]*newImg_dims2[3]; // We can also miss off the Rx channels as there is only 1 by now
        for (size_t iDx = 0; iDx < elements; iDx++)
        {
            hoNDArray<std::complex<float>> diagW;
            diagW.create(repetitions,repetitions);
            diagW.fill(std::complex<float>(0,0)); // Fill with zeros so off-diagonals are sensible.
            hoNDArray<std::complex<float>> img;
            img.create(1,repetitions);

            hoNDArray<std::complex<float>> repByCha;
            repByCha.create(repetitions,channelCount);
            hoNDArray<std::complex<float>> chaByCha;
            chaByCha.create(channelCount,channelCount);
            hoNDArray<std::complex<float>> repBy1;
            repBy1.create(repetitions,1);
            hoNDArray<std::complex<float>> chaBy1;
            chaBy1.create(channelCount,1);
            hoNDArray<std::complex<float>> finalOut;
            finalOut.create(channelCount,1);


            for (size_t rDx = 0; rDx < repetitions; rDx++)
            {
                diagW[rDx+(rDx*repetitions)] = pinvWeightRxComb[iDx+(rDx*elements)];
                //diagW[rDx+(rDx*repetitions)] = std::complex<float>(1.0,0.0); // Replace weighting with identity matrix for now
                img[rDx] = d2[iDx+(rDx*elements)];
            }

            //txmod'*diag(w(iDx,:))*txmod
             gemm(repByCha,diagW,txmod); // 16x16 * 16x8 = 16x8
             gemm(chaByCha,txmod,true,repByCha,false); // transpose the same version (as this applies a conjugate transpose!!!) so 8x16 * 16x8 = 8x8

             // invert
             hoNDArray<std::complex<float>> inverted(chaByCha);
             getri(inverted);

             //txmod'*diag(w(iDx,:))*img(:,iDx)
            gemm(repBy1,diagW,false,img,true); // 16x16 * 16x1 = 16x1
            gemm(chaBy1,txmod,true,repBy1,false); // transpose the same version (as this applies a conjugate transpose!!!)so 8x16 * 16x1 = 8x1
            //inverted * part2
            gemm(finalOut,inverted,chaBy1); // 8x8 * 8x1 = 8x1

            for (size_t cDx = 0; cDx < channelCount; cDx++)
            {
                d3[iDx+(cDx*elements)] = finalOut[cDx];
            }

        }

      std::vector<size_t> output_dims(5);
      output_dims[0] = m1->getObjectPtr()->matrix_size[0];
      output_dims[1] = m1->getObjectPtr()->matrix_size[1];
      output_dims[2] = m1->getObjectPtr()->matrix_size[2];
      output_dims[3] = m1->getObjectPtr()->channels;
      output_dims[4] = channelCount;

      // finally sort out the mess of slices into multiple images
      for (size_t sDx = 0; sDx <  m1->getObjectPtr()->slice; sDx++)
      {
          //Create a new image
          GadgetContainerMessage<ISMRMRD::ImageHeader>* cm1 =
                  new GadgetContainerMessage<ISMRMRD::ImageHeader>();

          //Copy the header from those stored in the third level container
          ISMRMRD::ImageHeader currSliceHeader = m3->getObjectPtr()->at(sDx);
          *(cm1->getObjectPtr()) = currSliceHeader;

          GadgetContainerMessage< hoNDArray< std::complex<float> > >* cm2 =
                  new GadgetContainerMessage<hoNDArray< std::complex<float> > >();
          cm1->cont(cm2);

          try{cm2->getObjectPtr()->create(&output_dims);}
          catch (std::runtime_error &err){
              GEXCEPTION(err,"Unable to allocate new image array\n");
              cm1->release();
              return GADGET_FAIL;
          }

          std::vector<size_t> subArrayStart(6);
          subArrayStart[0] = 0;
          subArrayStart[1] = 0;
          subArrayStart[2] = 0;
          subArrayStart[3] = sDx;
          subArrayStart[4] = 0;
          subArrayStart[5] = 0;

          std::vector<size_t> subArraySize(newImg_dims2);
          subArraySize[3] = 1;

          unmixedArray.get_sub_array(subArrayStart, subArraySize, *(cm2->getObjectPtr()));
          std::vector<size_t> reshapeVec(*(cm2->getObjectPtr()->get_dimensions()));
          reshapeVec.erase(reshapeVec.begin()+3);
          cm2->getObjectPtr()->reshape(reshapeVec);

          // modify header values
          cm1->getObjectPtr()->repetition = channelCount; // Gone from 16 phase encodes to 8 Tx channels
          cm1->getObjectPtr()->contrast = 0; // From a FID and STE to FA contrast
          cm1->getObjectPtr()->channels = 1; // From 32 Rx channels to a combined image.

          if (this->next()->putq(cm1) < 0) {
              return GADGET_FAIL;
          }

      }

      //Now add the new array to the outgoing message
      m1->release();

      return GADGET_OK;

}

GADGET_FACTORY_DECLARE(wtcPTxDREAMGadget)
}

