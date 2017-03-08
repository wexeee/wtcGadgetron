#include "wtcSplitGadget.h"
#include "hoNDObjectArray.h"
#include "hoNDArray_elemwise.h"
#include "mri_core_def.h"

namespace Gadgetron{

wtcSplitGadget::wtcSplitGadget()
{

}


int wtcSplitGadget::process( GadgetContainerMessage< ISMRMRD::ImageHeader>* m1,
                             GadgetContainerMessage< hoNDArray< std::complex<float> > >* m2)
{

    std::vector<size_t> img_dims(4);
    img_dims[0] = m1->getObjectPtr()->matrix_size[0];
    img_dims[1] = m1->getObjectPtr()->matrix_size[1];
    img_dims[2] = m1->getObjectPtr()->matrix_size[2];
    img_dims[3] = m1->getObjectPtr()->channels;

    size_t numRep = m1->getObjectPtr()->repetition;
    size_t slice = m1->getObjectPtr()->slice;
    GDEBUG_STREAM("Current slice = " << slice << std::endl);

        //Loop over S
        for (uint16_t rDx=0; rDx < numRep; rDx++) {

            //Create a new image
            GadgetContainerMessage<ISMRMRD::ImageHeader>* cm1 =
                    new GadgetContainerMessage<ISMRMRD::ImageHeader>();

            //Copy the header
            *cm1->getObjectPtr() = *m1->getObjectPtr();

            GadgetContainerMessage< hoNDArray< std::complex<float> > >* cm2 =
                    new GadgetContainerMessage<hoNDArray< std::complex<float> > >();
            cm1->cont(cm2);

            try{cm2->getObjectPtr()->create(&img_dims);}
            catch (std::runtime_error &err){
                GEXCEPTION(err,"Unable to allocate new image array\n");
                cm1->release();
                return GADGET_FAIL;
            }

            std::vector<size_t> subArrayStart(5);
            subArrayStart[0] = 0;
            subArrayStart[1] = 0;
            subArrayStart[2] = 0;
            subArrayStart[3] = 0;
            subArrayStart[4] = rDx;

            std::vector<size_t> subArraySize(img_dims);
            subArraySize.push_back(1);
            //subArraySize.push_back(1);

            m2->getObjectPtr()->get_sub_array(subArrayStart, subArraySize, *(cm2->getObjectPtr()));

            cm1->getObjectPtr()->repetition = rDx;
            cm1->getObjectPtr()->image_index = (slice*numRep)+rDx;

            // To DO - add slice and channel image comment.
            //image comment.
            GadgetContainerMessage<ISMRMRD::MetaContainer>* m3 = AsContainerMessage< ISMRMRD::MetaContainer >(m2->cont());
            GadgetContainerMessage<ISMRMRD::MetaContainer>* cm3 = new GadgetContainerMessage<ISMRMRD::MetaContainer>();

            //Copy the metacontainer
            *cm3->getObjectPtr() = *m3->getObjectPtr();

            std::ostringstream imageCommentStream;
            imageCommentStream << "Slice_" << m1->getObjectPtr()->slice << "_Channel_" << rDx;
            std::string imageComment = std::string(cm3->getObjectPtr()->as_str(GADGETRON_IMAGECOMMENT)) + "_" + imageCommentStream.str();

            //append isn't working for some reason. hack around it
            //cm3->getObjectPtr()->append(GADGETRON_IMAGECOMMENT, imageComment.c_str());
            cm3->getObjectPtr()->set(GADGETRON_IMAGECOMMENT, imageComment.c_str());
            cm2->cont(cm3);

//            GDEBUG_STREAM("GADGETRON_IMAGECOMMENT is " << cm3->getObjectPtr()->as_str(GADGETRON_IMAGECOMMENT));
            //Pass the image down the chain
            if (this->next()->putq(cm1) < 0) {
                return GADGET_FAIL;
            }
        }

//    m2->cont(NULL);
//    m2->release();
//    m1->release();


    return GADGET_OK;

}

GADGET_FACTORY_DECLARE(wtcSplitGadget)
}

