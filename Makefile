VERSION = 0.97

TARGET_LIST = \
	'arm/stm32f401_blackpill' 'arm/stm32f411_blackpill' \
	'arm/stm32f407_discovery' 'arm/versatilepb' \
	'avr/atmega32' 'avr/atmega328p' 'avr/atmega2560' \
	'mips/hf-risc' 'riscv/hf-riscv' 'riscv/hf-riscv-e' \
	'riscv/hf-riscv-llvm' 'riscv/riscv32-qemu' 'riscv/riscv32-qemu-llvm' \
	'riscv/riscv64-qemu' 'riscv/riscv64-qemu-llvm'

#ARCH = none

SERIAL_BAUD=57600
SERIAL_DEVICE=/dev/ttyUSB0

SRC_DIR = .

BUILD_DIR = $(SRC_DIR)/build
BUILD_APP_DIR = $(BUILD_DIR)/app
BUILD_HAL_DIR = $(BUILD_DIR)/hal
BUILD_DRIVERS_DIR = $(BUILD_DIR)/drivers
BUILD_KERNEL_DIR = $(BUILD_DIR)/kernel
BUILD_TARGET_DIR = $(BUILD_DIR)/target

-include $(BUILD_TARGET_DIR)/target.mak
-include $(SRC_DIR)/arch/$(ARCH)/arch.mak
-include $(SRC_DIR)/drivers/drivers.mak
INC_DIRS += -I $(SRC_DIR)/include -I $(SRC_DIR)/include/lib \
	-I $(SRC_DIR)/drivers/bus/include -I $(SRC_DIR)/drivers/device/include \
	-I $(SRC_DIR)/arch/common
CFLAGS += -D__VER__=\"$(VERSION)\" #-DALT_ALLOCATOR

incl:
ifeq ('$(ARCH)', 'none')
	@echo "You must specify a target architecture (ARCH=arch/target)."
	@echo "Supported targets are: $(TARGET_LIST)"
else
	@echo "ARCH = $(ARCH)" > $(BUILD_TARGET_DIR)/target.mak
endif

serial:
	stty ${SERIAL_BAUD} raw cs8 -parenb -crtscts clocal cread ignpar ignbrk -ixon -ixoff -ixany -brkint -icrnl -imaxbel -opost -onlcr -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke -F ${SERIAL_DEVICE}

load: serial
	cat image.bin > $(SERIAL_DEVICE)

debug: serial
	cat ${SERIAL_DEVICE}

## kernel
ucx: incl hal libs ddrivers kernel
	mv *.o $(SRC_DIR)/build/kernel
	$(AR) $(ARFLAGS) $(BUILD_TARGET_DIR)/libucxos.a \
		$(BUILD_KERNEL_DIR)/*.o

kernel: timer.o message.o pipe.o semaphore.o ecodes.o syscall.o coroutine.o ucx.o main.o

main.o: $(SRC_DIR)/init/main.c
	$(CC) $(CFLAGS) $(SRC_DIR)/init/main.c
ucx.o: $(SRC_DIR)/kernel/ucx.c
	$(CC) $(CFLAGS) $(SRC_DIR)/kernel/ucx.c
coroutine.o:
	$(CC) $(CFLAGS) $(SRC_DIR)/kernel/coroutine.c
syscall.o: $(SRC_DIR)/kernel/syscall.c
	$(CC) $(CFLAGS) $(SRC_DIR)/kernel/syscall.c
ecodes.o: $(SRC_DIR)/kernel/ecodes.c
	$(CC) $(CFLAGS) $(SRC_DIR)/kernel/ecodes.c
semaphore.o: $(SRC_DIR)/kernel/semaphore.c
	$(CC) $(CFLAGS) $(SRC_DIR)/kernel/semaphore.c
pipe.o: $(SRC_DIR)/kernel/pipe.c
	$(CC) $(CFLAGS) $(SRC_DIR)/kernel/pipe.c
message.o: $(SRC_DIR)/kernel/message.c
	$(CC) $(CFLAGS) $(SRC_DIR)/kernel/message.c
timer.o: $(SRC_DIR)/kernel/timer.c
	$(CC) $(CFLAGS) $(SRC_DIR)/kernel/timer.c

libs: console.o libc.o dump.o malloc.o list.o queue.o

queue.o: $(SRC_DIR)/lib/queue.c
	$(CC) $(CFLAGS) $(SRC_DIR)/lib/queue.c
list.o: $(SRC_DIR)/lib/list.c
	$(CC) $(CFLAGS) $(SRC_DIR)/lib/list.c
malloc.o: $(SRC_DIR)/lib/malloc.c
	$(CC) $(CFLAGS) $(SRC_DIR)/lib/malloc.c
dump.o: $(SRC_DIR)/lib/dump.c
	$(CC) $(CFLAGS) $(SRC_DIR)/lib/dump.c
libc.o: $(SRC_DIR)/lib/libc.c
	$(CC) $(CFLAGS) $(SRC_DIR)/lib/libc.c
console.o: $(SRC_DIR)/lib/console.c
	$(CC) $(CFLAGS) $(SRC_DIR)/lib/console.c
		
## kernel + application link
link:
ifeq ('$(ARCH)', 'avr/atmega32')
	$(LD) $(LDFLAGS) -o $(BUILD_TARGET_DIR)/image.elf $(BUILD_APP_DIR)/*.o -L$(BUILD_TARGET_DIR) -lucxos
else ifeq ('$(ARCH)', 'avr/atmega328p')
	$(LD) $(LDFLAGS) -o $(BUILD_TARGET_DIR)/image.elf $(BUILD_APP_DIR)/*.o -L$(BUILD_TARGET_DIR) -lucxos
else ifeq ('$(ARCH)', 'avr/atmega2560')
	$(LD) $(LDFLAGS) -o $(BUILD_TARGET_DIR)/image.elf $(BUILD_APP_DIR)/*.o -L$(BUILD_TARGET_DIR) -lucxos
else 
	$(LD) $(LDFLAGS) -T$(LDSCRIPT) -Map $(BUILD_TARGET_DIR)/image.map -o $(BUILD_TARGET_DIR)/image.elf $(BUILD_APP_DIR)/*.o -L$(BUILD_TARGET_DIR) -lucxos
endif
	$(DUMP) --disassemble --reloc $(BUILD_TARGET_DIR)/image.elf > $(BUILD_TARGET_DIR)/image.lst
	$(DUMP) -h $(BUILD_TARGET_DIR)/image.elf > $(BUILD_TARGET_DIR)/image.sec
	$(DUMP) -s $(BUILD_TARGET_DIR)/image.elf > $(BUILD_TARGET_DIR)/image.cnt
	$(OBJ) -O binary $(BUILD_TARGET_DIR)/image.elf $(BUILD_TARGET_DIR)/image.bin
	$(OBJ) -R .eeprom -O ihex $(BUILD_TARGET_DIR)/image.elf $(BUILD_TARGET_DIR)/image.hex
	$(SIZE) $(BUILD_TARGET_DIR)/image.elf
	hexdump -v -e '4/1 "%02x" "\n"' $(BUILD_TARGET_DIR)/image.bin > $(BUILD_TARGET_DIR)/code.txt

## applications
ECB: rebuild
	$(CC) $(CFLAGS) -o $(BUILD_APP_DIR)/tdes_driver.o app/tdes_driver/tdes_driver.c
	$(CC) $(CFLAGS) -o $(BUILD_APP_DIR)/app_ECB.o app/tdes_driver/app_ECB.c
	@$(MAKE) --no-print-directory link
	
# CTR: rebuild
# 	$(CC) $(CFLAGS) -o $(BUILD_APP_DIR)/tdes_driver.o app/tdes_driver/tdes_driver.c
# 	$(CC) $(CFLAGS) -o $(BUILD_APP_DIR)/app_CTR.o app/tdes_driver/app_CTR.c
# 	@$(MAKE) --no-print-directory link

# CBC: rebuild
# 	$(CC) $(CFLAGS) -o $(BUILD_APP_DIR)/tdes_driver.o app/tdes_driver/tdes_driver.c
# 	$(CC) $(CFLAGS) -o $(BUILD_APP_DIR)/app_CBC.o app/tdes_driver/app_CBC.c
# 	@$(MAKE) --no-print-directory link	



# clean and rebuild rules
rebuild:
	find '$(BUILD_APP_DIR)' -type f -name '*.o' -delete

clean:
	find '$(BUILD_APP_DIR)' '$(BUILD_KERNEL_DIR)' -type f -name '*.o' -delete
	find '$(BUILD_TARGET_DIR)' -type f -name '*.o' -delete -o -name '*~' \
		-delete -o -name 'image.*' -delete -o -name 'code.*' -delete
	find '$(SRC_DIR)' -type f -name '*.o' -delete

veryclean: clean
	echo "ARCH = none" > $(BUILD_TARGET_DIR)/target.mak
	find '$(BUILD_TARGET_DIR)' -type f -name '*.a' -delete
