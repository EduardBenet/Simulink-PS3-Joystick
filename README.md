# Simulink PS3 Controller for Raspberry Pi
This repository will aim to help setting up a PS3 controller to be operated using Simulink.

## Step 1: set up the Raspberry Pi using Trixie Litle

Download the Raspberry pi Imager from the [offical website](https://www.raspberrypi.com/software/)

## Step 2: Customize the image 

From `MATLAB > APPS > Hardware Manager > Add > Raspberry Pi`, and follow the prompts. 

In older MATLAB releases you had to go to: `MATLAB > Add Ons > Simulink Support Package for Raspberry Pi` click setup and customize the image to make sure that it can be used from the Simulink.

## Step 3: pair the controller to the raspberry PI. 

On the shell, you can follow the steps in the website below to connect the controller
[PS3 Controller pairing instructions](https://wouterdeschuyter.be/blog/configure-a-ps3-controller-to-automatically-connect-to-a-raspberry-pi)

It should be quite straigtforward, but [this](./Joystick_setup/README.md) is what I did

## Step 4: Create a device driver block

We now need to create the controller for the joystic. For this, we are going to create a MATLAB System block. This is a Device driver Block that takes a C file with the actual code needed to control the Joystick. A good examplem on how to do that can be found [here](https://uk.mathworks.com/help/ecoder/device-driver-blocks.html)
The C code here is derived from [this GitHub repository](https://gist.github.com/jasonwhite/c5b2048c15993d285130) containing a better explanation on how to actually connect the remote. 

