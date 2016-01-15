#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <stdexcept>
#include "SdlTools.h"
#include <string>
#include <SDL2/SDL.h>

using namespace std;

// In case of error, print the error code and close the application
void check_error_sdl(bool check, const char* message)
{
  if (check)
  {
    string ErrorMsg=string(message)+" "+string(SDL_GetError());
    throw runtime_error(ErrorMsg);
  }
}


