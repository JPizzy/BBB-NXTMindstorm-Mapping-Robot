# Project Synopsis
Using a BeagleBone Black we send messages, over bluetooth, to a NXT Mindstorm brick. The NXT robot maps a room by measuring distances, by using a NXT IR proximity sensor, and then displaying the mapped information to a website using node.js. Other functionality includes manually controlling the NXT robot using the website interface, manually controlling the robot using the zencape joystick, and increasing/decreasing it’s movement speed using the potentiometer.

### Instructions to Run
1. On your host machine run `make` to compile the project
2. On the target device, the application can be found under the public/myApps/ directory
3. From there navigate to the nxt-server-backup folder
4. Run `node server.js &`
5. Switch back to the project's main directory
6. Run `./nxtMapper`
7. Access the website by going to 192.168.7.2:8088

### Docs
The docs folder contains a guide (FinalProjectGuide.pdf) on how to connect the BeagleBone Black to the NXT brick using bluetooth. Also included are some retrospective documents to review our iterations of this project.

