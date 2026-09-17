#ifndef ANIMATIONS_H
#define ANIMATIONS_H

void fade_in_texture(unsigned int fadeSpeed, Color *color, bool *is_animation_finished);
void fade_out_texture(unsigned int fadeSpeed, Color *color, bool *is_animation_finished);

#endif // ANIMATIONS_H