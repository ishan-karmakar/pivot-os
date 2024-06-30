define kc
	set variable wait = 0
	continue
end

file build/kernel.elf
target remote :1234
