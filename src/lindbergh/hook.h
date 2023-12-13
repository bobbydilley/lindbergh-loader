
uint32_t get_crc32(const char *s,size_t n);
void getCPUID();
typedef struct 
{
    unsigned ebx;
    unsigned edx;
    unsigned ecx;
}cpuvendor;
