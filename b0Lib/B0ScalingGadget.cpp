//B0ScalingGadget.cpp

#include "B0ScalingGadget.h"
#include "mri_core_def.h"
#include <ismrmrd/xml.h>

using namespace Gadgetron;

int B0ScalingGadget::process_config(ACE_Message_Block* mb)
{
    ISMRMRD::IsmrmrdHeader hdr;
    ISMRMRD::deserialize(mb->rd_ptr(), hdr);

    if (hdr.sequenceParameters.is_present()) {
        if (hdr.sequenceParameters->TE.is_present()) {
            for (auto& te: *(hdr.sequenceParameters->TE)) {
                this->echoTimes_.push_back(te);
            }
        } else {
            GERROR("No echo times found in sequence parameters\n");
            return GADGET_FAIL;
        }
    } else {
        GERROR("Sequence parameters are required to do water fat seperations\n");
        return GADGET_FAIL;
    }

    for (auto& te: echoTimes_) {
        GDEBUG("Echo time: %f\n", te);
    }


    if (hdr.acquisitionSystemInformation.is_present() && hdr.acquisitionSystemInformation->systemFieldStrength_T.is_present()) {
        this->fieldStrength_ = *(hdr.acquisitionSystemInformation->systemFieldStrength_T);
        GDEBUG("Field strength: %f\n", this->fieldStrength_);
    } else {
        GERROR("Field strength not defined. Required for fat-water seperation\n");
        return GADGET_FAIL;
    }

     this->LarmorFrequencyHz_ = hdr.experimentalConditions.H1resonanceFrequency_Hz;
     GDEBUG("Larmor Frequency (Hz): %f\n", this->LarmorFrequencyHz_);

    return GADGET_OK;
}

int B0ScalingGadget::process(GadgetContainerMessage< ISMRMRD::ImageHeader>* m1,
                             GadgetContainerMessage< hoNDArray< float > >* m2)
{

    // Implement these couple of lines
    // turn into field units (T)
    //dte = diff(echotime)./1e6;
    //b0 = -phd./(theData.hdr.Meas.alLarmorConstant(1).*2.*pi)./dte;

  float* d = m2->getObjectPtr()->get_data_ptr();

  unsigned long int elements =  
    m2->getObjectPtr()->get_number_of_elements();

  float dte = (echoTimes_[1] - echoTimes_[0])/1000.0;
  float scalingFactor = (LarmorFrequencyHz_/fieldStrength_) * 2 * M_PI * dte;

  for (unsigned long int i = 0; i < elements; i++)
  {
      d[i] = d[i]/scalingFactor;
  }

  m1->getObjectPtr()->image_type = ISMRMRD::ISMRMRD_IMTYPE_MAGNITUDE;

  //Now pass on image
  if (this->next()->putq(m1) < 0) {
    return GADGET_FAIL;
  }

  return GADGET_OK;
}

GADGET_FACTORY_DECLARE(B0ScalingGadget)
