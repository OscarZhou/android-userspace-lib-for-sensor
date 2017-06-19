# android-userspace-lib-for-sensor


## Startup the emulator:

### In the directory: ~/Android/Sdk/emulator

1. ./emulator64-arm -avd Nexus4 -kernel /home/oscar/Documents/goldfish/arch/arm/boot/zImage

## The part of compiling the necessary file

### In the directory : "~/WORKING_DIRECTORY"

1. . build/envsetup.sh

2. lunch full-eng

### In the directory : "~/WORKING_DIRECTORY/device/generic/goldfish/sensors"

1. mm ( only compile needed file)  or mma (compile all files which is timing consuming)

(compiling test program for HAL layer)
The same: but should note that the different path when input "mm"
the directory should be your test program directory :  "~/WORKING_DIRECTORY/hardware/libhardware/tests/testsensors"

(compiling test program for kernel layer)
### In the directory: "~/Downloads/ass1/"
1. arm-none-linux-gnueabi-gcc -static test5.cpp -o test5
-------------------------------------------

Push test program into emulator:
### In directory Anroid/Sdk/platform-tools:
1. ./adb remount
2. ./adb push ~/WORKING_DIRECTORY/out/target/product/generic/system/bin/testforhal /dev
3. ./adb push ~/Downloads/ass1/test5 /dev

-------------------------------------------
## The part of preparing the runtime:
### In directory Anroid/Sdk/platform-tools:
1. ./adb remount
2. ./adb push ~/WORKING_DIRECTORY/out/target/product/generic/obj/lib/sensors.goldfish.so /system/lib/hw
3. ./adb shell

### In the root directory:
4. stop
5. chmod 777 dev/goldfish_sensor
6. echo 0 > /sys/fs/selinux/enforce
7. start

chmod 777 dev/goldfish_sensor
echo 0 > /sys/fs/selinux/enforce

(check the permission part)
.1 go to directory /dev: ll, to see if the permission of the file "goldfish_sensor" changes
.2 go to directory /system/lib/hw: ll to see if the date of the file "sensors_goldfish.so" is the latest one

### Go to directory /system/lib/hw
8. logcat | grep oscar
9. wait the emulator completely startup

--------------------------------------------
6. run the app. 
