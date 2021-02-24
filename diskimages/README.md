A few DD20 disk images,  
sector interlace  0, 11, 6, 1, 12, 7, 2, 13, 8, 3, 14, 9, 4, 15, 10, 5  

Format 1  
floppy1.dsk, floppy2.dsk, 98,560 bytes  

Sector length:  

Track size: 2464 bytes  

Format 2  
extbasic.dsk, hello.dsk, 99,184 bytes  
sector length  
0: 0x9b  
11: 0x99  
6: 0x9a  
1: 0x9a  
12:....  
5: 0x9a  

16 bytes of padding 0 between each track  
No padding at the end of last track  

Track size: 9b0/2480 bytes  

Format 3  
empty.dsk, 99,200 bytes  

Track size: 2480  
2480x40 = 99200  




