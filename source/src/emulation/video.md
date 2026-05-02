ST7789
======

The ST7789 display controller uses command 0x36 (Memory Data Access Control or MADCTL) to define the orientation, rotation, and RGB/BGR color order. Commonly used values for 0x36 are 0x00 (portrait), 0x60 (landscape), 0xC0 (portrait flipped), and 0xA0 (landscape flipped).

* Bit 7 (MY):  Row Address Order      (0=Top-to-Bottom, 1=Bottom-to-Top)
* Bit 6 (MX):  Column Address Order   (0=Left-to-Right, 1=Right-to-Left)
* Bit 5 (MV):  Row/Column Exchange    (0=Normal, 1=Reverse)
* Bit 4 (ML):  Vertical Refresh Order (0=Refresh Top-to-Bottom, 1=Refresh Bottom-to-Top)
* Bit 3 (RGB): Color Order            (0=RGB, 1=BGR)
* Bit 2 (MH): Horizontal Refresh Order

Portrait (0x00):  Normal, Left-to-Right, Top-to-Bottom
Landscape (0x60): Row/Column exchange (MV) + Column order (MX)
Inverted/Flipped (0xC0): Row order (MY) + Column order (MX)

ILI9341
=======

The ILI9341 0x36 register, known as Memory Access Control (MADCTL), defines the pixel read/write scanning direction (portrait/landscape and screen orientation). Typical values set by writing to this address are 0x48 (Portrait), 0x28 (Landscape), 0x88 (Portrait 180°), or 0xE8 (Landscape 180°)

Common 0x36 MADCTL Settings

* 0x48 (Portrait):       MY=0, MX=1, MV=0, ML=0, BGR=1
* 0x28 (Landscape 90°):  MY=0, MX=0, MV=1, ML=0, BGR=1
* 0x88 (Portrait 180°):  MY=1, MX=0, MV=0, ML=1, BGR=1
* 0xE8 (Landscape 270°): MY=1, MX=1, MV=1, ML=0, BGR=1

MADCTL (0x36) Bit Definition

* Bit 7 (MY): Row Address Order
* Bit 6 (MX): Column Address Order
* Bit 5 (MV): Row/Column Exchange (swaps Width/Height)
* Bit 4 (ML): Vertical Refresh Order
* Bit 3 (BGR): RGB-BGR Order
* Bit 2 (MH): Horizontal Refresh Order

