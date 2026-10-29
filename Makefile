# =============================================================================
#  Smart Lamp - FreeRTOS trên STM32F103C8 (Blue Pill)
#  Project chính dùng Keil C (keil/SmartLamp.uvprojx). Makefile này để:
#   - chạy unit test trên máy tính (make test)
#   - biên dịch thử bằng Arm GNU Toolchain (arm-none-eabi-gcc) nếu không có Keil
#
#  make            biên dịch -> build/smart_lamp.elf / .bin / .hex
#  make flash      nạp qua ST-Link (st-flash)
#  make test       chạy unit test trên máy tính (gcc của máy)
#  make sim        kiểm thử tích hợp bằng mô phỏng FreeRTOS POSIX (Linux)
#  make clean
# =============================================================================
TARGET   := smart_lamp
BUILD    := build
CROSS    ?= arm-none-eabi-
CC       := $(CROSS)gcc
OBJCOPY  := $(CROSS)objcopy
SIZE     := $(CROSS)size

# ---- Nguồn theo từng tầng ----
APP_SRC  := main.c $(wildcard app/src/*.c)
MID_SRC  := $(wildcard middlewares/src/*.c) $(wildcard bsp/src/*.c)
DRV_SRC  := $(wildcard hal/src/*.c)
RTOS_SRC := middlewares/FreeRTOS/tasks.c middlewares/FreeRTOS/queue.c \
            middlewares/FreeRTOS/list.c middlewares/FreeRTOS/timers.c \
            middlewares/FreeRTOS/event_groups.c \
            middlewares/FreeRTOS/portable/GCC/ARM_CM3/port.c \
            middlewares/FreeRTOS/portable/MemMang/heap_4.c
SPL_SRC  := $(wildcard platform/StdPeriph_Driver/src/*.c) platform/cmsis/device/system_stm32f10x.c
ASM_SRC  := platform/startup/gcc/startup_stm32f10x_md.s
LDSCRIPT := platform/startup/gcc/stm32f103c8_flash.ld

INC := -Iapp/inc -Ibsp/inc -Imiddlewares/inc -Ihal/inc \
       -Imiddlewares/FreeRTOS/include -Imiddlewares/FreeRTOS/portable/GCC/ARM_CM3 \
       -Iplatform/cmsis/core -Iplatform/cmsis/device -Iplatform/StdPeriph_Driver/inc

MCU     := -mcpu=cortex-m3 -mthumb
DEFS    := -DUSE_STDPERIPH_DRIVER -DSTM32F10X_MD
CFLAGS  := $(MCU) $(DEFS) $(INC) -std=gnu11 -Os -g3 -ffunction-sections -fdata-sections \
           -Wall -Wextra -Wshadow -Wno-unused-parameter
LDFLAGS := $(MCU) -T$(LDSCRIPT) -Wl,--gc-sections -Wl,-Map=$(BUILD)/$(TARGET).map \
           --specs=nano.specs --specs=nosys.specs -Wl,--print-memory-usage

OBJS := $(addprefix $(BUILD)/,$(APP_SRC:.c=.o) $(MID_SRC:.c=.o) $(DRV_SRC:.c=.o) \
                              $(RTOS_SRC:.c=.o) $(SPL_SRC:.c=.o) $(ASM_SRC:.s=.o))

# Mã của hãng (SPL, CMSIS): tắt cảnh báo để chỉ thấy cảnh báo của code nhóm
$(BUILD)/platform/%.o: platform/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -w -MMD -MP -c $< -o $@

.PHONY: all flash test sim clean

all: $(BUILD)/$(TARGET).bin $(BUILD)/$(TARGET).hex
	@$(SIZE) $(BUILD)/$(TARGET).elf

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD)/%.o: %.s
	@mkdir -p $(dir $@)
	$(CC) $(MCU) -x assembler-with-cpp -c $< -o $@

$(BUILD)/$(TARGET).elf: $(OBJS) $(LDSCRIPT)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

$(BUILD)/$(TARGET).bin: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILD)/$(TARGET).hex: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

flash: $(BUILD)/$(TARGET).bin
	st-flash --reset write $< 0x08000000

# ---- Unit test chạy trên máy tính: chỉ các module thuần logic, không cần phần cứng ----
TEST_SRC := test/test_main.c app/src/lamp_fsm.c app/src/menu.c app/src/effects.c middlewares/src/xprintf.c
test:
	@mkdir -p $(BUILD)/test
	gcc -std=c11 -Wall -Wextra -Iapp/inc -Imiddlewares/inc $(TEST_SRC) -o $(BUILD)/test/run_tests
	./$(BUILD)/test/run_tests

# ---- Kiểm thử tích hợp: toàn bộ tầng Application + FreeRTOS POSIX port, phần cứng giả (chỉ chạy trên Linux) ----
POSIX := middlewares/FreeRTOS/portable/ThirdParty/GCC/Posix
SIM_SRC := test/sim/sim_main.c test/sim/sim_hw.c \
           app/src/app_ipc.c app/src/task_input.c app/src/task_lamp.c app/src/lamp_fsm.c app/src/menu.c \
           app/src/effects.c app/src/task_display.c app/src/task_log.c app/src/task_console.c app/src/task_monitor.c \
           middlewares/src/console.c bsp/src/ssd1306.c bsp/src/font5x7.c middlewares/src/xprintf.c \
           middlewares/FreeRTOS/tasks.c middlewares/FreeRTOS/queue.c middlewares/FreeRTOS/list.c \
           middlewares/FreeRTOS/timers.c middlewares/FreeRTOS/event_groups.c middlewares/FreeRTOS/portable/MemMang/heap_4.c \
           $(POSIX)/port.c $(POSIX)/utils/wait_for_event.c
SIM_INC := -Itest/sim/inc -Iapp/inc -Ibsp/inc -Imiddlewares/inc -Imiddlewares/FreeRTOS/include -I$(POSIX) -I$(POSIX)/utils
sim:
	@mkdir -p $(BUILD)/sim
	gcc -std=gnu11 -O1 -g -w $(SIM_INC) -Dcpu_monitor_timebase_get=cpu_monitor_timebase_get_hw \
	    -c middlewares/src/cpu_monitor.c -o $(BUILD)/sim/cpu_monitor.o
	gcc -std=gnu11 -O1 -g -w $(SIM_INC) $(SIM_SRC) $(BUILD)/sim/cpu_monitor.o -lpthread -o $(BUILD)/sim/run_sim
	./$(BUILD)/sim/run_sim

clean:
	rm -rf $(BUILD)

-include $(OBJS:.o=.d)
