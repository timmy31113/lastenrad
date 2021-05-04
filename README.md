# lastenrad
Control system for a prototype cargo bike with a bio-electric hybrid drivetrain concept

This project contains the code for an Arduino controlling the electric systems of a three wheeled Dutch-style cargo bike. 

- A synchronous permanently commutated three-phase AC hub motor drives the rear wheel. 
It is powered by an external motor controller with current and movement direction control.
The motor and controller also allow a regenerative braking function.

- The cyclists pedals into a synchronous permanently commutated three-phase AC generator with a gear transmission.
The generator load is regulated by a power-MOSFET performing PWM after rectification. 
A current sensor indicates how much power the cyclist is putting into the generator.

- The UI provides adjustment options for the desired degree of electric assist and the target pedaling RPM and torque.

The system aims to simulate an automatic CVT gearbox and the goal is an intuitive and comfortable driving experience 
that allows the cyclist to concentrate on the road situation while maintaining optimal power delivery to the wheel.
