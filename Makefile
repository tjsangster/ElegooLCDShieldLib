CXX=avr-g++
CXXFLAGS=--std=c++14 -mmcu=${MCU} -DF_CPU=${F_CPU} -Os -fno-threadsafe-statics
AVRDUDE=avrdude
OBJCOPY=avr-objcopy
BAUDRATE=19200
MCU=atmega2560
F_CPU=16000000UL
MCU_PARTNO=m2560
TARGET=test

all:
	${CXX} ${CXXFLAGS} ${TARGET}.cpp lcdshield.cpp -o ${TARGET} 
	${OBJCOPY} -O ihex ${TARGET} ${TARGET}.hex

install:
	${AVRDUDE} -c wiring -p ${MCU_PARTNO} -U flash:w:${TARGET}.hex:i -D -P /dev/ttyACM0 

clean:
	rm ${TARGET} ${TARGET}.hex