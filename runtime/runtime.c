#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdint.h>
// #include <efi.h>

#include <stdio.h>

uintptr_t get_RT_physaddr(void) {
    struct stat st;
    stat("/sys/firmware/efi/runtime", &st);
    char addr_buf[11];
    addr_buf[10] = '\0';
    int fd = open("/sys/firmware/efi/runtime", O_RDONLY);
    read(fd, addr_buf, st.st_size);
    return strtol(addr_buf, NULL, 0);
}

void *map_devmem(uintptr_t phys_addr) {
    int mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    return mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, phys_addr);
}

int main(void) {
    printf("ramiel runtime v0.1\n^^^^^^^^^^^^^^^^^^^\n");

    uintptr_t runtime_physaddr = get_RT_physaddr();
    printf("found RT physical address @ 0x%lx...\n", runtime_physaddr);


}
