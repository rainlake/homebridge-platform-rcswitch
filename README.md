# homebridge-platform-rcswitch
[![NPM Version](https://img.shields.io/npm/v/homebridge-platform-wemo.svg)](https://www.npmjs.com/package/homebridge-platform-rcswitch)

RCSwitch plugin for the awesome  [Homebridge](https://github.com/nfarina/homebridge) project.

## Currently supports
- Etekcity Tap 5 port Power plug
- other 433 Mhz remote plugs should work.

# Installation

1. Install homebridge using: npm install -g homebridge
2. Install this plugin using: npm install -g homebridge-platform-rcswitch
3. Update your configuration file. See the sample below.

# Configuration

Configuration sample:

`send_pin`, `sniffer_pin` is the gpio pin you are using to send/receive signal. it is different than the physical pin you are using. see [wireingpi.com](http://wiringpi.com/pins/) for details.

`switches` is the list of the "buttons" codes on your remote. start without any switch configed, press the button on your remote, you should get your code in homebridge log console.


 ```javascript
{
    "bridge": {
        "name": "#####",
        "username": "",
        "port": 51826,
        "pin": ""
    },

    "description": "",

    "platforms": [
        {
          "platform": "RCSwitch",
          "name": "RCSwitch Platform",
          "send_pin": 0,
          "sniffer_pin": 2,
          "tolerance": 90,
          "switches": [
                {
                        "name" : "Zap Plug Port 1",
                        "on": {
                                "code":xxxxxx,
                                "pulse":188
                        },
                        "off": {
                                "code":xxxxxx,
                                "pulse":188
                        }
                }
          ]
        }
    ]
}

```

The module should work on raspberry pi. due to raspberry pi and linux is not real time os/device, you might get different result on different device/time.

I'm using [this 433Mhz kit from ebay, include MX-FS-03V and MX-05V](http://www.ebay.com/sch/i.html?_nkw=433Mhz+RF+Transmitter+Module+and+Receiver+Link+Kit+)

# Credits

Credit goes to
- [wireing pi](http://wiringpi.com/pins/)
- 433 control codes ported from [433Utils](https://github.com/ninjablocks/433Utils)
- [rfoutlet project](https://github.com/timleland/rfoutlet) and his [blog post](https://timleland.com/wireless-power-outlets/)
- [http://scottfrees.com/](http://scottfrees.com/) for his great tutorial for asynchronous call.

# License

Published under the MIT License.