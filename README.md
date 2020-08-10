<img style="float: right;" src="SirMetal/SirMetal/docs/images/logo.png">

# SirMetal
A metal based game editor 


Initially inspired by The Cherno project game engine series: https://www.youtube.com/user/TheChernoProject before going my own way. The series is pretty cool, check it out!

This is my attempt to a dx12 engine, this is my third iteration to a so called engine. 
My first one was a poor Opengl viewport, then a more serious approach to a dx11 engine, but mostly geared toward getting stuff on screen quickly. 
Finally this is my third attempt trying to use dx12. 

It is my pleasure to introduce you to Sir Engine, the 3rd of his name.

Here a I will keep a chronological list of the progress:

##### Table of Contents  
[0.1.0: basic drawing](#v010)  
[0.2.0: basic editor layout](#v020)  
[0.3.0: engine structure and slection shader](#v030)  

## 0.1.0 <a name="v010"/>
This version is the hello world.
* Baic cocoa window
* Basic goemetry rendering in metal 
* Imgui
* Dx12 init
* Resize and clear color

![alt text](./docs/images/SirMetal01.png "basic")

## 0.1.2 <a name="v012"/>
Basic editor UI, the first step toward building an editor workflow
* Basic editor UI with docking
* Logging to editor, console and file 
* Loading shaders from file

![alt text](./docs/images/SirMetal02.png "basicui")

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
