#define M400_ID 0x05F0

typedef struct channel {
   unsigned int offset;
   size_t nBytes;
   char value[2];
   float scaling;
   unsigned char precision;
   char units[4];
   char name[64];
} channel;
