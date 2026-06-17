### Preliminary Requirements for RVC SW Controller
- An RVC automatically cleans and mops household surface.
- It goes straight forward while cleaning.
- If its sensors found an obstacle, it stops cleaning, turns aside left or right, and goes forward with cleaning.
- If there are obstacles in both front, left and right, it move backward and turn aside left or right, and goes forward.
- If it detects dust, power up the cleaning for a while.
- We do not consider the detail design and implementation on HW controls.
- We only focus on the automatic cleaning function.

### Future or Extended Requirements to Consider
- The RVC will add or change sensors.
- It will be able to circulate one spot for a while.
- It will have to communicate with a mobile app.
- It can do machine learning and inferring for more efficient cleaning.

### Environment
- Builder: CMake
- Unit Test Framework: Google Test
- System Test: Web-based Simulator
