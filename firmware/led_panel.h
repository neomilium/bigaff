void    led_panel_init (volatile uint8_t* (*framebuffer_get_display) (void));
void    led_panel_set_intensity (const uint8_t intensity);
uint8_t led_panel_get_intensity (void);
