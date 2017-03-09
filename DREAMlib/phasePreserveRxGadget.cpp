//phasePreserveRxGadget.cpp
// This gadget should do a recieve coil combination whilst preserving all other dimensions.

#include "phasePreserveRxGadget.h"
#include "hoNDFFT.h"
#include "GrappaUnmixingGadget.h"
#include "hoNDArray_elemwise.h"
#include "hoNDArray_utils.h"
#include "ImageIOAnalyze.h"
using namespace Gadgetron;

int phasePreserveRxGadget::process(GadgetContainerMessage< ISMRMRD::ImageHeader>* m1,
                             GadgetContainerMessage< hoNDArray< std::complex<float> > >* m2)
{
    // wtc, expecting 3D (including slices) images with contrasts and repetitions dimentions as well as the normal 4 (3rd [2nd PE] will be singleton).
    std::vector<size_t> dimensions;
    m2->getObjectPtr()->get_dimensions(dimensions);

//    Gadgetron::ImageIOAnalyze gt_exporter2;
//    gt_exporter2.export_array_complex(*(m2->getObjectPtr()),"/media/sf_Data/20170308_MoreB1Maps/Debug/rxuncombined");
//    GDEBUG_STREAM("Image size = [" << dimensions[0] << " " << dimensions[1] << " " << dimensions[2] << "]" <<  std::endl
//                  << "Slices = " << dimensions[3] << std::endl
//                  << "Channels = " << dimensions[4] << std::endl
//                  << "N (contrasts) = " << dimensions[5] << std::endl
//                  << "S (repetitions) = " << dimensions[6]<< std::endl
//                 );

    size_t channelDim = dimensions[4];
    size_t contrastsDim = dimensions[5];
    size_t repetitionsDim = dimensions[6];

    //Create output, the size of output should be [E0 E1 E2 SLC CHA = 1 N S]
    std::vector<size_t> dimensionsOut;
    dimensionsOut = dimensions;
    dimensionsOut[4]= 1;//channel dimension (5th) = 1

    //Create a new image
    GadgetContainerMessage<ISMRMRD::ImageHeader>* cm1 =
            new GadgetContainerMessage<ISMRMRD::ImageHeader>();

    //Copy the header
    *cm1->getObjectPtr() = *m1->getObjectPtr();

    GadgetContainerMessage< hoNDArray< std::complex<float> > >* cm2 =
            new GadgetContainerMessage<hoNDArray< std::complex<float> > >();
    cm1->cont(cm2);

    cm1->getObjectPtr()->channels = 1;

    try{cm2->getObjectPtr()->create(&dimensionsOut);}
    catch (std::runtime_error &err ){
      GEXCEPTION(err,"Unable to create combined image array\n");
      return GADGET_FAIL;
    }    

    // Move anything being contained in m2 to cm2
    cm2->cont(m2->cont());
    m2->cont(NULL);

    // Sum over the channels and then the N (contrasts) dimension.
    // Then find max in the S (repetition) dimension
    std::vector<size_t> dimensionsSum = dimensions;

    hoNDArray<float> absImg;
    absImg.create(&dimensionsSum);

    hoNDArray<float> sumOverCon;
    sumOverCon.create(&dimensionsSum);

    dimensionsSum.erase(dimensionsSum.begin()+5);
    hoNDArray<float> sumOverChaCon;
    sumOverChaCon.create(&dimensionsSum);

    std::vector<size_t> dimensionsMax = dimensions;
    dimensionsMax.pop_back();//remove the repetitions (S) dimension (end)

    hoNDArray<size_t> txMaxMapFull;
    txMaxMapFull.create(&dimensionsMax);

    dimensionsMax.pop_back();dimensionsMax.pop_back();//remove the contrasts (S) and channels dimension (end)
    hoNDArray<size_t> txMaxMap;
    txMaxMap.create(&dimensionsMax);

    Gadgetron::abs(*(m2->getObjectPtr()),absImg);
    Gadgetron::sum_over_dimension(absImg,sumOverCon,size_t(5)); //sum over contrasts
    Gadgetron::sum_over_dimension(sumOverCon,sumOverChaCon,size_t(4)); //sum over channels

    // Find maximum index in the final S (repetition) dimension)
    unsigned long int elements = txMaxMap.get_number_of_elements();

    // find max index in the repetition dimension.
      for (unsigned long int iDx = 0; iDx < elements; iDx++)
      {
          float maxValue =0.0;
          size_t maxInd = 0;

          for(size_t rr = 0; rr < repetitionsDim; rr++)
          {
              size_t currentIndex = (elements*rr)+iDx;
            if (abs(sumOverChaCon[currentIndex]) > maxValue)
            {
              maxValue = abs(sumOverChaCon[currentIndex]);
              maxInd = rr;
            }
          }
          txMaxMap[iDx] = maxInd;
      }

      // Now copy the values in the three four dimensions of txMaxMap out to cover the whole array, i.e. into the channels and contrasts dimenstions
      for(size_t cDx = 0; cDx < dimensions[4]; cDx++)
      {
          for(size_t nDx = 0; nDx < dimensions[5]; nDx++)
          {
              size_t copyOffset = txMaxMap.get_number_of_elements()*(cDx+nDx*dimensions[4]);
              memcpy(txMaxMapFull.begin()+copyOffset ,txMaxMap.begin(),txMaxMap.get_number_of_elements()*sizeof(size_t));
          }
      }

      std::vector<size_t> dims_ccurMax  = dimensions;

      hoNDArray<std::complex<float>> ccurMax;
      ccurMax.create(&dims_ccurMax);

      dims_ccurMax.erase(dims_ccurMax.begin()+4); // will be summed over channels
      hoNDArray<std::complex<float>> ccurMaxSum;
      ccurMaxSum.create(&dims_ccurMax);

      // fill all elements of the first index of the last dimension
      elements = ccurMax.get_number_of_elements();
      size_t elementsPerRep = elements/repetitionsDim;
      for (size_t iDx = 0; iDx < elements; iDx++ )
      {
          size_t currentRep = size_t(std::floor(double(iDx)/double(elementsPerRep)));
          size_t baseIndex = iDx - (currentRep*elementsPerRep);
          size_t thisIndex = baseIndex + (txMaxMapFull(baseIndex)*elementsPerRep);
          ccurMax[iDx] = m2->getObjectPtr()->at(thisIndex);
      }

    //ccurrmax= exp(1i.*-angle(ccurrmax));
    //rxcmb = squeeze(sum(image_data_rOS.*ccurrmax, 5));
    elements = ccurMax.get_number_of_elements();
    for (size_t iDx = 0; iDx < elements; iDx++ )
    {
        std::complex<float> complexAngle = std::exp(float(-1.0)*std::complex<float>(0.0,1) * std::arg(ccurMax[iDx]));
        ccurMax[iDx] = m2->getObjectPtr()->at(iDx) * complexAngle;
    }

    sum_over_dimension(ccurMax,ccurMaxSum,size_t(4)); //sum over channels
    //ccurMax.squeeze();

    //copy into the output
    memcpy(cm2->getObjectPtr()->begin(),ccurMaxSum.begin(),ccurMaxSum.get_number_of_elements()*sizeof(std::complex<float>));

    std::vector<size_t> dimensions2;
    cm2->getObjectPtr()->get_dimensions(dimensions2);

    // This information is required later in the chain store it in the headers somewhere
    // pos = round(size(txmaxmap)/2);
    // txmaxmap( pos(1), pos(2), pos(3) )
    std::vector<size_t> pos;
    for(std::vector<size_t>::iterator it = dimensionsMax.begin(); it != dimensionsMax.end(); ++it)
    {
        pos.push_back(size_t(std::round(double(*it)/2)));
    }

    //size_t txMaxMapStore = txMaxMap(pos[0],pos[1],pos[2]);

//    GDEBUG_STREAM("pos Image size = [" << pos[0] << " " << pos[1] << " " << pos[3] << "]" <<  std::endl
//                     << "txMaxMap(pos[0],pos[1],pos[3]) = " << txMaxMap(pos[0],pos[1],pos[3]) << std::endl);

    cm1->getObjectPtr()->user_int[0] = int32_t(txMaxMap(pos[0],pos[1],pos[3]));

//              Gadgetron::ImageIOAnalyze gt_exporter;
//              gt_exporter.export_array_complex(*(cm2->getObjectPtr()),"/media/sf_Data/20170308_MoreB1Maps/Debug/rxcombined");


  //Now pass on image
  if (this->next()->putq(cm1) < 0) {
    return GADGET_FAIL;
  }

  return GADGET_OK;
}

GADGET_FACTORY_DECLARE(phasePreserveRxGadget)
