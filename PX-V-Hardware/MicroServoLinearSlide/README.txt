                   .:                     :,                                          
,:::::::: ::`      :::                   :::                                          
,:::::::: ::`      :::                   :::                                          
.,,:::,,, ::`.:,   ... .. .:,     .:. ..`... ..`   ..   .:,    .. ::  .::,     .:,`   
   ,::    :::::::  ::, :::::::  `:::::::.,:: :::  ::: .::::::  ::::: ::::::  .::::::  
   ,::    :::::::: ::, :::::::: ::::::::.,:: :::  ::: :::,:::, ::::: ::::::, :::::::: 
   ,::    :::  ::: ::, :::  :::`::.  :::.,::  ::,`::`:::   ::: :::  `::,`   :::   ::: 
   ,::    ::.  ::: ::, ::`  :::.::    ::.,::  :::::: ::::::::: ::`   :::::: ::::::::: 
   ,::    ::.  ::: ::, ::`  :::.::    ::.,::  .::::: ::::::::: ::`    ::::::::::::::: 
   ,::    ::.  ::: ::, ::`  ::: ::: `:::.,::   ::::  :::`  ,,, ::`  .::  :::.::.  ,,, 
   ,::    ::.  ::: ::, ::`  ::: ::::::::.,::   ::::   :::::::` ::`   ::::::: :::::::. 
   ,::    ::.  ::: ::, ::`  :::  :::::::`,::    ::.    :::::`  ::`   ::::::   :::::.  
                                ::,  ,::                               ``             
                                ::::::::                                              
                                 ::::::                                               
                                  `,,`


http://www.thingiverse.com/thing:2038205
Servo Linear Actuator (95-137mm) v1 by EraseGrey is licensed under the Creative Commons - Attribution license.
http://creativecommons.org/licenses/by/3.0/

# Summary

Servo linear actuator designed around a Emax ES08MAII. Should fit other micro 9g servo. Actuator length range from 95-137mm between M3 mount holes. The servo arm is 25mm and the linkage is 50mm.  Subtract 92.03mm from actuator distance to trig servo angle and the equation below was written where servo angle 0° is straight down (retract) and 180° is straight up (extend) with angle limit  30°-150°. 

The base frame and the retainer are to be glued together with the actuator rod assembled. I printed the rod Z up and it's weaker than I'd like so I would think about printing it flat on the bed next time. It's a rough design which I'll revise in the future.

Servo Angle = 180° - Degrees(ACos(((Act. Length - Act. Ext.)^2 + Servo Arm^2 - Servo Link^2) / (2 x (Act. Length - Act. Ext.) x Servo Arm)))

Servo Angle = 30° - 150°
Act. Length = Actuator Length 95 - 137mm
Act, Ext = Actuator Extension = 92.03mm
Servo Arm = 25mm
Servo Link = 50mm

Best Regards,