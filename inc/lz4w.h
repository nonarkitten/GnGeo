#ifndef __LZ4W_H__
#define __LZ4W_H__

inline unsigned int lz4w_length(void *source) { return *(unsigned int*)source; }
extern unsigned int lz4w_unpack(void *source, void *destination);

#endif