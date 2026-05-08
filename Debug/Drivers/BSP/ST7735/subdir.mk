################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/BSP/ST7735/font.c \
../Drivers/BSP/ST7735/logo.c \
../Drivers/BSP/ST7735/logo_128_160.c \
../Drivers/BSP/ST7735/logo_160_80.c \
../Drivers/BSP/ST7735/st7735.c \
../Drivers/BSP/ST7735/st7735_lcd.c \
../Drivers/BSP/ST7735/st7735_reg.c 

OBJS += \
./Drivers/BSP/ST7735/font.o \
./Drivers/BSP/ST7735/logo.o \
./Drivers/BSP/ST7735/logo_128_160.o \
./Drivers/BSP/ST7735/logo_160_80.o \
./Drivers/BSP/ST7735/st7735.o \
./Drivers/BSP/ST7735/st7735_lcd.o \
./Drivers/BSP/ST7735/st7735_reg.o 

C_DEPS += \
./Drivers/BSP/ST7735/font.d \
./Drivers/BSP/ST7735/logo.d \
./Drivers/BSP/ST7735/logo_128_160.d \
./Drivers/BSP/ST7735/logo_160_80.d \
./Drivers/BSP/ST7735/st7735.d \
./Drivers/BSP/ST7735/st7735_lcd.d \
./Drivers/BSP/ST7735/st7735_reg.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/BSP/ST7735/%.o Drivers/BSP/ST7735/%.su Drivers/BSP/ST7735/%.cyclo: ../Drivers/BSP/ST7735/%.c Drivers/BSP/ST7735/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../Drivers/BSP/Camera -I../Drivers/BSP/ST7735 -I../Middlewares/Third_Party/FatFs/src -I../Drivers/BSP/ST7789 -I../Drivers/BSP/SDcard -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-BSP-2f-ST7735

clean-Drivers-2f-BSP-2f-ST7735:
	-$(RM) ./Drivers/BSP/ST7735/font.cyclo ./Drivers/BSP/ST7735/font.d ./Drivers/BSP/ST7735/font.o ./Drivers/BSP/ST7735/font.su ./Drivers/BSP/ST7735/logo.cyclo ./Drivers/BSP/ST7735/logo.d ./Drivers/BSP/ST7735/logo.o ./Drivers/BSP/ST7735/logo.su ./Drivers/BSP/ST7735/logo_128_160.cyclo ./Drivers/BSP/ST7735/logo_128_160.d ./Drivers/BSP/ST7735/logo_128_160.o ./Drivers/BSP/ST7735/logo_128_160.su ./Drivers/BSP/ST7735/logo_160_80.cyclo ./Drivers/BSP/ST7735/logo_160_80.d ./Drivers/BSP/ST7735/logo_160_80.o ./Drivers/BSP/ST7735/logo_160_80.su ./Drivers/BSP/ST7735/st7735.cyclo ./Drivers/BSP/ST7735/st7735.d ./Drivers/BSP/ST7735/st7735.o ./Drivers/BSP/ST7735/st7735.su ./Drivers/BSP/ST7735/st7735_lcd.cyclo ./Drivers/BSP/ST7735/st7735_lcd.d ./Drivers/BSP/ST7735/st7735_lcd.o ./Drivers/BSP/ST7735/st7735_lcd.su ./Drivers/BSP/ST7735/st7735_reg.cyclo ./Drivers/BSP/ST7735/st7735_reg.d ./Drivers/BSP/ST7735/st7735_reg.o ./Drivers/BSP/ST7735/st7735_reg.su

.PHONY: clean-Drivers-2f-BSP-2f-ST7735

