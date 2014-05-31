################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../parser/TraceParser.cpp 

OBJS += \
./parser/TraceParser.o 

CPP_DEPS += \
./parser/TraceParser.d 


# Each subdirectory must supply rules for building sources it contributes
parser/%.o: ../parser/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I"/home/shalini/research/firefox/source/BugDetection" -O0 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


