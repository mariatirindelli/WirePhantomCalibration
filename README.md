# WirePhantomCalibration
Implemention of the wire phantom calibration to calibrate a tracked US probe

This repo provides an implementation of the wire phantom calibration algorithm. 
The code was tested against the data in the "test" folder. 

Below, we report a description of the data acquisition procedure used to acquire the data. 

1. We used a fCal
2. ![image](https://user-images.githubusercontent.com/48152056/164425427-aa748a03-439a-449c-9476-6af503b76c8e.png)

To retrieve the Phantom Coordinate System wrt to the Tracker Coordinate Frame, we did the following: 
2.a With a (calibrated) stylus we pointed at known locations on the phantom. 
2.b Given these known points locations, we compute the axes of the phantom coordinate system. According to the PLUS documentation, the phantom coordinate system is defined as: 
// todo: add drawing

3. We recorded 2 sweeps using the tracked US probe.
  3.a As we recorded the data using ImFusion, we used the ImFusion Plugin "ImFusionMhaExporter" in https://github.com/mariatirindelli/ImFusionMhaExporter to save the data as temporal .mha
4. We annotate each sweep manually
5. We used the script "generate_landmarks_coordinates.py" in https://github.com/mariatirindelli/ImFusionMhaExporter to generate a txt file containing the information regarding the landmark position for each frame, as
#frameId,nWireId,wireId,row,col
Where row,col are the row and col pixel location of the landmarks

6. We run the main file on the acquired data and segmented landmark to perform the calibration

# Dependencies
To run the calibration script, one need to build the PLUS library (https://github.com/PlusToolkit/PlusBuild/).
In order to do so (Instruciton for Debug Build): 
>> git clone https://github.com/PlusToolkit/PlusBuild/
>> mkdir PlusBuild && cd PlusBuild 
>> cmake -DCMAKE_BUILD_TYPE=Debug ..
>> make 

# Compile the calibration script
>> git clone https://github.com/mariatirindelli/WirePhantomCalibration
>> cd WirePhantomCalibration && mkdir build
>> cmake-gui
![image](https://user-images.githubusercontent.com/48152056/164428897-82570360-86ab-4792-80dd-1bc76ad61fb4.png)
Set the cmake variables in order to point to the correct location of the PlusLib
>> make





