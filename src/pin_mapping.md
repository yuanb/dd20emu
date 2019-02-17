FPGA:  GPIO_1,  
DE1:  JP2  

Address bus:  
GPIO_1 25 24 23 22 21 20 19 18  
A      7  6  5  4  3  2  1  0  

Data bus:  
GPIO_1 17 16 15 14 13 12 11 10  
D      7  6  5  4  3  2  1  0  

/IORQ : GPIO_1:30, JP2:35  
/WR : GPIO_1:28, JP2:33 : Raspi_pin:8,  Raspi_GPIO14  
/RD : GPIO_1:29, JP2:34 : Raspi_pin:10, Raspi_GPIO15  

GND : JP2: 12, 30  

IO PORT:  
10H : 0001 0000  
11H : 0001 0001  
12H : 0001 0010  
13H : 0001 0011  

74AC138:  
A0: : Z80_A0 : GPIO_1:18, JP2:21  
A1: : Z80_A1 : GPIO_1:19, JP2:22  
A2: : Z80_A2 : GPIO_1:20, JP2:23  
/E1 : Z80_A3 : GPIO_1:21, JP2:24  
/E2 : Z80_/IORQ  
E3: : Z80_A4 : GPIO_1:22, JP2:25  

/O0(Y0) : Raspi_pin:12, Raspi_GPIO18  
/O1(Y1) : Raspi_pin:16, Raspi_GPIO23  
/O2(Y2) : Raspi_pin:18, Raspi_GPIO24  
/O3(Y3) : Raspi_pin:22, Raspi_GPIO25  
