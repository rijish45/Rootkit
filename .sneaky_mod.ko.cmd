cmd_/home/rg239/Rootkit/sneaky_mod.ko := ld -r -m elf_x86_64 -z max-page-size=0x200000 -T ./scripts/module-common.lds --build-id  -o /home/rg239/Rootkit/sneaky_mod.ko /home/rg239/Rootkit/sneaky_mod.o /home/rg239/Rootkit/sneaky_mod.mod.o ;  true
