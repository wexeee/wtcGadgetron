//B0ScalingGadget.cpp

#include "B0ScalingGadget.h"
#include "mri_core_def.h"
#include <ismrmrd/xml.h>
#include "ImageIOAnalyze.h"
#include <boost/filesystem.hpp>

namespace bf = boost::filesystem;

using namespace Gadgetron;

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
     
    bf::path p(folder.value());
    if (!bf::is_directory(p))
        bf::create_directory(p);

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
      d[i] = -1.0*d[i]/scalingFactor; // Note the -1 which is in Des' code.
  }

  if (enableOutput.value())
  {
      //filename
      bf::path p(folder.value());

      std::string outFileName = "";

      outFileName.append(file_prefix.value());

      std::string timestring;
      timestring = get_date_string();
      timestring.append(get_time_string());
      outFileName.append(timestring);

      p /= outFileName;
      outFileName = p.string();

      hoNDArray<float> toSave(m2->getObjectPtr());
      toSave.squeeze();

      Gadgetron::ImageIOAnalyze gt_exporter;
      gt_exporter.export_array(toSave,outFileName);

      // save pixel sizes, position and orientation out into a separate
      std::vector<float> pixelSizes, position,orientation;
      for (int iDx = 0; iDx <3; iDx++)
      {
          pixelSizes.push_back((float)m1->getObjectPtr()->field_of_view[iDx]/(float)m1->getObjectPtr()->matrix_size[iDx]);
          position.push_back((float)m1->getObjectPtr()->position[iDx]);
          orientation.push_back((float)m1->getObjectPtr()->read_dir[iDx]);
          orientation.push_back((float)m1->getObjectPtr()->phase_dir[iDx]);
          orientation.push_back((float)m1->getObjectPtr()->slice_dir[iDx]);
      }
      outFileName.append("_HEADER");
      std::ofstream output_file(outFileName);
      std::ostream_iterator<float> output_iterator(output_file, "\n");
      std::copy(pixelSizes.begin(), pixelSizes.end(), output_iterator);
      std::copy(position.begin(), position.end(), output_iterator);
      std::copy(orientation.begin(), orientation.end(), output_iterator);

      output_file.close();
   }

  // change to Hz offset for scanner output
  for (unsigned long int i = 0; i < elements; i++)
  {
      d[i] = d[i]* (LarmorFrequencyHz_/fieldStrength_);
  }

  m1->getObjectPtr()->image_type = ISMRMRD::ISMRMRD_IMTYPE_REAL; // Change it to real so that the Float to Short Gadget recenters

  //Now pass on image
  if (this->next()->putq(m1) < 0) {
    return GADGET_FAIL;
  }

  return GADGET_OK;
}

GADGET_FACTORY_DECLARE(B0ScalingGadget)
