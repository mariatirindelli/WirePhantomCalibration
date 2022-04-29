/*!

*/

#include "PlusConfigure.h"
#include "PlusFidPatternRecognition.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkPlusProbeCalibrationAlgo.h"
#include "vtkIGSIOSequenceIO.h"
#include "vtkIGSIOTrackedFrameList.h"
#include "vtkIGSIOTransformRepository.h"
#include "vtkXMLDataElement.h"
#include "vtkPoints.h"
#include "vtksys/CommandLineArguments.hxx"

///////////////////////////////////////////////////////////////////

class PatternRecognitionWrapper : public PlusFidPatternRecognition
        {
public:
    PatternRecognitionWrapper() : PlusFidPatternRecognition(){}
    ~PatternRecognitionWrapper() override = default;
    void setDebugMode() {m_FidSegmentation.SetDebugOutput(true);}
        };

// todo: this is not really optimized as it reopens the file for each frame but for now we keep it like this
std::vector< std::vector<double> > get_fiducials_from_file(const std::string& filepath, unsigned int frame_id);


int main (int argc, char* argv[])
{
    bool pattern_recognition = false;

     //std::string calibration_sweep_path = "/home/maria/Desktop/materiale_imfusion/wire-phantom-calibration/US-Calibration-Data/DataRecordings/USTC_Ulterius_RandomStepperMotionData1.igs.mha";
    // std::string calibration_sweep_path = "/home/maria/Desktop/materiale_imfusion/wire-phantom-calibration/US-Calibration-Data/DataRecordings/sweep2.mha";
    // std::string calibration_sweep_path = "/home/maria/Desktop/materiale_imfusion/wire-phantom-calibration/US-Calibration-Data/DataRecordings/segmentedTestDataTemp.mha";

    std::string calibration_sweep_path = "/home/maria/Desktop/materiale_imfusion/wire-phantom-calibration/US-Calibration-Data/SegmentedSweeps/SweepLabels0.mha";
    std::string fiducial_file_path = "/home/maria/Desktop/materiale_imfusion/wire-phantom-calibration/US-Calibration-Data/SegmentedSweeps/SweepFiducials0.txt";
    const char* config_path = "/home/maria/imfusion/plus-calibration/config.xml";

    vtkPlusProbeCalibrationAlgo* calibrationAlgo = vtkPlusProbeCalibrationAlgo::New();

    vtkSmartPointer<vtkXMLDataElement> configRootElement = vtkSmartPointer<vtkXMLDataElement>::New();
    if (PlusXmlUtils::ReadDeviceSetConfigurationFromFile(configRootElement, config_path)==PLUS_FAIL)
    {
        LOG_ERROR("Unable to read configuration from file ");
        return EXIT_FAILURE;
    }

    // Reading the Optimizer configuration options
    auto res = calibrationAlgo->ReadConfiguration(configRootElement);

    // Reading the coordinates system definitions
    // Read coordinate definitions
    vtkSmartPointer<vtkIGSIOTransformRepository> transformRepository = vtkSmartPointer<vtkIGSIOTransformRepository>::New();
    if ( transformRepository->ReadConfiguration(configRootElement) != PLUS_SUCCESS )
    {
        LOG_ERROR("Failed to read CoordinateDefinitions!");
        return EXIT_FAILURE;
    }

    PatternRecognitionWrapper patternRecognition;
    patternRecognition.setDebugMode();
    //PlusFidPatternRecognition patternRecognition;
    PlusFidPatternRecognition::PatternRecognitionError error;
    patternRecognition.ReadConfiguration(configRootElement);

    // Load and segment calibration tracked frame list
    vtkSmartPointer<vtkIGSIOTrackedFrameList> calibrationTrackedFrameList = vtkSmartPointer<vtkIGSIOTrackedFrameList>::New();
    if( vtkIGSIOSequenceIO::Read(calibration_sweep_path, calibrationTrackedFrameList) != PLUS_SUCCESS )
    {
        LOG_ERROR("Failed to read tracked frames from sequence metafile from: " << calibration_sweep_path );
        return EXIT_FAILURE;
    }

    // If fiducials are not manually set, the fiducials search can be done automatically on the input tracked frames.
    // For doing so, it is necessary to correctly tune the segmentation parameters in the config xml files
    if (pattern_recognition)
    {
        int numberOfSuccessfullySegmentedCalibrationImages = 0;
        if (patternRecognition.RecognizePattern(calibrationTrackedFrameList, error, &numberOfSuccessfullySegmentedCalibrationImages) != PLUS_SUCCESS)
        {
            LOG_ERROR("Error occured during segmentation of calibration images!");
            return EXIT_FAILURE;
        }
    }

    // If fiducials are manually labelled and stored, they are read here and assigned to the corresponding frame in the
    // tracked frames
    else
    {
        // iterate over the frames
        for (unsigned int currentFrameIndex = 0; currentFrameIndex < calibrationTrackedFrameList->GetNumberOfTrackedFrames(); currentFrameIndex++) {
            igsioTrackedFrame *trackedFrame = calibrationTrackedFrameList->GetTrackedFrame(currentFrameIndex);

            // retrieve the fiducials for the current frame
            std::vector<std::vector<double>> fiducials = get_fiducials_from_file(fiducial_file_path, currentFrameIndex);

            vtkSmartPointer<vtkPoints> fiducialPoints = vtkSmartPointer<vtkPoints>::New();
            fiducialPoints->SetNumberOfPoints(fiducials.size());

            for (unsigned int i = 0; i < fiducials.size(); ++i)
            {
                fiducialPoints->InsertPoint(i, fiducials[i][0], fiducials[i][1], 0.0);
            }
            fiducialPoints->Modified();
            trackedFrame->SetFiducialPointsCoordinatePx(fiducialPoints);
        }
    }

    // Initialize the probe calibration algo
    vtkSmartPointer<vtkPlusProbeCalibrationAlgo> probeCal = vtkSmartPointer<vtkPlusProbeCalibrationAlgo>::New();
    probeCal->ReadConfiguration(configRootElement);

    // todo: change the validation frame list
    // Calibrate
    if (probeCal->Calibrate( calibrationTrackedFrameList, calibrationTrackedFrameList, transformRepository, patternRecognition.GetFidLineFinder()->GetNWires()) != PLUS_SUCCESS)
    {
        LOG_ERROR("Calibration failed!");
        return EXIT_FAILURE;
    }

    return 0;

}

// this function is very hacky - todo: write it better
std::vector< std::vector<double> > get_fiducials_from_file(const std::string& filepath, unsigned int frame_id)
{
    std::vector< std::vector<double> > fiducials;

    fstream newfile;
    newfile.open(filepath, ios::in); //open a file to perform read operation using file object
    if (newfile.is_open()) { //checking whether the file is open
        std::string tp;
        while (getline(newfile, tp)) { //read data from file object and put it into string.
            std::istringstream f(tp);
            std::string s;
            std::vector<double> current_fiducial;

            // Getting the first item, corresponding with the frame id
            getline(f, s, ',');

            unsigned int current_frame_id = 20000;
            // if the frame id coincides with the passed frame id, then extract the other info
            try {
                current_frame_id = std::stoi(s);
            }
            catch (...) {
                continue;
            }

            if (current_frame_id != frame_id)
                continue;

            // extracting the other info
            std::vector<int> fiducial_info;
            fiducial_info.push_back(frame_id);
            while (getline(f, s, ',')) {
                cout << s << endl;

                int current_value = -1;
                try {
                    current_value = std::stoi(s);
                }
                catch (...) {
                    continue;
                }
                fiducial_info.push_back(current_value);
            }
            current_fiducial.push_back(fiducial_info.at(3)); // pushing back row
            current_fiducial.push_back(fiducial_info.at(4)); // pushing back col
            fiducials.push_back(current_fiducial);
        }
    }
        newfile.close(); //close the file object.
        return fiducials;
}
