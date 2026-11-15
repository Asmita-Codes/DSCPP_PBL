#ifndef SDLINC_H
#define SDLINC_H

#define SDL_MAIN_HANDLED          
#if defined(__has_include)
#  if __has_include(<SDL.h>)
#    include <SDL.h>
#    include <SDL_ttf.h>
#  else
#    include <SDL2/SDL.h>
#    include <SDL2/SDL_ttf.h>
#  endif
#else
#  include <SDL.h>
#  include <SDL_ttf.h>
#endif

#endif
