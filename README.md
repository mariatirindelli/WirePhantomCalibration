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

### How does the calibration works
Assume to have a phantom with an NWire at the row 9 of the phantom grid, cabled as in the picture: 

![grafik](https://user-images.githubusercontent.com/48152056/165902594-6d529558-b812-4f65-bb37-271bc551cae3.png)

Now, consider the ultrasound image plane in the image, indicated as a solid red line in the drawing below, and the points X1, X2, X3 indicating the points where the Ultrasound image intersects the Nwire frame, visible as white dots in the ultrasound image

![grafik](https://user-images.githubusercontent.com/48152056/165905108-920c8e29-0cc6-497d-80f6-cc92f05d30c8.png)

where {H} indicates the phantom coordinate system and {U} indicates the image coordinate system. 

If we consider the following triangles, we can see that the triangles ABC and AX2X1 are similar.
Therefore, it holds true that:

![grafik](https://user-images.githubusercontent.com/48152056/165909496-3803ee4a-933a-403e-957b-527e6503cb55.png)

Note that this holds true, regardless on the coordinate system used. Therefore:

![grafik](https://user-images.githubusercontent.com/48152056/165909681-b9e186e1-1332-4436-b433-ccf0adfa937b.png)

Where "H" indicates the phantom coordinate system and "U" the Ultrasound coordinate system. 

Furthermore we can see that:

![grafik](https://user-images.githubusercontent.com/48152056/165917343-09a4476e-24a1-4c59-bf9b-d515a3d88a7f.png)


Given a point in the Ultrasound Image coordinate system, we can express the same point in the phantom coordinate system (H) as 

![grafik](https://user-images.githubusercontent.com/48152056/165918408-1f542f39-c021-4164-812c-fec005e7fa6a.png)

and: 

![grafik](https://user-images.githubusercontent.com/48152056/166210305-5b1fbe02-7324-4d4a-9045-6ee2fc5c71c5.png)

### Preparing the config .xml file
The config .xml is parsed and used by PLUS class to load the parameters of given algorithms. Here, we provide a description of a basic .xml config to perform the US probe calibration

1. *Coordintate definition* This block defines the different coordinate systems involved in the calibration

```xml
<CoordinateDefinitions>

        <Transform From="Phantom" To="Reference"
                   Matrix="-0.0907699158887156 -0.184598217527959 -0.978613468359728 233.721887857712
                           0.889602812204198 -0.456720481653725 0.00363842773349177 -20.0402839931967
                           -0.447624461896284 -0.870247033734382 0.205675573145117 -779.669497681758
                           0 0 0 1"
               Date="2011.12.01 17:57:00" Error="0.0" />
        <Transform From="Image" To="TransducerOriginPixel"
                   Matrix="1 0 0 -410
                          0 1 0 0
                          0 0 1 0
                          0 0 0 1"
                   Date="2011.12.06 17:57:00" Error="0.0" />
    </CoordinateDefinitions>
```

In This case, we have the PhantomToReference matrix, defining the transformation of the physical phantom wrt the reference (tracker) coordinate system. This transformation matrix was found at the previous step ("Finding the PhantomToReference transform") and should be copied here in the .xml file. 
The ImageToTransducerOriginPixel is the transformation between the image origin (top left pixel of the image) to the center of the transducer (the middle of the transducer) in pixel. It corresponds to a translation along the image width of half of the image width.

2. *Phantom Definition* : This block defines the geometry of the phantom and the arrangment of the phantom wiring
    
```xml
<PhantomDefinition>
        <Description
                Name="fCAL"
                Type="Multi-N"
                Version="1.2"
                WiringVersion="1.1"
                Institution="Queen's University PerkLab" />
        <Geometry>
            <Pattern Type="NWire">
                <Wire Name="1:D9_d9" EndPointFront="-10.0 0.0 -50.0" EndPointBack="-10.0 40.0 -50.0" />
                <Wire Name="2:E9_h9" EndPointFront="0.0 0.0 -50.0" EndPointBack="30.0 40.0 -50.0" />
                <Wire Name="3:I9_i9" EndPointFront="40.0 0.0 -50.0" EndPointBack="40.0 40.0 -50.0" />
            </Pattern>
            <Pattern Type="NWire">
                <Wire Name="4:D10_d10" EndPointFront="-10.0 0.0 -60.0" EndPointBack="-10.0 40.0 -60.0" />
                <Wire Name="5:F10_g10" EndPointFront="10.0 0.0 -60.0" EndPointBack="20.0 40.0 -60.0" />
                <Wire Name="6:I10_i10" EndPointFront="40.0 0.0 -60.0" EndPointBack="40.0 40.0 -60.0" />
            </Pattern>
            <Pattern Type="NWire">
                <Wire Name="7:D11_d11" EndPointFront="-10.0 0.0 -70.0" EndPointBack="-10.0 40.0 -70.0" />
                <Wire Name="8:E11_h11" EndPointFront="0.0 0.0 -70.0" EndPointBack="30.0 40.0 -70.0" />
                <Wire Name="9:I11_i11" EndPointFront="40.0 0.0 -70.0" EndPointBack="40.0 40.0 -70.0" />
            </Pattern>
            <Pattern Type="NWire">
                <Wire Name="10:D12_d12" EndPointFront="-10.0 0.0 -80.0" EndPointBack="-10.0 40.0 -80.0" />
                <Wire Name="11:F12_g12" EndPointFront="10.0 0.0 -80.0" EndPointBack="20.0 40.0 -80.0" />
                <Wire Name="12:I12_i12" EndPointFront="40.0 0.0 -80.0" EndPointBack="40.0 40.0 -80.0" />
            </Pattern>
            <Pattern Type="NWire">
                <Wire Name="10:D13_d13" EndPointFront="-10.0 0.0 -90.0" EndPointBack="-10.0 40.0 -90.0" />
                <Wire Name="11:E13_h13" EndPointFront="0.0 0.0 -90.0" EndPointBack="30.0 40.0 -90.0" />
                <Wire Name="12:I13_i13" EndPointFront="40.0 0.0 -90.0" EndPointBack="40.0 40.0 -90.0" />
            </Pattern>
        </Geometry>
    </PhantomDefinition>
```

In *Geometry/Pattern Type="NWire"* We report the positions (In physical space, referred to the phantom coordinate system) of each wire constituting an NWire, for each defined NWire. 
  
2. *vtkPlusProbeCalibrationAlgo* : This block contains the information on the coordinate system naming, needed for the calibration
```xml
    <vtkPlusProbeCalibrationAlgo
            ImageCoordinateFrame="Image"
            ProbeCoordinateFrame="Probe"
            PhantomCoordinateFrame="Phantom"
            ReferenceCoordinateFrame="Reference" />
```
  
