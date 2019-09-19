#ifndef FONT_H_
#define FONT_H_

typedef struct {
	uint8_t_width;
	uint8_t_height;
	const char * font_name;
	uint8_t first_char;
	uint8_t last_char;
	uint8_t * font_bitmap;
} font_t;

extern const font_t font_6x8;

#endif /* FONT_H_ */
