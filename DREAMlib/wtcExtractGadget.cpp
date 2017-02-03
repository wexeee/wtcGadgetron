/*
 * (wtc)ExtractMagnitudeGadget.cpp
 *
 *  Created on: Nov 8, 2011
 *      Author: Michael S. Hansen
 *  Modified by W Clarke to pass through metacontainers as well (see FloatToFixPointGadget.cpp).
 */

#include "GadgetIsmrmrdReadWrite.h"
#include "wtcExtractGadget.h"
#include "mri_core_data.h"

namespace Gadgetron{
wtcExtractGadget::wtcExtractGadget()
: extract_mask_(GADGET_EXTRACT_MAGNITUDE)
{

}

wtcExtractGadget::~wtcExtractGadget()
{

}

int wtcExtractGadget::process(GadgetContainerMessage<ISMRMRD::ImageHeader> *m1, GadgetContainerMessage<hoNDArray<std::complex<float> > > *m2)
{
        int em = extract_mask.value();
	if (em > 0) {
		if (em < GADGET_EXTRACT_MAX ) {
			extract_mask_ = static_cast<unsigned short>(em);
		}
	}
    // WTC modification to Extract gadget
    GadgetContainerMessage<ISMRMRD::MetaContainer>* m3 = AsContainerMessage<ISMRMRD::MetaContainer>(m2->cont());

	static int counter = 0;
	for (size_t m = GADGET_EXTRACT_MAGNITUDE; m < GADGET_EXTRACT_MAX; m = m<<1) {
		if (extract_mask_ & m) {
			GadgetContainerMessage<ISMRMRD::ImageHeader>* cm1 =
					new GadgetContainerMessage<ISMRMRD::ImageHeader>();

			//Copy the header
			*cm1->getObjectPtr() = *m1->getObjectPtr();

			GadgetContainerMessage<hoNDArray< float > > *cm2 =
					new GadgetContainerMessage<hoNDArray< float > >();

			boost::shared_ptr< std::vector<size_t> > dims = m2->getObjectPtr()->get_dimensions();

			try{cm2->getObjectPtr()->create(dims.get());}
			catch (std::runtime_error &err){
				GEXCEPTION(err,"Unable to create unsigned short storage in Extract Magnitude Gadget");
				return GADGET_FAIL;
			}

			std::complex<float>* src = m2->getObjectPtr()->get_data_ptr();
			float* dst = cm2->getObjectPtr()->get_data_ptr();

            //Copy the metacontainer
            GadgetContainerMessage<ISMRMRD::MetaContainer>* cm3 = new GadgetContainerMessage<ISMRMRD::MetaContainer>;
            *cm3->getObjectPtr() = *m3->getObjectPtr();
            cm2->cont(cm3);

			float pix_val;
			for (unsigned long i = 0; i < cm2->getObjectPtr()->get_number_of_elements(); i++) {
				switch (m) {
				case GADGET_EXTRACT_MAGNITUDE:
					pix_val = abs(src[i]);
					break;
				case GADGET_EXTRACT_REAL:
					pix_val = real(src[i]);
					break;
				case GADGET_EXTRACT_IMAG:
					pix_val = imag(src[i]);
					break;
				case GADGET_EXTRACT_PHASE:
					pix_val = arg(src[i]);
					break;
				default:
					GDEBUG("Unexpected extract mask %d, bailing out\n", m);
					return GADGET_FAIL;
				}
				dst[i] = pix_val;
			}

			cm1->cont(cm2);
			cm1->getObjectPtr()->data_type = ISMRMRD::ISMRMRD_FLOAT;//GADGET_IMAGE_REAL_FLOAT;

			switch (m) {
			case GADGET_EXTRACT_MAGNITUDE:
				cm1->getObjectPtr()->image_type = ISMRMRD::ISMRMRD_IMTYPE_MAGNITUDE;//GADGET_IMAGE_MAGNITUDE;
				break;
			case GADGET_EXTRACT_REAL:
				cm1->getObjectPtr()->image_type = ISMRMRD::ISMRMRD_IMTYPE_REAL;
				cm1->getObjectPtr()->image_series_index += 1000; //Ensure that this will go in a different series
				break;
			case GADGET_EXTRACT_IMAG:
				cm1->getObjectPtr()->image_type = ISMRMRD::ISMRMRD_IMTYPE_IMAG;
				cm1->getObjectPtr()->image_series_index += 2000; //Ensure that this will go in a different series
				break;
			case GADGET_EXTRACT_PHASE:
				cm1->getObjectPtr()->image_type = ISMRMRD::ISMRMRD_IMTYPE_PHASE;
				cm1->getObjectPtr()->image_series_index += 3000; //Ensure that this will go in a different series
				break;
			default:
				GDEBUG("Unexpected extract mask %d, bailing out\n", m);
				break;
			}


			if (this->next()->putq(cm1) == -1) {
				m1->release();
				GDEBUG("Unable to put extracted images on next gadgets queue");
				return GADGET_FAIL;
			}
		}
	}

	m1->release(); //We have copied all the data in this case
	return GADGET_OK;
}


GADGET_FACTORY_DECLARE(wtcExtractGadget)
}
