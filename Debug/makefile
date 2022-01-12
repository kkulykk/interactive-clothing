################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include USB_HOST/Target/subdir.mk
-include USB_HOST/App/subdir.mk
-include Middlewares/ST/STM32_USB_Host_Library/Core/Src/subdir.mk
-include Middlewares/ST/STM32_USB_Host_Library/Class/CDC/Src/subdir.mk
-include Drivers/STM32F4xx_HAL_Driver/Src/subdir.mk
-include Core/Startup/subdir.mk
-include Core/Src/subdir.mk
-include Core/Components/wm8994/subdir.mk
-include Core/Components/ts3510/subdir.mk
-include Core/Components/stmpe811/subdir.mk
-include Core/Components/stmpe1600/subdir.mk
-include Core/Components/st7789h2/subdir.mk
-include Core/Components/st7735/subdir.mk
-include Core/Components/s5k5cag/subdir.mk
-include Core/Components/ov5640/subdir.mk
-include Core/Components/ov2640/subdir.mk
-include Core/Components/otm8009a/subdir.mk
-include Core/Components/nt35510/subdir.mk
-include Core/Components/mfxstm32l152/subdir.mk
-include Core/Components/lsm303dlhc/subdir.mk
-include Core/Components/lsm303agr/subdir.mk
-include Core/Components/ls016b8uy/subdir.mk
-include Core/Components/lis3dsh/subdir.mk
-include Core/Components/lis302dl/subdir.mk
-include Core/Components/l3gd20/subdir.mk
-include Core/Components/ili9341/subdir.mk
-include Core/Components/ili9325/subdir.mk
-include Core/Components/i3g4250d/subdir.mk
-include Core/Components/ft6x06/subdir.mk
-include Core/Components/exc7200/subdir.mk
-include Core/Components/cs43l22/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := project_version2
BUILD_ARTIFACT_EXTENSION := elf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
project_version2.elf \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
project_version2.list \

OBJCOPY_BIN += \
project_version2.bin \


# All Target
all: main-build

# Main-build Target
main-build: project_version2.elf secondary-outputs

# Tool invocations
project_version2.elf: $(OBJS) $(USER_OBJS) /home/bohdanm/Documents/Studying_year_2/term_1/Subjects/Principles_of_computer_organization/project_v2/interactive-clothing/STM32F411VETX_FLASH.ld makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-gcc -o "project_version2.elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m4 -T"/home/bohdanm/Documents/Studying_year_2/term_1/Subjects/Principles_of_computer_organization/project_v2/interactive-clothing/STM32F411VETX_FLASH.ld" --specs=nosys.specs -Wl,-Map="project_version2.map" -Wl,--gc-sections -static -u _printf_float --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -Wl,--start-group -lc -lm -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

project_version2.list: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "project_version2.list"
	@echo 'Finished building: $@'
	@echo ' '

project_version2.bin: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objcopy  -O binary $(EXECUTABLES) "project_version2.bin"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(SIZE_OUTPUT)$(OBJDUMP_LIST)$(EXECUTABLES)$(OBJS)$(S_DEPS)$(S_UPPER_DEPS)$(C_DEPS)$(OBJCOPY_BIN) project_version2.elf
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST) $(OBJCOPY_BIN)

fail-specified-linker-script-missing:
	@echo 'Error: Cannot find the specified linker script. Check the linker settings in the build configuration.'
	@exit 2

warn-no-linker-script-specified:
	@echo 'Warning: No linker script specified. Check the linker settings in the build configuration.'

.PHONY: all clean dependents fail-specified-linker-script-missing warn-no-linker-script-specified

-include ../makefile.targets
