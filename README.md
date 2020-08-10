<img style="float: right;" src="SirMetal/SirMetal/docs/images/logo.png">

# SirMetal
A metal based game editor 






Here a I will keep a chronological list of the progress, click on the image to play a video (if an associated one exists):

##### Table of Contents  
[0.1.0: basic drawing](#v010)  
[0.2.0: basic editor layout](#v020)  
[0.3.0: engine structure and slection shader](#v030)

## What is this project

## What is not

## 0.1.0 <a name="v010"/>
This version is the hello world.
* Baic cocoa window
* Basic goemetry rendering in metal 
* Imgui
* Dx12 init
* Resize and clear color

[![basic](./docs/images/SirMetal01.png "basic")](https://www.youtube.com/watch?v=3p58WVu8q5QERE)


## 0.1.2 <a name="v012"/>
Basic editor UI, the first step toward building an editor workflow
* Basic editor UI with docking
* Logging to editor, console and file 
* Loading shaders from file

[![basicui](./docs/images/SirMetal02.png "basic")](https://www.youtube.com/watch?v=p89QT_giSf0)

## 0.1.5 <a name="v012"/>
Developing editor workflow and engine backend systems
* Added concpet of project
* Assets loaded from project folder, for now meshes and shaders
* Loading obj meshes
* Added texture, shader and mesh manager to handle resources through opaque handles
* Added camera controls with settings being loaded and saved with project
* Added jump flooding for creating selection shader
* Added pso hashing to generate required PSOs on the fly if needed, user does not have to worry

![alt text](./docs/images/SirMetal03.png "meshes")
