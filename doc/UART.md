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

8bit  0 0 0   0 0 0 0 0
     ------- ----------
      CMD       DATA


Command Table
-------------

CMD Note | Code| Bin Code| DATA Note              |
--------------------------------------------------|
Unused   |  0  |  0b000  | Unused                 |
Init LED |  1  |  0b001  | Count 0-31             |
LED Mode |  2  |  0b010  | Mode ID choose         |
LED Color|  3  |  0b011  | Color ID choose        |
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
