# Copyright public licence and also I don't care.
# 2020 Josh "NeverCast" Lloyd.
from micropython import const
import micropython
from esp32 import RMT
import time

# The peripheral clock is 80MHz or 12.5 nanoseconds per clock.
# The smallest precision of timing requried for neopixels is
# 0.35us, but I've decided to go with 0.05 microseconds or
# 50 nanoseconds. 50 nanoseconds = 12.5 * 4 clocks.
# By dividing the 80MHz clock by 4 we get a clock every 50 nanoseconds.

# Neopixel timing in RMT clock counts.
T_0H = const(40 // 5) # 0.40 microseconds / 50 nanoseconds
T_0L = const(85 // 5) # 0.85 microseconds / 50 nanoseconds
T_1H = const(80 // 5) # 0.80 microseconds / 50 nanoseconds
T_1L = const(45 // 5) # 0.45 microseconds / 50 nanoseconds

# Encoded timings for bits 0 and 1.
D_ZERO = (T_0H, T_0L)
D_ONE = (T_1H, T_1L)

# [D_ONE if ((channel >> bit) & 1) else D_ZERO for channel in channels for bit in range(num_bits - 1, -1, -1)]
# Reset signal is low for longer than 50 microseconds.
#T_RST = const(510 // 5) # > 50 microseconds / 50 nanoseconds
T_RST = const(600 // 5) # > 50 microseconds / 50 nanoseconds

# Channel width in bits 
CHANNEL_WIDTH = const(8)

class Pixels:
    @micropython.native
    def __init__(self, pin, pixel_count, rmt_channel=1, pixel_channels=3):
        self.rmt = RMT(rmt_channel, pin=pin, clock_div=4)
        self.pixelPulseLen = CHANNEL_WIDTH*pixel_channels*2
        # pixels stores the data sent out via RMT
        self.pixels = D_ZERO * (pixel_channels * CHANNEL_WIDTH * pixel_count) + (T_RST,)
        # colors is only used for __getitem__

    @micropython.viper
    def write(self):
        # The bus should be idle low ( I think... )
        # So we finish low and start high.
        #pulses = tuple()
        #print("Before write: ")
        #print(self.pixels)
        #startTime=time.ticks_us()
        self.rmt.write_pulses(self.pixels, start=1)
        #self.rmt.write_pulses(self.pixelByteArr, start=1)
        #stopTime=time.ticks_us()
        #print ("RMT write took: " + str(stopTime-startTime))
        #print("after write; ")
        #print(self.pixels)

    #@micropython.viper
    def __setitem__(self, pixel_index, colors):
        #startTime=time.ticks_us()
        # pixels[0] = (r, g, b)
        pulseIndex = int(0)
        pxl = bytearray(self.pixelPulseLen)
        #print("comparing")
        for color in colors:
            #print("Color: " + str(color))
            colorInt=int(color)
            for bits in range(CHANNEL_WIDTH):
                #print(bin(colorInt))
                bit = colorInt & 128
                #print("Bit" + str(bit))
                if bit:
                    #print("1", end="")
                    pxl[pulseIndex] = T_1H
                    pxl[pulseIndex+1] = T_1L
                else:
                    #print("0", end="")
                    pxl[pulseIndex] = T_0H
                    pxl[pulseIndex+1] = T_0L
                colorInt = colorInt << 1
                pulseIndex += 2
        #print("")
        pxlTup = tuple(pxl)
        pulseIndex = int(pixel_index*self.pixelPulseLen)
        #self.insertPxl(pulseIndex, pxlTup)

        #print(pxlTup)
        #ref_pixel = tuple(clocks for bit in (D_ONE if ((channel >> bit) & 1) else D_ZERO for channel in colors for bit in range(CHANNEL_WIDTH - 1, -1, -1)) for clocks in bit)
        #cnt=0
        #while(cnt < len(ref_pixel)):
            #if ref_pixel[cnt] == 16 and ref_pixel[cnt+1] == 9 : print("1", end="")
            #elif ref_pixel[cnt] == 8 and ref_pixel[cnt+1] == 17 : print("0", end="")
            #else: print("x")
            #cnt +=2
        #print("")


        #clocks for bit in (
        #D_ONE if ((channel >> bit) & 1) 
        #else D_ZERO 
        #for channel in colors 
        #    for bit in range(CHANNEL_WIDTH - 1, -1, -1)):
        #        for clocks in bit:


        #for color in colors:
        #    colorInt=int(color)
        #    for bits in range(CHANNEL_WIDTH):
        #        bit = colorInt & 1
        #        if bit == 1:
        #            pxl = pxl + D_ONE
        #        else:
        #            pxl = pxl + D_ZERO
        #        colorInt = colorInt >> 1
        #self.insertPxl(pulseIndex, pxl)
        #self.pixels = self.pixels[:pulseIndex] + pxl + self.pixels[(pulseIndex+self.pixelPulseLen):]


        #self_pixels = self_pixels[:pulseIndex] + tuple(clocks for bit in (D_ONE if ((channel >> bit) & 1) else D_ZERO for channel in colors for bit in range(CHANNEL_WIDTH - 1, -1, -1)) for clocks in bit) + self_pixels[(pulseIndex+self.pixelPulseLen):]
        self.pixels = self.pixels[:pulseIndex] + tuple(clocks for bit in (D_ONE if ((channel >> bit) & 1) else D_ZERO for channel in colors for bit in range(CHANNEL_WIDTH - 1, -1, -1)) for clocks in bit) + self.pixels[(pulseIndex+self.pixelPulseLen):]
        #self.pixels = self_pixels
        #pixelLst = list(self.pixels)
        #pixelLst[pulseIndex:(pulseIndex+self.pixelPulseLen)] = list(clocks for bit in (D_ONE if ((channel >> bit) & 1) else D_ZERO for channel in colors for bit in range(CHANNEL_WIDTH - 1, -1, -1)) for clocks in bit)
        #self.pixels = tuple(pixelLst)
        #stopTime=time.ticks_us()
        #print ("__setitem__ took: " + str(stopTime-startTime))
        #print("after ´__setitems__")
        #print(self.pixels)

    @micropython.native
    def insertPxl(self, index, pxl):
        self.pixels = self.pixels[:index] + pxl + self.pixels[(index+self.pixelPulseLen):]
