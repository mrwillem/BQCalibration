################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/BQCalib.c \
../src/bqfloat.c \
../src/convert.c \
../src/serial.c 

OBJS += \
./src/BQCalib.o \
./src/bqfloat.o \
./src/convert.o \
./src/serial.o 

C_DEPS += \
./src/BQCalib.d \
./src/bqfloat.d \
./src/convert.d \
./src/serial.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/willem/linuxworkspace/BQ34Calibration/inc" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


