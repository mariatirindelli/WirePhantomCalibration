cmake_minimum_required(VERSION 3.20)
project(plus_calibration)

set(CMAKE_CXX_STANDARD 11)

add_executable(plus_calibration main.cpp)

find_package(PlusLib REQUIRED)
find_package(ITK REQUIRED)
if( PlusLib_FOUND )
    MESSAGE(STATUS "Found PlusLib")
endif( PlusLib_FOUND )

message(STATUS "${PlusLib_INCLUDE_DIRS}")
target_link_libraries(${PROJECT_NAME}
        PUBLIC
        PlusWidgets
        vtkxio
        vtkPlusCommon
        vtkPlusRendering
        vtkPlusOpenIGTLink
        vtkPlusImageProcessing
        vtkPlusUsSimulator
        vtkPlusVolumeReconstruction
        vtkPlusHaptics
        vtkPlusDataCollection
        vtkPlusCalibration
        vtkPlusServer
        vtkIOCore
        itkv3p_netlib
        )
