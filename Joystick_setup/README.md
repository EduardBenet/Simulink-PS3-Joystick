# PS3 Controller Bluetooth Setup

This guide pairs a PS3 (SIXAXIS / DualShock 3) controller with the Raspberry Pi over Bluetooth.

Connect to the Pi from a direct shell, or open one from MATLAB:
```matlab
r = raspberrypi;
openShell(r)
```

## 1. Update the Pi and install dependencies
```
sudo apt-get update
sudo apt-get upgrade
sudo apt-get install libusb-dev joystick evtest
```

## 2. Build `sixpair`
`sixpair` writes the Raspberry Pi's Bluetooth address into the controller so it knows which host to connect to. The source is mirrored in this repository (`sixpair.c`) in case it stops being available upstream.
```
mkdir ~/sixpair
cd ~/sixpair
wget https://raw.githubusercontent.com/EduardBenet/SimulinkJoystick/refs/heads/main/Joystick_setup/sixpair.c
gcc -o sixpair sixpair.c -lusb
```

## 3. One-time Bluetooth configuration

PS3 controllers do not support Secure Simple Pairing and mishandle legacy PIN pairing, so they can never complete a standard Bluetooth bond. Left at their defaults, BlueZ will repeatedly try and fail to bond the controller — visible as `pincode_reply() Invalid PIN length` errors in the bluetooth log, and a PIN/password prompt every time the controller powers on. BlueZ instead ships a dedicated `sixaxis` plugin that talks to these controllers directly, but it only does so once classic bonding is no longer required.

**Allow the controller to connect without a classic Bluetooth bond:**
```
sudo bash -c 'cat > /etc/bluetooth/input.conf << EOF
[General]
ClassicBondedOnly=false
EOF'
```

**Disable Bluetooth ERTM**, a known source of connection instability with these controllers:
```
sudo bash -c 'echo 1 > /sys/module/bluetooth/parameters/disable_ertm'
echo "options bluetooth disable_ertm=Y" | sudo tee /etc/modprobe.d/bluetooth.conf
```

**Enable automatic adapter power-on at boot**, so the controller can reconnect after a reboot without a manual `bluetoothctl power on`. In `/etc/bluetooth/main.conf`, under `[Policy]`, set:
```
AutoEnable=true
```

**Apply the changes:**
```
sudo systemctl restart bluetooth
```

This section only needs to be done once per Raspberry Pi image.

## 4. Pair the controller

The `sixaxis` plugin only registers and authorizes a controller once it has seen it over USB — pressing the PS button on a controller BlueZ has no prior record of will simply be refused. The reliable procedure is:

1. Plug the controller into the Pi with a USB cable. The four player LEDs start blinking.
2. Press the PS button. This lets `sixpair` (below) and the `sixaxis` plugin see the controller and register it.
3. Run `sixpair` to (re)write the Pi's Bluetooth address into the controller:
   ```
   sudo ~/sixpair/sixpair
   ```
4. Unplug the USB cable.
5. Start bluetoothctl.
   ```
   sudo bluetoothctl
   agent on
   default-agent
   connect MAC
   turst MAC
   ```
6. Connect the PS controller cable and press the PS button. (controller connects immediately).
7. Unplug the controoler and press the PS button again. The controller connects over Bluetooth using the address `sixpair` just wrote.

A successful connection typically completes within a few seconds. You can confirm it in the bluetooth log:
```
sudo journalctl -u bluetooth -n 20 --no-pager
```
Look for `sixaxis: compatible device connected` followed by `sixaxis: setting up new device`, with no PIN-related errors.

Reboot once to confirm the pairing survives a restart:
```
sudo reboot
```

## 5. Verify the controller

Confirm the controller is visible and reporting input correctly before wiring anything up in Simulink:
```
ls /dev/input/js*
jstest /dev/input/js0
```
`jstest` prints a live-updating line of axis and button values as you move the sticks or press buttons (Ctrl+C to exit). If `/dev/input/js0` doesn't exist, the controller hasn't connected at the kernel level yet — see Troubleshooting. If it exists but the values never change, the connection succeeded but input isn't reaching the device.

For lower-level inspection — for example, confirming exactly how many axes and buttons the controller reports, which matters for the driver's fixed-size `axis`/`button` arrays — use `evtest`:
```
sudo evtest
```
It lists all input devices, lets you select the controller, and prints raw event codes as they occur.

## Troubleshooting

**No USB device appears when the controller is plugged in.**
Check `dmesg | tail` for a USB attach event. If nothing shows up, the cable is the problem — many mini-USB cables are charge-only and carry no data lines. Try a different cable or port.

**The controller retries endlessly and the bluetooth log shows `Refusing connection ... unknown device`.**
BlueZ has no record of this controller. Repeat step 4 in full, starting from the USB plug-in — a Bluetooth-only PS button press cannot register a new controller on its own.

**A PIN/password prompt appears every time the controller powers on.**
`ClassicBondedOnly` was not actually disabled. Confirm `/etc/bluetooth/input.conf` contains `ClassicBondedOnly=false` and that the bluetooth service was restarted afterward.

**`bluetoothctl info <MAC>` shows `Connected: yes`, but `/dev/input/js0` never appears.**
The kernel's `hid_sony` module can get stuck believing a controller MAC is already connected after a previous session didn't disconnect cleanly (check `dmesg | tail` for `failed to claim input`). Clear it by reloading the module, then press the PS button again:
```
sudo rmmod hid_sony
sudo modprobe hid_sony
```

**Do not run `bluetoothctl remove <MAC>` on a controller that is already working.**
This deletes the registration the `sixaxis` plugin depends on and forces you to redo step 4 from the beginning. It is not a harmless reset for these controllers, unlike for standard Bluetooth peripherals.