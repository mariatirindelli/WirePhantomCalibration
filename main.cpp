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


struct Fiducial
{
    Fiducial(double x, double y, double z, int frame_idx=-1): x(x), y(y), z(z), frame_idx(frame_idx) {};
    double x;
    double y;
    double z;
    int frame_idx = -1;
};

std::vector<std::vector<Fiducial>> get_fiducials_from_file(const std::string& filepath,
                                                           double x_spacing=1,
                                                           double y_spacing=1,
                                                           double z_spacing=1);

vtkSmartPointer<vtkIGSIOTrackedFrameList> read_tracked_frames(const std::string& mha_file,
                                                              const std::string& fiducial_txt_path);

std::vector<double> string2vector(const std::string& input_string);

int main (int argc, char* argv[])
{
    const char* config_path = "/home/maria/imfusion/plus-calibration/config_calibration.xml";

    std::string calibration_sweep_path = "/home/maria/Desktop/materiale_imfusion/wire-phantom-calibration/US-Calibration-Data/SegmentedSweeps/SweepLabels0.mha";
    std::string calibration_fiducial_file_path = "/home/maria/Desktop/materiale_imfusion/wire-phantom-calibration/US-Calibration-Data/SegmentedSweeps/SweepFiducials0.txt";

    std::string validation_sweep_path = "/home/maria/Desktop/materiale_imfusion/wire-phantom-calibration/US-Calibration-Data/SegmentedSweeps/SweepLabels0.mha";
    std::string validation_fiducial_file_path = "/home/maria/Desktop/materiale_imfusion/wire-phantom-calibration/US-Calibration-Data/SegmentedSweeps/SweepFiducials0.txt";

    // Reading the configuration file
    vtkSmartPointer<vtkXMLDataElement> configRootElement = vtkSmartPointer<vtkXMLDataElement>::New();
    if (PlusXmlUtils::ReadDeviceSetConfigurationFromFile(configRootElement, config_path)==PLUS_FAIL)
    {
        LOG_ERROR("Unable to read configuration from file ");
        return EXIT_FAILURE;
    }

    // Reading the coordinate system names from the config file
    vtkSmartPointer<vtkIGSIOTransformRepository> transformRepository = vtkSmartPointer<vtkIGSIOTransformRepository>::New();
    if ( transformRepository->ReadConfiguration(configRootElement) != PLUS_SUCCESS )
    {
        LOG_ERROR("Failed to read CoordinateDefinitions!");
        return EXIT_FAILURE;
    }

    // The pattern recognition algorithm is not really used, it is only initialized to read the phantom geometry
    PlusFidPatternRecognition patternRecognition;
    patternRecognition.ReadConfiguration(configRootElement);

    // Reading the calibration file
    vtkSmartPointer<vtkIGSIOTrackedFrameList> calibrationTrackedFrameList =
            read_tracked_frames(calibration_sweep_path, calibration_fiducial_file_path);

    // Reading the validation file
    vtkSmartPointer<vtkIGSIOTrackedFrameList> validationTrackedFrameList =
            read_tracked_frames(validation_sweep_path, validation_fiducial_file_path);

    // Initialize the probe calibration algo
    vtkSmartPointer<vtkPlusProbeCalibrationAlgo> probeCal = vtkSmartPointer<vtkPlusProbeCalibrationAlgo>::New();
    probeCal->ReadConfiguration(configRootElement);

    // Calibrate
    if (probeCal->Calibrate( validationTrackedFrameList, calibrationTrackedFrameList, transformRepository, patternRecognition.GetFidLineFinder()->GetNWires()) != PLUS_SUCCESS)
    {
        LOG_ERROR("Calibration failed!");
        return EXIT_FAILURE;
    }

    return 0;
}

std::vector<double> string2vector(const std::string& input_string)
{
    std::vector<double> output_vector;
    std::string item;
    std::stringstream text_stream(input_string);
    while (std::getline(text_stream, item, ' ')) {
        output_vector.push_back(stod(item));
    }
    return output_vector;
}

vtkSmartPointer<vtkIGSIOTrackedFrameList> read_tracked_frames(const std::string& mha_file,
                                                              const std::string& fiducial_txt_path)
{
    vtkSmartPointer<vtkIGSIOTrackedFrameList> calibrationTrackedFrameList = vtkSmartPointer<vtkIGSIOTrackedFrameList>::New();
    if( vtkIGSIOSequenceIO::Read(mha_file, calibrationTrackedFrameList) != PLUS_SUCCESS )
    {
        LOG_ERROR("Failed to read tracked frames from sequence metafile from: " << mha_file );
        return calibrationTrackedFrameList;
    }

    // Getting the image spacing
    auto spacing = string2vector(calibrationTrackedFrameList->GetCustomString("ElementSpacing"));

    // Read the fiducials positions from the txt file and save them to the corresponding tracked frame
    std::vector<std::vector<Fiducial>> fiducials = get_fiducials_from_file(fiducial_txt_path,
                                                                           spacing.at(0),
                                                                           spacing.at(1),
                                                                           spacing.at(2));

    // iterate over the frames and assign labeled fiducials coordinates to each frame
    for (unsigned int currentFrameIndex = 0; currentFrameIndex < calibrationTrackedFrameList->GetNumberOfTrackedFrames(); currentFrameIndex++) {
        igsioTrackedFrame *trackedFrame = calibrationTrackedFrameList->GetTrackedFrame(currentFrameIndex);

        vtkSmartPointer<vtkPoints> fiducialPoints = vtkSmartPointer<vtkPoints>::New();
        fiducialPoints->SetNumberOfPoints((long long)fiducials.at(currentFrameIndex).size());

        for (unsigned int i = 0; i < fiducials.at(currentFrameIndex).size(); ++i)
        {
            fiducialPoints->InsertPoint(i, fiducials.at(currentFrameIndex).at(i).x, fiducials.at(currentFrameIndex).at(i).y, fiducials.at(currentFrameIndex).at(i).z);
        }
        fiducialPoints->Modified();
        trackedFrame->SetFiducialPointsCoordinatePx(fiducialPoints);
    }
    return calibrationTrackedFrameList;
}


std::vector<std::vector<Fiducial>> get_fiducials_from_file(const std::string& filepath,
                                                           double x_spacing,
                                                           double y_spacing,
                                                           double z_spacing)
{
    std::vector< std::vector<Fiducial> > fiducials;

    ifstream infile;
    infile.open(filepath); // file containing numbers in 3 columns

    // Return empty vector if the file didn't open
    if(infile.fail())
        return fiducials;

    // Reading the first line containing the column names
    std::string tmp;
    infile >> tmp;

    // Reading the columns into the fiducials vector
    int frame_idx, nWireId, wireId, row, col, num;
    while(infile >> frame_idx && infile >> nWireId && infile >> wireId && infile >> row && infile >> col)
    {
        if (frame_idx >= fiducials.size())
            fiducials.emplace_back();

        fiducials.at(frame_idx).push_back(Fiducial(col*x_spacing, row*y_spacing, 0*z_spacing, frame_idx));
        ++num;
    }
    infile.close();

    return fiducials;
}
