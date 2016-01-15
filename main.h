#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

#include <string>
#include "optionparser.h"
#include "tools.h"

enum BlendEffect
{
  FirstEffect,
  ZoomInOut,
  AlphaBlending,
  AlphaMoving,
  MovingToRight,
  MovingToLeft,
  Mosaic,
  LastEffect,
};

unsigned char* GetImage(std::string aFileName, int & width, int & height, int & aRawDataSize);

bool WaitAndCheckForQuit(Uint32 aWaitTime);
void DoBlendEffect(SDL_Renderer* aRenderer,BlendEffect aEffect, SDL_Texture* aCurrentTexture, SDL_Texture* aOldTexture);

// Load an image from "fname" and return an SDL_Texture with the content of the image
SDL_Texture* load_texture(const std::string fname, SDL_Renderer *renderer);
std::vector<SDL_Texture*> LoadTextureStripes(const std::string aFileName, SDL_Renderer *renderer);

struct Arg: public option::Arg
{
  static void printError(const char* msg1, const option::Option& opt, const char* msg2)
  {
    fprintf(stderr, "%s", msg1);
    fwrite(opt.name, opt.namelen, 1, stderr);
    fprintf(stderr, "%s", msg2);
  }

  static option::ArgStatus Unknown(const option::Option& option, bool msg)
  {
    if (msg) printError("Unknown option '", option, "'\n");
    return option::ARG_ILLEGAL;
  }

  static option::ArgStatus Required(const option::Option& option, bool msg)
  {
    if (option.arg != 0)
      return option::ARG_OK;

    if (msg) printError("Option '", option, "' requires an argument\n");
    return option::ARG_ILLEGAL;
  }

  static option::ArgStatus NonEmpty(const option::Option& option, bool msg)
  {
    if (option.arg != 0 && option.arg[0] != 0)
      return option::ARG_OK;

    if (msg) printError("Option '", option, "' requires a non-empty argument\n");
    return option::ARG_ILLEGAL;
  }

  static option::ArgStatus Numeric(const option::Option& option, bool msg)
  {
    char* endptr = 0;
    if (option.arg != 0 && strtol(option.arg, &endptr, 10)) {};
    if (endptr != option.arg && *endptr == 0)
      return option::ARG_OK;

    if (msg) printError("Option '", option, "' requires a numeric argument\n");
    return option::ARG_ILLEGAL;
  }
};

enum  optionIndex { UNKNOWN, HELP, TIME, LOOP, EFFECT };
const option::Descriptor usage[] =
{
  {
    UNKNOWN, 0,"", "",Arg::None, "USAGE: example [options]\n\n"
    "Options:"
  },
  {HELP,    0,"", "help",Arg::None,    "  --help  \tPrint usage and exit." },
  {TIME,    0,"t", "time",Arg::Numeric, "  --time, -t  \tTime between two images." },
  {LOOP,    0,"l", "loop",Arg::None,   "  --loop, -l \tLoop forever." },
  {EFFECT,  0,"e", "effect",Arg::Numeric, "  --effect, -e  \tUse effect number x to blend images." },
  {
    UNKNOWN, 0,"",  "",Arg::None, "\nExamples:\n"
    "  ./SdlTest -t 10 -e 1 file1 file2\n"
  },
  {0,0,0,0,0,0}
};

#endif // MAIN_H_INCLUDED
