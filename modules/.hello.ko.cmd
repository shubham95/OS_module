cmd_/home/uchiha/os_project/modules/hello.ko := ld -r -m elf_x86_64  -z max-page-size=0x200000 -T ./scripts/module-common.lds  --build-id  -o /home/uchiha/os_project/modules/hello.ko /home/uchiha/os_project/modules/hello.o /home/uchiha/os_project/modules/hello.mod.o ;  true