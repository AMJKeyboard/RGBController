UART
====

 AVR307 - Half duplex UART using the USI Interface

CONFIG
======

+ SYSTEM_CLOCK 8000000
+ BAUDRATE 9600
+ DATA_BITS  8
+ START_BIT  1
+ STOP_BIT  1

PROTOCOL
========


Transfer command unit
---------------------
        low    hight
     <------- <------
     ________ _______
8bit  0 0 0 0 0 0 0 0
      X ----- -------
      H  CMD    DATA


Command Table
-------------

CMD Note | Code| Bin Code| DATA Note              |
--------------------------------------------------|
Unused   |  0  |  0b000  | Unused                 |
Init LED |  1  |  0b001  | Count 0-31             |
LED Color|  2  |  0b010  | Color ID choose        |
LED Mode |  2  |  0b011  | Mode ID choose         |
LED Light|  4  |  0b100  | Brightness Control 1-20|
FWVER    |  5  |  0b101  | Unused                 |
CMDExt   |  6  |  0b110  | to CommandExt Table    |
Unused   |  7  |  0b111  | Unused                 |


CommandExt Table
----------------

CMDExt DATA| Code|Bin Code | DATA Note                |
------------------------------------------------------|
Unused     |  0  | 0b00000 |                          |
LED ON     |  1  | 0b00001 |                          |
LED OFF    |  2  | 0b00010 |                          |
LED Refresh|  3  | 0b00011 |                          |
INT Hight  |  4  | 0b00100 |                          |
INT Low    |  5  | 0b00101 |                          |


Mode Table
----------

Name    | Code| Bin Code | Note                |
-----------------------------------------------|
Unused  |  0  |  0b00000 |                     |
        |  1  |  0b00001 | Brightness 1 level  |
        |  2  |  0b00010 | Brightness 2 level  |
        |  3  |  0b00011 | Brightness 3 level  |
        |  4  |  0b00100 | Breathing 1 level   |
        |  5  |  0b00101 | Breathing 2 level   |
        |  6  |  0b00110 | Breathing 3 level   |
        |  7  |  0b00111 | Random 1 level      |
        |  8  |  0b01000 | Random 2 level      |
        |  9  |  0b01001 | Random 3 level      |
        |  10 |  0b01010 | Turnover 1 level    |
        |  11 |  0b01011 | Turnover 2 level    |
        |  12 |  0b01100 | Turnover 3 level    |


Color
-----

### Single color

> white + Rainbow independent + Random Color
>
> 10 kinds


### Colorful

> Random rainbow + Ordered rainbow + Random Full Color 
>
> 3 kinds
