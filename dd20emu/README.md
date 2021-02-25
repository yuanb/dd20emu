### Arduino UNO implementation  

~~~
Pin definitions, Arduino Uno
GND
4,  PD4,    RD Data
3,  PD3,    /WRReq
2,  PD2,    /EnDrv
A3, PC3,    Step3
A2, PC2,    Step2
A1, PC1,    Step1
A0, PC0,    Step0
~~~

Step0 is on PC0[Input]  
Step1 is on PC1[Input]  
Step2 is on PC2[Input]  
Step3 is on PC3[Input]  

/En Drv is on PD2[Input]  
/WR Req is on PD3[Input]  
RD Data is on PD4[Output]  

### Arduino Mega 2560 implementation  

~~~
Pin definitions, Arduino Mega2560
GND
4 ,  PG5,    RD Data
3,   PE5,    /WrReq
2,   PE4,    /EnDrv
A11, PK3,    Step3
A10, PK2,    Step2
A9,  PK1,    Step1
A8,  PK0,    Step0  
~~~

Step0 is on PK0[Input]  
Step1 is on PK1[Input]  
Step2 is on PK2[Input]  
Step3 is on PK3[Input]  

/En Drv is on PG2[Input]  
/WR Req is on PG3[Input]  
RD Data is on PG5[Output]  
