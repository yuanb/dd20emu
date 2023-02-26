/*
    DD-20 emulator
    Copyright (C) 2020,2023 https://github.com/yuanb/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef	_WRDATA_H_
#define	_WRDATA_H_

/* Port and bit for WRDATA ICP5*/
/* Could also use ICP4 (PL0), which is defined as digital pin 49*/
//#define PORT_WRDATA PINL   //PL1 used in wrdata.ino
//#define WR_DATA_BIT 1
//const byte wrDataPin = 48;

#define TIFR    TIFR5   // timer 5 flag register
#define TOV     TOV5    // overflow flag
#define OCF     OCF5A   // output compare flag
#define ICF     ICF5    // input capture flag
#define TCCRA   TCCR5A  // timer 5 control register A
//#define COMA1   COM5A1  // timer 5 output compare mode bit 1
//#define COMA0   COM5A0  // timer 5 output compare mode bit 0
#define TCCRB   TCCR5B  // timer 5 control register B
#define CS2     CS52    // timer 5 clock select bit 2
#define CS1     CS51    // timer 5 clock select bit 1
#define CS0     CS50    // timer 5 clock select bit 0
//#define WGM2    WGM52   // timer 5 waveform mode bit 2
//#define TCCRC   TCCR5C  // timer 5 control register C
//#define FOC     FOC5A   // force output compare flag
//#define OCR     OCR5A   // timer 5 output compare register
#define TCNT    TCNT5   // timer 5 counter
#define TIMSK   TIMSK5
#define ICNC    ICNC5   //noise canceler
#define ICES    ICES5   //input capture edge select: 1-rising, 0-falling
#define ICIE    ICIE5   //Input Capture Interupt enable

#define PULSETIME 1
#ifdef PULSETIME
#define WRBUF_SIZE    512
#else
#define WRBUF_SIZE    140 // 6+4+126+1+1+2
#endif

#endif	//_WRDATA_H_
