# FanControlLinearized
Linearized control of a PWM fan.  
RPM measured by Frequency Counter using Timers 1 &; 2

## Project Details
The muffin fans used, for example, in Personal Computers can be controlled with a PWM signal but the RPM does not control to zero.
On the low RMP side, there is a range of PWm for which the RPM does not change.
A 92x92 mm square fan was found to have the same RPM (ust below 1000 RPM) for PWM values below about 70 (out of 255).  

**SCREEN SHOT of an uncorrected RPM PLOT**  
![LinearStepBy10_PWM_92mmFan.png](LinearStepBy10_PWM_92mmFan.png)

This firmware linearized the fan by mapping a PWM set value input to the serial monitor of 0 to 255 to a reduce range of 70 to 255.
This firmware also inverts the duty cycle because a transistor was used to buffer the GPIO pin output to the fan PWM input. 
This is because of reports of fans with a pull up to 6V which would exceed the input voltage on 5V and certainly 3.3 Volt controllers.

**SCREEN SHOT of LINEARIZED RPM PLOT**
![MapPWMfrom70to255_92mmFan.png](MapPWMfrom70to255_92mmFan.png)  
Plot of the linearized Fan RPM verse input. (Made by the auto setup feature built into this program) 

## Instructions
Connect the hardware, the fan through an inverting transistor. 
Load the firmware into an UNO using the Arduino IDE.
From the Arduino IDE open the serial plotter. The short cut keys "CTRL, SHIFT, L" is your friend.
Type a number in the interval [0, 255] to set the linearized PWM to the fan.
Typa a number > 255 to start an automatic stepping of the PWM by 10.
Typa a number < 0 to stop the automatic stepping of the PWM.
