# Make-OHIO-Project (First Place!)

MakeOHIO is a hackathon-style event hosted by students at The Ohio State University in which teams have 24 hours to design, build, and implement a solution to a proposed engineering challenge. Projects are evaluated by industry professionals based on how well they perform their intended task and how effectively they address the larger problem.

For this project, my group and I chose to tackle the AEP challenge. The American Electric Power (AEP) LineSense Challenge tasks participants with building a prototype that demonstrates dynamic line rating techniques to help prevent power grid overloads caused by fluctuating environmental conditions. Using a microcontroller, sensors, and a conductor simulator (such as nichrome wire), the system must evaluate how factors like ambient temperature, wind speed, and conductor sag affect a transmission line’s ability to safely carry current without exceeding its Maximum Temperature Limit (MTL). The goal is to provide real-time insights—through visualization, alerts, or analytics—that allow grid operators to identify when a line transitions from a “safe” state to a “stressed” state.

Our solution incorporated several sensors working together to monitor environmental and physical conditions affecting the conductor. We used a temperature sensor to measure ambient temperature around the line. To capture wind speed, we designed and 3D printed an anemometer and used an IR sensor to measure its rotational speed, allowing us to calculate wind speed in MPH. A LiDAR sensor was used to measure conductor sag by detecting changes in the distance to the wire, which indicates increased stress on the line. Additionally, a thermistor was attached to the conductor to directly measure the wire temperature, providing another critical input for determining system safety.

All sensor data was processed using an ESP32 microcontroller. The ESP32 ran an equation that combined the measured variables to estimate the stress level of the conductor and determine whether the line was operating within safe limits. The system then transmitted the status of the line (safe vs. stressed) to a mobile device via Bluetooth, providing a simple real time monitoring interface. The entire system—including hardware integration, sensor calibration, and C++ programming—was designed and implemented within the 24 hour competition window.

Link to video of presentation: https://www.youtube.com/shorts/23V-NVDwGgY

Link to Make OHI/O Website: https://hack.osu.edu/make/2026/



