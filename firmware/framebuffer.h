#ifndef __FRAMEBUFFER_H__
#  define __FRAMEBUFFER_H__

#  include <stdint.h>

void    framebuffer_init (void);
volatile uint8_t *framebuffer_get_display (void);
void 	framebuffer_swap (void);

void    framebuffer_display_string_P (const char *string);
void    framebuffer_display_string (const char *string);
void    framebuffer_finish_line (void);

void    framebuffer_display_partial_char (const char c, const uint8_t offset);
void    framebuffer_display_char (const char c);
void    framebuffer_blank_column (const uint8_t columns);

#  define framebuffer_display_line( S ) framebuffer_display_string( S ); framebuffer_finish_line();
#  define framebuffer_display_line_P( S ) framebuffer_display_string_P( S ); framebuffer_finish_line();

void    framebuffer_display_string_center (const char *string);

#endif // __FRAMEBUFFER_H__
