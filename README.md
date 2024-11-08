# RTL8720dn-Deauther
![logo](https://github.com/user-attachments/assets/ab8ebf84-eee2-4298-8975-2e8dad13c1b3)

# CREDIT
#### A modified version of [tesa-klebeband RTL8720dn-Deauther](https://github.com/tesa-klebeband/RTL8720dn-Deauther).

Please refer to this document for more information.
#### Changes:
- Added CSS.
- Removed the reason input.
- Used a checkbox to choose the target instead of form input.
- Changed LED behaviors.

![preview](https://i.imgur.com/Z4QIp9m.jpeg)
![preview](https://i.imgur.com/pHEs8CX.jpeg)

# DISCLAIMER
This tool has been made for educational and testing purposes only. Any misuse or illegal activities conducted with the tool are strictly prohibited. I am **not** responsible for any consequences arising from the use of the tool, which is done at your own risk.
## Building
Building is done using the ArduinoIDE. Make sure you have added the board manager URL for Realtek MCUs and installed them in the board manager (see [here](https://www.amebaiot.com/en/amebad-bw16-arduino-getting-started/) for tutorial).
1) Clone this repo using `git clone https://github.com/n0tAv1ru5/RTL8720dn-5GHz-Wifi-Deauther.git`
2) Open .ino file the cloned folder in the Arduino IDE
3) Connect your board and click upload (hold the boot button, connect to your PC, and release the button then try upload again if the upload fails).
### Notes
Most RTL8720dn devboards have the upload and log serial ports isolated. Make sure to bridge **LOG_UART_TX** and **LP_UART_TX**, as well as **LOG_UART_RX** and **LP_UART_RX** during uploading.
![log](https://i.imgur.com/kcUu1Gn.png)
## Using RTL8720dn-Deauther
The RTL8720dn hosts a WiFi network with the name of `AF` and a password of `0123456789`. Connect to this network and type the IP of your RTL8720dn (typically **192.168.1.1**) into a webbrowser of a choice. You will see the following options:
* Rescan networks: Rescan and detect all WiFi networks in your area. After a successful scan, the networks are listed in the above table.
* Launch Deauth-Attack: Deauthenticates all clients connected to a network. Enter the network number from the table at the top and a reason code from the table at the bottom of the page. After that, click **Launch Deauth-Attack**.
* Stop Deauth-Attack: Stops and ongoing deauth attack. You probably have to reconnect to the Deauthers WiFi after starting an attack.
### Leds
The RTL8720dn-Deauther utilizes the RGB led that most devboards have. This is what the different colors indicate:
* Red: The system state. Lights up when the system is usable.
* Green: Lights up when a HTTP communication between a device and the Deauther is happening and device is scanning.
* Blue: Flashes when a deauth frame is being sent.
## License
All files within this repo are released under the GNU GPL V3 License as per the LICENSE file stored in the root of this repo.
