![](http://www.old-computers.com/museum/photos/vtech_laser310_side_s.jpg)
# dd20emu

This project is dedicated to create an Arduino UNO based Laser-200/310 DD20/DI40 Floppy drive interface and drive emulator.
Since at the moment this project is created, I only have a Laser-200 on hand, which has relative less amount of RAM (6K) on board. The design will include 16K (6264 SRAM x 2) memory expansion as well.

Devices I have:
* Laser 200, Basic version 1.1, 6KB RAM.
* Laser 310, Basic 2.0, 16K RAM.
* 64K RAM extension cartridge for Laser 310. 

Todo list:
* Collect documents regarding DD20 interface and Disk image format.
* Design a 16K SRAM extension card to Laser 200
* I have about 2.25 hours length of Laser 310 data tape. They are Demo, Utilities, Games, Basic program I wrote etc. I would like to convert them to vz files. Giving they are 30 years old, the recorder I used back then was not a standard data recorder, there is only a little hope. [This link](http://www.pagetable.com/?p=32) describes how data tape encoding works on Apple I.

2017-03-29<br />
[3ChipVZ](http://intertek00.customer.netspace.net.au/3ChipVZ/), Trying to recreate Laser200/310 with Teensy

2017-03-01<br />
With some patience, I manually decoded the Laser310 tape recording header in Audacity. Click on the images to see full pictures.Bit pulses are 1: three low/short pulses, 0: one low/short and one cycle of high/long pulses.<br />
Laser200/Laser310's cassette port operates at 600 bps Baud rate. This is a snapshot of the cassette waveforms found in VZ300 technical manual<br />
[![Laser 310 cassette waveforms](https://raw.githubusercontent.com/yuanb/dd20emu/master/site/images/cassette_waveforms.png)](https://raw.githubusercontent.com/yuanb/dd20emu/master/site/images/cassette_waveforms.png)
The chart above shows: 1.66ms/bit, which is 1000ms/1.66ms = 602.41bps. 0.277us/pulse, 50/50 duty cycle.<br />

It seems the Laser310 cassette program recording header is :<br />
0x80 - Leading bytes(many, about 3.61s, probably 252 bytes)<br />
0xFE - Sync bytes(5 bytes)<br />
0xF0 - File type : Basic, 0xF1 : Binary<br />
0xxx - File name (BUST OUT), up to 17 bytes<br />
0x00 - End of header, there is an extra *pause* after this byte<br /> 
0x7AE9 - Start Address, LSB first, MSB later<br />
......<br />
[![Manual decoded Laser310 game recording header](https://raw.githubusercontent.com/yuanb/dd20emu/master/site/images/bust_out_header.png)](https://raw.githubusercontent.com/yuanb/dd20emu/master/site/images/bust_out_header.png)

Here is a [ASCII Chart](http://www.bluesock.org/~willg/dev/ascii.html)

2017-02-20<br />
Created dd20emu project on github.
