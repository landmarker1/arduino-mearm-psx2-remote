# MeArm
MeArm controlled via Arduino, PWM board and PSX2 wireless remote

![Imgur](https://i.imgur.com/TlcY6Lq.jpg)

* MeArm (Classic): https://shop.mime.co.uk/collections/mearm/products/mearm-pocket-sized-robot-arm
* Adafruit 16-Channel 12-bit PWM/Servo Driver - I2C interface: https://www.adafruit.com/product/815
* PS2 wireless remote from eBay.
* You'll need Visual Micro to build with Visual Studio: http://www.visualmicro.com/

Main issue is still the servors jittering and buzzing. Some said it could be due to RF interfernce and to add ferrite cores. It might have helped a bit but they still jitter and buzz mostly when the arm is extended. Others said this is normal as the servo is actively trying to maintain its position under stress.
