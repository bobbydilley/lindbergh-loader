#define _GNU_SOURCE
#include <dlfcn.h>
#include <link.h>
#include <stdio.h>
#include <stdlib.h>

static struct {
    ElfW(Addr) start, end;
} *segments;
static int n;
static int (*real_main)(int argc,char **argv);

uint32_t __crc32(const char *s,size_t n) {
	uint32_t crc=0xFFFFFFFF;
	
	for(size_t i=0;i<n;i++) {
		char ch=s[i];
		for(size_t j=0;j<8;j++) {
			uint32_t b=(ch^crc)&1;
			crc>>=1;
			if(b) crc=crc^0xEDB88320;
			ch>>=1;
		}
	}
	return ~crc;
}

static int callback(struct dl_phdr_info *info, size_t size, void *data) {
    /*n = info->dlpi_phnum;
    segments = malloc(n * sizeof *segments);
    for(int i = 0; i < n; ++i) {
        segments[i].start = info->dlpi_addr + info->dlpi_phdr[i].p_vaddr;
        segments[i].end = info->dlpi_addr + info->dlpi_phdr[i].p_vaddr + info->dlpi_phdr[i].p_memsz;
    }
    char *type;
    int p_type;
    printf("Name: \"%s\" (%d segments)\n", info->dlpi_name, info->dlpi_phnum);
    for (int j = 0; j < info->dlpi_phnum; j++) {
        p_type = info->dlpi_phdr[j].p_type;
        type =  (p_type == PT_LOAD) ? "PT_LOAD" :
                (p_type == PT_DYNAMIC) ? "PT_DYNAMIC" :
                (p_type == PT_INTERP) ? "PT_INTERP" :
                (p_type == PT_NOTE) ? "PT_NOTE" :
                (p_type == PT_INTERP) ? "PT_INTERP" :
                (p_type == PT_PHDR) ? "PT_PHDR" :
                (p_type == PT_TLS) ? "PT_TLS" :
                (p_type == PT_GNU_EH_FRAME) ? "PT_GNU_EH_FRAME" :
                (p_type == PT_GNU_STACK) ? "PT_GNU_STACK" :
                (p_type == PT_GNU_RELRO) ? "PT_GNU_RELRO" : NULL;

        printf("    %2d: [%14p; memsz:0x%7x; size:0x%7x] flags: 0x%x; ", j,
                (void *) (info->dlpi_addr + info->dlpi_phdr[j].p_vaddr),
                info->dlpi_phdr[j].p_memsz, info->dlpi_phdr[j].p_filesz ,info->dlpi_phdr[j].p_flags);
        if (type != NULL)
            printf("%s\n", type);
        else
            printf("[other (0x%x)]\n", p_type);*/
        if((info->dlpi_phnum >= 3) && (info->dlpi_phdr[2].p_type == PT_LOAD) && (info->dlpi_phdr[2].p_flags == 5))
        {
        //    printf("Aca voy a dumpear el codigo:\n");
            //FILE *file = fopen("dump.bin","w+b");
            
            //fwrite((void *)(info->dlpi_addr + info->dlpi_phdr[2].p_vaddr)+10,128,1,file);
            //fclose(file);
            int crc = 0;
            crc = __crc32((void *)(info->dlpi_addr + info->dlpi_phdr[2].p_vaddr+10),128);
            printf("Crc32: 0x%x\n",crc);
        }
    //}
    return 1;
}

__attribute__((__constructor__))
static void setup(void) {
    dl_iterate_phdr(callback, NULL);
    //real_main = dlsym(RTLD_NEXT, "main");
    //exit(0);
}

__attribute__((__destructor__))
static void teardown(void) {
    free(segments);
}
/*
__attribute__((__noinline__))
int main(int argc,char **argv) {
    ElfW(Addr) addr = (ElfW(Addr))__builtin_extract_return_addr(__builtin_return_address(0));
    for(int i = 0; i < n; ++i) {
        if(addr >= segments[i].start && addr < segments[i].end) {
            // Do Nothing
            return 0;
        }
    }
    return real_main(argc, argv);
}*/