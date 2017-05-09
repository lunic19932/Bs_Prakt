################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../consumer.c \
../fifo.c \
../main.c \
../producer1.c \
../producer2.c 

OBJS += \
./consumer.o \
./fifo.o \
./main.o \
./producer1.o \
./producer2.o 

C_DEPS += \
./consumer.d \
./fifo.d \
./main.d \
./producer1.d \
./producer2.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


