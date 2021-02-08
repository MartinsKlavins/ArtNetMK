# ArtNetMK
Art-Net library for supporting DMX512 over ethernet for Arduino and ESP boards. Alongside basic DMX512 packet support, here is some notable features.

### Notable features
* Auto-configuration and support for Class A network
* Discovery:
  - replying to ArtPoll with necessary (directed or limited) broadcast type
  - sending ArtPoll/ArtPollReply with limited broadcast
* Ability to send on selected destination IP or automatically created IP address list (stores 4 IPs)

### Technical details
Supported OpCodes: ArtDmx, ArtPoll, ArtPollReply, ArtNzs, ArtAddress, ArtIpProg
