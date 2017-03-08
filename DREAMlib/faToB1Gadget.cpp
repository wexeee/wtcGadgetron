#include "faToB1Gadget.h"
#include "hoNDObjectArray.h"
#include "asymSINCPulse.h"
#include "hoNDImage.h"

// File output on gtron box
#include "ImageIOAnalyze.h"
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
namespace bf = boost::filesystem;

#define GAMMA 0.0002675222099 //gyromagnetic ratio for protons 1/(us uT) with 2pi
namespace Gadgetron{

std::string get_time_string()
    {
        time_t rawtime;
        struct tm * timeinfo;
        time ( &rawtime );
        timeinfo = localtime ( &rawtime );

        std::stringstream str;
        str << std::setw(2) << std::setfill('0') << timeinfo->tm_hour
            << std::setw(2) << std::setfill('0') << timeinfo->tm_min
            << std::setw(2) << std::setfill('0') << timeinfo->tm_sec;

        std::string ret = str.str();

        return ret;
    }

std::string get_date_string()
    {
        time_t rawtime;
        struct tm * timeinfo;
        time ( &rawtime );
        timeinfo = localtime ( &rawtime );

        std::stringstream str;
        str << timeinfo->tm_year+1900
            << std::setw(2) << std::setfill('0') << timeinfo->tm_mon+1
            << std::setw(2) << std::setfill('0') << timeinfo->tm_mday;

        std::string ret = str.str();

        return ret;
    }


faToB1Gadget::faToB1Gadget():
    rfvolt_(0)
{

}


int faToB1Gadget::process_config(ACE_Message_Block* mb)
{

    ISMRMRD::IsmrmrdHeader h;
    ISMRMRD::deserialize(mb->rd_ptr(),h);

    if (h.userParameters)
    {
        for (std::vector<ISMRMRD::UserParameterDouble>::const_iterator i (h.userParameters->userParameterDouble.begin());
             i != h.userParameters->userParameterDouble.end(); i++)
        {
            if (i->name == "PulseVoltage") {
                rfvolt_ = i->value;
                GDEBUG("PulseVoltage found: %f V\n", rfvolt_);
            } else {
                GDEBUG("WARNING: unused user parameter parameter %s found\n", i->name.c_str());
            }
        }
    } else {
        GDEBUG("PulseVoltage are supposed to be in the UserParameters. No user parameter section found\n");
        return GADGET_OK;
    }

    timestring_ = get_date_string();
    timestring_.append(get_time_string());

    bf::path p(folder.value());
    if (!bf::is_directory(p))
        bf::create_directory(p);
}

int faToB1Gadget::process( GadgetContainerMessage< ISMRMRD::ImageHeader>* m1,
                             GadgetContainerMessage< hoNDArray< std::complex<float> > >* m2)
{

    // Calculate the B1 map first
    //Create a new image
    GadgetContainerMessage<ISMRMRD::ImageHeader>* cm1 =
            new GadgetContainerMessage<ISMRMRD::ImageHeader>();

    //Copy the header
    *cm1->getObjectPtr() = *m1->getObjectPtr();

    GadgetContainerMessage< hoNDArray< std::complex<float> > >* cm2 =
            new GadgetContainerMessage<hoNDArray< std::complex<float> > >();
    cm1->cont(cm2);

    boost::shared_ptr< std::vector<size_t> > dims = m2->getObjectPtr()->get_dimensions();

    try{cm2->getObjectPtr()->create(dims.get());}
    catch (std::runtime_error &err){
        GEXCEPTION(err,"Unable to allocate new image array\n");
        cm1->release();
        return GADGET_FAIL;
    }

    std::complex<float>* src = m2->getObjectPtr()->get_data_ptr();
    std::complex<float>* dst = cm2->getObjectPtr()->get_data_ptr();

// Calculate the pulse integral using trapezium rule.
// In matlab: pulse_int   = trapz(linspace(0,tau,numel(pulse_shape)),pulse_shape);
float pulseInt = 0.0;
float timeStep = float(pulseDuration)/float(pulsePoints-1);
for (int iDx = 0; iDx < (pulsePoints-1); iDx++ )
{
    pulseInt += timeStep*(asymSincPulse[iDx+1] + asymSincPulse[iDx])/2.0;
}


//flipmap = flipmap./pulse_int;
//b1map = flipmap./gamma./180*pi;
//b1map_in_nT_per_V = b1map_in_uT .*1000/rfvolt;
//multiply by 10 for conversion to ushort for display.
size_t elements = m2->getObjectPtr()->get_number_of_elements();
for (size_t iDx = 0; iDx < elements; iDx++ )
{
    dst[iDx] = std::complex<float>(10.0,0.0)*(float(M_PI/180.0)*(src[iDx]/pulseInt)/float(GAMMA)) * float(1000.0/rfvolt_);
}
    // Modify the headers to add this as an extra image/series
    cm1->getObjectPtr()->image_series_index += 500;

    //image comment.
    GadgetContainerMessage< ISMRMRD::MetaContainer >* cm3 = new GadgetContainerMessage< ISMRMRD::MetaContainer >;
    cm2->cont(cm3);
    std::string imageComment = "GT_B1(nT/V)";
    cm3->getObjectPtr()->set(GADGETRON_IMAGECOMMENT, imageComment.c_str());

    std::string seriesDescription1 = "_B1";
    cm3->getObjectPtr()->append(GADGETRON_SEQUENCEDESCRIPTION,seriesDescription1.c_str() );

    //Pass the b1 image down the chain
    if (this->next()->putq(cm1) < 0) {
        return GADGET_FAIL;
    }


    if (enableOutput.value())
    {
        //filename
        bf::path p(folder.value());

        std::string outFileName = "";

        outFileName.append(file_prefix.value());

        std::string sliceNum = (boost::format("Slice_%02i_")%cm1->getObjectPtr()->slice).str();

        //std::ostringstream sliceNum;
        //sliceNum << "Slice_" << cm1->getObjectPtr()->slice << "_";

        outFileName.append(sliceNum);

        outFileName.append(timestring_);

        p /= outFileName;
        outFileName = p.string();

        // Save the array out
        hoNDArray< std::complex <float> > toSave(cm2->getObjectPtr());
        toSave.squeeze();
        Gadgetron::ImageIOAnalyze gt_exporter;
        gt_exporter.export_array_complex_real_imag(toSave,outFileName); // only export real and imag parts. Can be changed by using export_array_complex to do r, i, m and p.

        // save pixel sizes, position and orientation out into a separate
        std::vector<float> pixelSizes, position,orientation;
        for (int iDx = 0; iDx <3; iDx++)
        {
            pixelSizes.push_back((float)cm1->getObjectPtr()->field_of_view[iDx]/(float)cm1->getObjectPtr()->matrix_size[iDx]);
            position.push_back((float)cm1->getObjectPtr()->position[iDx]);
            orientation.push_back((float)cm1->getObjectPtr()->read_dir[iDx]);
            orientation.push_back((float)cm1->getObjectPtr()->phase_dir[iDx]);
            orientation.push_back((float)cm1->getObjectPtr()->slice_dir[iDx]);
        }
        outFileName.append("_HEADER");
        std::ofstream output_file(outFileName);
        std::ostream_iterator<float> output_iterator(output_file, "\n");
        std::copy(pixelSizes.begin(), pixelSizes.end(), output_iterator);
        std::copy(position.begin(), position.end(), output_iterator);
        std::copy(orientation.begin(), orientation.end(), output_iterator);

        output_file.close();
    }
    // Scale the fa images so that they are 10 times the flip angle.
    // The scaling of the phase images should be sorted in the floatToUShortGadget
    for (size_t iDx = 0; iDx < elements; iDx++ )
    {
        src[iDx] = std::complex<float>(10.0,0.0)*(src[iDx]);
    }

    // Add some image comments
    GadgetContainerMessage< ISMRMRD::MetaContainer >* m3 = new GadgetContainerMessage< ISMRMRD::MetaContainer >;
    m2->cont(m3);
    std::string imageComment2 = "GT_FA(degrees x 10)";
    m3->getObjectPtr()->set(GADGETRON_IMAGECOMMENT, imageComment2.c_str());

    std::string seriesDescription2 = "_FA";
    m3->getObjectPtr()->append(GADGETRON_SEQUENCEDESCRIPTION,seriesDescription2.c_str() );

    // Now pass the original fa image down the chain
    if (this->next()->putq(m1) < 0) {
        return GADGET_FAIL;
    }

    return GADGET_OK;

}

GADGET_FACTORY_DECLARE(faToB1Gadget)
}

