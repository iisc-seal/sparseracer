################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../racedetector/HBGraph.cpp \
../racedetector/UAFDetector.cpp 

OBJS += \
./racedetector/HBGraph.o \
./racedetector/UAFDetector.o 

CPP_DEPS += \
./racedetector/HBGraph.d \
./racedetector/UAFDetector.d 


# Each subdirectory must supply rules for building sources it contributes
racedetector/%.o: ../racedetector/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I"../" -O0 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


