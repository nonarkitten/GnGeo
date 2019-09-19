#ifndef FONT_H_
#define FONT_H_

typedef struct {
	unsigned char_width;
	unsigned char_height;
	const char * font_name;
	unsigned char first_char;
	unsigned char last_char;
	unsigned char * font_bitmap;
} font_t;

extern const font_t font_6x8;

#endif /* FONT_H_ */
