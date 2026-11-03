# ===== NusaOS Makefile =====
CXX := g++
LD  := ld

CXXFLAGS := -m32 -std=c++17 -O2 -Wall -Wextra -ffreestanding -fno-exceptions -fno-rtti \
            -fno-stack-protector -fno-pic -fno-use-cxa-atexit -fno-threadsafe-statics \
            -mno-sse -mno-mmx -nostdlib -Isrc
ASFLAGS  := -m32
LDFLAGS  := -m elf_i386 -T linker.ld -z noexecstack

BUILD   := build
CPP_SRC := $(shell find src -name '*.cpp')
ASM_SRC := $(shell find src -name '*.S')
OBJS    := $(patsubst src/%.cpp,$(BUILD)/%.o,$(CPP_SRC)) \
           $(patsubst src/%.S,$(BUILD)/%.S.o,$(ASM_SRC))

all: $(BUILD)/kernel.elf

$(BUILD)/kernel.elf: $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

$(BUILD)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/%.S.o: src/%.S
	@mkdir -p $(dir $@)
	$(CXX) $(ASFLAGS) -c $< -o $@

# ISO berbasis GRUB (untuk demo)
iso: $(BUILD)/kernel.elf
	@mkdir -p $(BUILD)/iso/boot/grub
	cp $(BUILD)/kernel.elf $(BUILD)/iso/boot/kernel.elf
	cp grub/grub.cfg $(BUILD)/iso/boot/grub/grub.cfg
	grub-mkrescue -o $(BUILD)/nusaos.iso $(BUILD)/iso

# Jalankan cepat saat development
run: $(BUILD)/kernel.elf
	qemu-system-i386 -kernel $(BUILD)/kernel.elf -serial stdio

# Jalankan lewat ISO GRUB
run-iso: iso
	qemu-system-i386 -cdrom $(BUILD)/nusaos.iso -serial stdio

# Debug: terminal lain -> gdb build/kernel.elf, lalu (gdb) target remote :1234
debug: $(BUILD)/kernel.elf
	qemu-system-i386 -kernel $(BUILD)/kernel.elf -serial stdio -s -S

clean:
	rm -rf $(BUILD)

.PHONY: all iso run run-iso debug clean
