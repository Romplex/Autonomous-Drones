F405_TARGETS   += $(TARGET)
FEATURES       += VCP SDCARD
HSE_VALUE       = 16000000

TARGET_SRC = \
            drivers/accgyro/accgyro_mpu6000.c \
            drivers/barometer/barometer_ms56xx.c \
            drivers/compass/compass_hmc5883l.c \
            drivers/compass/compass_qmc5883l.c \
            drivers/compass/compass_ist8310.c \
            drivers/compass/compass_ist8308.c \
            drivers/compass/compass_mag3110.c \
            drivers/max7456.c \
            drivers/light_ws2811strip.c \
            drivers/light_ws2811strip_stdperiph.c