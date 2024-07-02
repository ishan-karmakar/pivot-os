define kc
	set variable wait = 0
	continue
end

file build/debug/kernel.elf
target remote :1234
