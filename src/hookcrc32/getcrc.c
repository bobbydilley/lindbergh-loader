#include <link.h>
#include <stdio.h>


uint32_t __crc32(const char *s, size_t n) {
  uint32_t crc = 0xFFFFFFFF;

  for (size_t i = 0; i < n; i++) {
    char ch = s[i];
    for (size_t j = 0; j < 8; j++) {
      uint32_t b = (ch ^ crc) & 1;
      crc >>= 1;
      if (b)
        crc = crc ^ 0xEDB88320;
      ch >>= 1;
    }
  }
  return ~crc;
}

int main(int argc, char *argv[]) 
{
  printf("You have entered %d arguments:\n", argc);

  for (int i = 0; i < argc; i++) {
    printf("%s\n", argv[i]);
  }
  FILE *f;
  char buffer[128];
  int crc = 0;
  f = fopen(argv[1], "r+b");
  fseek(f, 10, SEEK_SET);
  fread(buffer,128,1,f);
  crc = __crc32(buffer, 128);
  printf("Crc32: 0x%x\n", crc);

  return 0;
}
