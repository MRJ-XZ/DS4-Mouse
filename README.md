# DS4-Mouse
A small project to control Ubuntu GUI environment using a Dualshock4. By using Xorg API and xdotool command line tool to communicate with GUI service of linux and a custom driver to get buttons and motion sensors data from Dualshock4, this project simulates a physical mouse actions.

This project is prototype and under development; mainly because it uses command line tools to generate click and hold actions and uses older version of Xorg API (uses X11 instead of XCB which is newer). Some keyboard keys will be added in future.

