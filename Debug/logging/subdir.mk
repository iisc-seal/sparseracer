################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../logging/Logger.cpp 

OBJS += \
./logging/Logger.o 

CPP_DEPS += \
./logging/Logger.d 


# Each subdirectory must supply rules for building sources it contributes
logging/%.o: ../logging/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I"/home/shalini/research/firefox/source/BugDetection" -O0 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


