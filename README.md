# WirePhantomCalibration
Implemention of the wire phantom calibration to calibrate a tracked US probe

This repo provides an implementation of the wire phantom calibration algorithm. 
The code was tested with the data contained in https://drive.google.com/drive/folders/1xyd7NPewnFswJNDuWIy-YAbZR7nK0i4Q 

We used a fCal3.1  - the model is contained in Data/PhantomModel/fCal_3.1.stl

![image](https://user-images.githubusercontent.com/48152056/164425427-aa748a03-439a-449c-9476-6af503b76c8e.png)

We used 5 NWires. The workspace Data/PhantomModel/PhantomWiring.iws contains a visualization on the wiring of the phantom.


Below, we report a description of steps to find the PhantomToReference coordinate system and to acquire the ultrasound data. 

## Calibrating the stylus
To calibrate the stylus, we acquired a tracking stream of the stylus in ImFusion, by keeping the stylus tip on a fixed rotation and rotating the top part of the stylus. 
The data are stored in Data/DataRecordings/pointerTipCalibration.imf

We used the ImFusion Stylus calibration method to find the Transformation Matrix between the stylus tip and the stylus coordinate system.

## Finding the PhantomToReference transform
The phantom coordinate system is as defined in http://perk-software.cs.queensu.ca/plus/doc/nightly/user/ApplicationfCalCoordinateSystemDefinitions.html
that is, it has the origin in a5, the x axis going from col A to B, the z axis going from row 4 to 1 and the y axis going from the fronto to the back of the phantom:

![grafik](https://user-images.githubusercontent.com/48152056/164456073-1731a84f-03e5-4e9d-88ea-30e21353120c.png)

Note:
By design of the .stl model of the phantom, the phantom coordinate system is centered with the world coordinate system when loaded in ImFusion. 

To find the transform of the phantom wrt to the reference coordinate system we do the following: 

1. Place the stylus on known landmarks in the phantom, and recorded a tracking stream with the stylus still in these positions. Specifically, we chose the points 
1, 2, 4, 5, 7, 8, 10, 11 on the physical phantom.

2. We select the same corresponding points on the phantom .stl model and store them in a txt file (data/phantom_landmarks.txt)

3. we launch the find_phantom_to_reference_transform.py script to compute the actual transformation of the physical phantom with respect to the world (reference) coordinate system, corresponding to the PhantomToReference transform. The script does the following: 
  a. Average the tracking stream obtained from the stylus for each acquired point, so to reduce the noise
  b. Performs a corresponding point rigid registration between the points acquired on the physical phantom with the stylus and the corresponding points on the phantom .stl model.

## Ultrasound Sweep Acquisition
1. We recorded 2 sweeps using the tracked US probe.
  1.a As we recorded the data using ImFusion, we used the ImFusion Plugin "ImFusionMhaExporter" in https://github.com/mariatirindelli/ImFusionMhaExporter to save the data as temporal .mha. The original .imf sweeps are stored in Data/DataRecordings/wirePhantomSweeps.imf. The file contains 3 sweeps. 
2. We annotate each sweep manually. The label data converted to temporal .mha are stored in Data/DataRecordings/SegmentedSweeps/SweepLabels0.mha and Data/DataRecordings/SegmentedSweeps/SweepLabels1.mha
3. We used the script "generate_landmarks_coordinates.py" in https://github.com/mariatirindelli/ImFusionMhaExporter to generate a txt file containing the information regarding the landmark position for each frame, as
#frameId,nWireId,wireId,row,col
Where row,col are the row and col pixel location of the landmarks

## Calibration

### Compiling the PLUS Library
To run the calibration script, one need to build the PLUS library (https://github.com/PlusToolkit/PlusBuild/).
In order to do so (Instruciton for Debug Build): 
```console
git clone https://github.com/PlusToolkit/PlusBuild/
mkdir PlusBuild && cd PlusBuild 
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

### Compile the calibration script
```console
git clone https://github.com/mariatirindelli/WirePhantomCalibration
cd WirePhantomCalibration && mkdir build
```
Set the CMAKE variablesin order to point to the correct location of the PlusLib
```console
cmake-gui
```

![image](https://user-images.githubusercontent.com/48152056/164428897-82570360-86ab-4792-80dd-1bc76ad61fb4.png)
Set the cmake variables 
```console
make
```

## Running the calibration

6. We run the main file on the acquired data and segmented landmark to perform the calibration
