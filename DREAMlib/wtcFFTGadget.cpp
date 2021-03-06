#include "wtcFFTGadget.h"
#include "hoNDFFT.h"
#include "hoNDArray_utils.h"

namespace Gadgetron{

wtcFFTGadget::wtcFFTGadget()
  :image_counter_(0)
{

}

int wtcFFTGadget::process( GadgetContainerMessage<IsmrmrdReconData>* m1)
{

    //Iterate over all the recon bits
    for(std::vector<IsmrmrdReconBit>::iterator it = m1->getObjectPtr()->rbit_.begin();
        it != m1->getObjectPtr()->rbit_.end(); ++it)
    {
        //Grab a reference to the buffer containing the imaging data
        IsmrmrdDataBuffered & dbuff = it->data_;

        //7D, fixed order [E0, E1, E2, CHA, N, S, SLC, LOC]
        uint16_t E0 = dbuff.data_.get_size(0);
        uint16_t E1 = dbuff.data_.get_size(1);
        uint16_t E2 = dbuff.data_.get_size(2);
        uint16_t CHA = dbuff.data_.get_size(3);
        uint16_t N = dbuff.data_.get_size(4);
        uint16_t S = dbuff.data_.get_size(5);
        uint16_t SLC = dbuff.data_.get_size(6);


        //Image will be [E0,E1,E2,CHA,N,S,SLC] big
        std::vector<size_t> img_dims(7);
        img_dims[0] = E0;
        img_dims[1] = E1;
        img_dims[2] = E2;
        img_dims[3] = SLC;
        img_dims[4] = CHA;
        img_dims[5] = N;
        img_dims[6] = S;

        std::vector<size_t> img_dims2(7);
        img_dims2[0] = E0;
        img_dims2[1] = E1;
        img_dims2[2] = E2;
        img_dims2[3] = CHA;
        img_dims2[4] = N;
        img_dims2[5] = S;
        img_dims2[6] = SLC;

        size_t numberOfSlices = SLC;

        // Calculate the vector to sort the centricly ordered slices.
        size_t endOfLoop = std::ceil(float(numberOfSlices)/2.0);
        std::vector<size_t> sortVector;
        for (size_t iDx = 0; iDx < endOfLoop; iDx++)
        {
            if (numberOfSlices % 2 == 0) // Is even
            {
                if(sortVector.size()<numberOfSlices)
                {
                    sortVector.push_back(endOfLoop+iDx);
                }
                sortVector.push_back(iDx);
            }else{
                sortVector.push_back(iDx);
                if(sortVector.size()<numberOfSlices)
                {
                    sortVector.push_back(endOfLoop+iDx);
                }
            }
        }

        // Create temparary array
        hoNDArray< std::complex<float>> tmpArray;
        tmpArray.create(img_dims2);

        //Create a new image
        GadgetContainerMessage<ISMRMRD::ImageHeader>* cm1 =
                new GadgetContainerMessage<ISMRMRD::ImageHeader>();
        GadgetContainerMessage< hoNDArray< std::complex<float> > >* cm2 =
                new GadgetContainerMessage<hoNDArray< std::complex<float> > >();
        GadgetContainerMessage<std::vector<ISMRMRD::ImageHeader>>* cm3 =
                new GadgetContainerMessage<std::vector<ISMRMRD::ImageHeader>>();
        cm1->cont(cm2);
        cm2->cont(cm3);

        //TODO do we want an image attribute string?
        try{cm2->getObjectPtr()->create(&img_dims);}
        catch (std::runtime_error &err)
        {
            GEXCEPTION(err,"Unable to allocate new image array\n");
            cm1->release();
            return GADGET_FAIL;
        }

        //Set some information into the image header
        //Use the middle header for some info
//        ISMRMRD::AcquisitionHeader & acqhdr = dbuff.headers_(dbuff.sampling_.sampling_limits_[1].center_,
//                dbuff.sampling_.sampling_limits_[2].center_,
//                0, 0, numberOfSlices-1);

//        cm1->getObjectPtr()->matrix_size[0]     = E0;
//        cm1->getObjectPtr()->matrix_size[1]     = E1;
//        cm1->getObjectPtr()->matrix_size[2]     = E2;
//        cm1->getObjectPtr()->field_of_view[0]   = dbuff.sampling_.recon_FOV_[0];
//        cm1->getObjectPtr()->field_of_view[1]   = dbuff.sampling_.recon_FOV_[1];
//        cm1->getObjectPtr()->field_of_view[2]   = dbuff.sampling_.recon_FOV_[2];
//        cm1->getObjectPtr()->channels           = CHA;

//        cm1->getObjectPtr()->average = acqhdr.idx.average;
//        cm1->getObjectPtr()->slice = SLC;
//        cm1->getObjectPtr()->contrast = N; // This isn't the index but rather the size! Use this until I can find something better.
//        cm1->getObjectPtr()->phase = acqhdr.idx.phase;
//        cm1->getObjectPtr()->repetition = S; // This isn't the index but rather the size!
//        cm1->getObjectPtr()->set = acqhdr.idx.set;
//        cm1->getObjectPtr()->acquisition_time_stamp = acqhdr.acquisition_time_stamp;

//        memcpy(cm1->getObjectPtr()->position, acqhdr.position, sizeof(float)*3);
//        memcpy(cm1->getObjectPtr()->read_dir, acqhdr.read_dir, sizeof(float)*3);
//        memcpy(cm1->getObjectPtr()->phase_dir, acqhdr.phase_dir, sizeof(float)*3);
//        memcpy(cm1->getObjectPtr()->slice_dir, acqhdr.slice_dir, sizeof(float)*3);
//        memcpy(cm1->getObjectPtr()->patient_table_position, acqhdr.patient_table_position, sizeof(float)*3);
//        cm1->getObjectPtr()->data_type = ISMRMRD::ISMRMRD_CXFLOAT;
//        cm1->getObjectPtr()->image_index = 0;

        //Loop over Slices to reorder and to record each slice's header in cm3
        for (uint16_t slc=0; slc < SLC; slc++) {

            // Sort the order
            size_t slcToUse = sortVector[slc];
            //Copy the 6D data block [E0,E1,E2,CHA,N,S] for this loc into the output image            
            memcpy(tmpArray.begin()+tmpArray.calculate_offset(0,0,0,0,0,0,slc), &dbuff.data_(0,0,0,0,0,0,slcToUse), E0*E1*E2*CHA*N*S*sizeof(std::complex<float>));

            // The header
            ISMRMRD::AcquisitionHeader & currSliceAcqhdr = dbuff.headers_(dbuff.sampling_.sampling_limits_[1].center_,
                    dbuff.sampling_.sampling_limits_[2].center_,
                    0, 0, slcToUse);

            ISMRMRD::ImageHeader currentSliceHeader;

            currentSliceHeader.matrix_size[0]     = E0;
            currentSliceHeader.matrix_size[1]     = E1;
            currentSliceHeader.matrix_size[2]     = E2;
            currentSliceHeader.field_of_view[0]   = dbuff.sampling_.recon_FOV_[0];
            currentSliceHeader.field_of_view[1]   = dbuff.sampling_.recon_FOV_[1];
            currentSliceHeader.field_of_view[2]   = dbuff.sampling_.recon_FOV_[2];
            currentSliceHeader.channels           = CHA;

            currentSliceHeader.average = currSliceAcqhdr.idx.average;
            currentSliceHeader.slice = slc;
            currentSliceHeader.contrast = N; // This isn't the index but rather the size! Use this until I can find something better.
            currentSliceHeader.phase = currSliceAcqhdr.idx.phase;
            currentSliceHeader.repetition = S; // This isn't the index but rather the size!
            currentSliceHeader.set = currSliceAcqhdr.idx.set;
            currentSliceHeader.acquisition_time_stamp = currSliceAcqhdr.acquisition_time_stamp;

            memcpy(currentSliceHeader.position, currSliceAcqhdr.position, sizeof(float)*3);
            memcpy(currentSliceHeader.read_dir, currSliceAcqhdr.read_dir, sizeof(float)*3);
            memcpy(currentSliceHeader.phase_dir, currSliceAcqhdr.phase_dir, sizeof(float)*3);
            memcpy(currentSliceHeader.slice_dir, currSliceAcqhdr.slice_dir, sizeof(float)*3);
            memcpy(currentSliceHeader.patient_table_position, currSliceAcqhdr.patient_table_position, sizeof(float)*3);
            currentSliceHeader.data_type = ISMRMRD::ISMRMRD_CXFLOAT;
            currentSliceHeader.image_index = ++image_counter_;

            cm3->getObjectPtr()->push_back(currentSliceHeader);
            if (slc == 0)
            {
                *(cm1->getObjectPtr()) = currentSliceHeader;
                cm1->getObjectPtr()->slice = SLC;
            }
       }


    // Swap order of dimensions
    std::vector<size_t> dimOrder = {0,1,2,6,3,4,5};
    Gadgetron::permute(&tmpArray,cm2->getObjectPtr(),&dimOrder);

    //Do the FFTs in place
    hoNDFFT<float>::instance()->ifft3c( *cm2->getObjectPtr() );

    //Pass the image down the chain
    if (this->next()->putq(cm1) < 0) {
        return GADGET_FAIL;
    }

    m1->release();
}
    return GADGET_OK;

}

GADGET_FACTORY_DECLARE(wtcFFTGadget)
}
