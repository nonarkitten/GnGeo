#ifndef __LZ4W_H__
#define __LZ4W_H__

extern unsigned int lz4w_length(void *source);
extern unsigned int lz4w_unpack(void *source, void *destination);
extern void* lz4w_pack(void* source, int size, int* out_size);

#endif