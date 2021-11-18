ESP32_files = ampy -p /dev/ttyS2 ls
for file in ESP32_files:
    if file != "boot.py":
        ampy -p /dev/ttyS2 rm file


for lib in libdir:
    mpy-cross -march=xtensawin -o ../build/lib.mpy lib.py

cp scripts/day0.xml build/.
mpy-cross -march=xtensawin -o ../genJMRIdecoderClient.mpy genJMRIdecoderClient.py

for file in build:
    ampy -p /dev/ttyS2 put file


sudo minicom -D /dev/ttyS2
