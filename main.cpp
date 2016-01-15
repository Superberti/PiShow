#include <SDL2/SDL.h>
#include <SDL2/SDL_opengles2.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <turbojpeg.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <assert.h>
#include <errno.h>
#include <stdexcept>
#include <sys/time.h>
#include <unistd.h>
#include "main.h"
#include "GaussianBlur.h"
#include "SdlTools.h"

using namespace std;

int gScreenWidth=1920;
int gScreenHeight=1200;
double gScreenAspectRatio=double(gScreenWidth)/double(gScreenHeight);
const int gSegmentHeight=10;
SDL_Window* gWindow = NULL;
string gOldFilename,gCurrentFilename;

int main(int argc, char** argv)
{
  const int DefaultSleepTime_s=5;
  int SleepTime_s=DefaultSleepTime_s;
  int DefaultBlendEffect=AlphaBlending;
  int MyBlendEffect=DefaultBlendEffect;
  // Kommandozeile auswerten
  argc-=(argc>0);
  argv+=(argc>0); // skip program name argv[0] if present
  option::Stats  stats(usage, argc, argv);
  option::Option options[stats.options_max], buffer[stats.buffer_max];
  option::Parser parse(usage, argc, argv, options, buffer);

  if (parse.error())
    return 1;

  if (options[HELP] || argc == 0)
  {
    option::printUsage(std::cout, usage);
    return 0;
  }

  //std::cout << "--plus count: " <<
  //options[PLUS].count() << "\n";
  if (options[TIME])
  {
    StringToInt(std::string(options[TIME].arg),SleepTime_s);
    std::cout << "Angegebene Zeit: "<<options[TIME].arg<<std::endl;
  }

  if (options[EFFECT])
  {
    StringToInt(std::string(options[EFFECT].arg),MyBlendEffect);
    if (MyBlendEffect>FirstEffect && MyBlendEffect<LastEffect)
    {
      std::cout << "Benutze Effekt Nr.: "<<options[EFFECT].arg<<std::endl;
    }
    else
      MyBlendEffect=DefaultBlendEffect;
  }

  bool DoLoop=options[LOOP];

  for (option::Option* opt = options[UNKNOWN]; opt; opt = opt->next())
    std::cout << "Unknown option: " << opt->name << "\n";

  for (int i = 0; i < parse.nonOptionsCount(); ++i)
    std::cout << "Non-option #" << i << ": " << parse.nonOption(i) << "\n";

  // Initialize SDL
  check_error_sdl(SDL_Init(SDL_INIT_VIDEO) != 0, "Unable to initialize SDL");

  SDL_DisplayMode current;

  // Get current display mode of all displays.
  for(int i = 0; i < SDL_GetNumVideoDisplays(); ++i)
  {

    check_error_sdl(SDL_GetCurrentDisplayMode(i, &current),"SDL_GetCurrentDisplayMode failed.");

    // On success, print the current display mode.
    printf("Display #%d: current display mode is %dx%dpx @ %dhz. \n", i, current.w, current.h, current.refresh_rate);
    if (i==0)
    {
      // Größen vom ersten Display übernehmen
      gScreenWidth=current.w;
      gScreenHeight=current.h;
      gScreenAspectRatio=double(gScreenWidth)/double(gScreenHeight);
    }

  }


  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);

  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
  //SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);


  // Create and initialize a 800x600 window
  gWindow = SDL_CreateWindow("Test SDL 2", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                             gScreenWidth, gScreenHeight, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
  check_error_sdl(gWindow == NULL, "Unable to create window");

  // Create and initialize a hardware accelerated renderer that will be refreshed in sync with your monitor (at approx. 60 Hz)
  SDL_Renderer* renderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  check_error_sdl(renderer == NULL, "Unable to create a renderer");

  // Set the default renderer color to corn blue
  //SDL_SetRenderDrawColor(renderer, 100, 149, 237, 255);
  // Query renderer info
  SDL_RendererInfo info;
  check_error_sdl( SDL_GetRendererInfo(renderer, &info),"SDL_GetRendererInfo failed.");
  printf("Renderer info:\r\n");
  printf("Renderer is a software fallback: %s\r\n",info.flags & SDL_RENDERER_SOFTWARE ? "YES" : "NO");
  printf("Renderer uses hardware acceleration: %s\r\n",info.flags & SDL_RENDERER_ACCELERATED ? "YES" : "NO");
  printf("Present is synchronized with the refresh rate: %s\r\n",info.flags & SDL_RENDERER_PRESENTVSYNC ? "YES" : "NO");
  printf("Renderer supports rendering to texture: %s\r\n",info.flags & SDL_RENDERER_TARGETTEXTURE ? "YES" : "NO");

  SDL_ShowCursor(SDL_DISABLE);
  /*
  // Initialize SDL_img
  int flags=IMG_INIT_JPG | IMG_INIT_PNG;
  int initted = IMG_Init(flags);
  check_error_sdl_img((initted & flags) != flags, "Unable to initialize SDL_image");

  // Load the image in a texture
  SDL_Texture *texture = load_texture("img_test.png", renderer);
  */

  int ErrCount=0;
  const int MaxErrCount=5;

  SDL_Texture *OldTexture=NULL;
  SDL_Texture *CurrentTexture=NULL;

  bool quit=false;
  do
  {
    ErrCount=0;
    for (int i = 0; i < parse.nonOptionsCount(); ++i)
    {
      try
      {
        gCurrentFilename=parse.nonOption(i);

        CurrentTexture=load_texture(gCurrentFilename,renderer);

        DoBlendEffect(renderer, (BlendEffect)MyBlendEffect, CurrentTexture, OldTexture);

        quit= WaitAndCheckForQuit(SleepTime_s*1000);
        //sleep(SleepTime_s);

        if (OldTexture!=NULL)
          SDL_DestroyTexture(OldTexture);

        OldTexture=CurrentTexture;
        gOldFilename=gCurrentFilename;

      }
      catch (exception& aErr)
      {
        ErrCount++;
        fprintf(stderr,"Error: %s\r\n",aErr.what());
      }
      if (ErrCount>=MaxErrCount)
      {
        fprintf(stderr,"Too many errors, leaving program...\r\n");
        break;  // zu viele Fehler...
      }

      if (quit)
        break;
    }
    if (ErrCount>=MaxErrCount)
      break;  // zu viele Fehler...
    if (quit)
      break;
  }
  while (DoLoop && !quit);


  if (OldTexture!=NULL)
    SDL_DestroyTexture(OldTexture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(gWindow);
  SDL_Quit();
  fprintf(stderr,"Programm normal beendet!\r\n");
  return 0;
}

bool WaitAndCheckForQuit(Uint32 aWaitTime_ms)
{
  bool quit=false;
  Uint32 start=SDL_GetTicks();
  while(SDL_GetTicks()-start < aWaitTime_ms && !quit)
  {
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      switch (event.type)
      {
        case SDL_QUIT:
          /* Quit */
          quit = true;
          break;
      }
    }
    SDL_Delay(10);
  }
  return quit;
}

// Load an image from "fname" and return an SDL_Texture with the content of the image
SDL_Texture* load_texture(const std::string aFileName, SDL_Renderer *renderer)
{
  int Height=0;
  int Width=0;
  int RawDataLength=0;
  unsigned char* pImage=NULL;
  SDL_Surface *image=NULL;
  SDL_Surface* formattedSurface = NULL;
  SDL_Texture *img_texture=NULL;

  void* mPixels=NULL;
  int mPitch=0;
  try
  {
    pImage=GetImage(aFileName,Width,Height,RawDataLength);
    // Ist das Bild größer als die maximale Texturgröße (2048x2048)?
    if (Width>2048 || Height>2048)
      throw runtime_error(strprintf("Image dimensions exceeding max. texture size (2048x2048). Width=%d Height=%d",Width,Height));
    image = SDL_CreateRGBSurfaceFrom(pImage,
                                     Width,
                                     Height,
                                     32,
                                     4*Width,
                                     0,
                                     0,
                                     0,
                                     0);

    check_error_sdl(image == NULL, "SDL_CreateRGBSurfaceFrom failed");
    Uint32 format = SDL_GetWindowPixelFormat(gWindow);
    if (format==SDL_PIXELFORMAT_UNKNOWN)
      throw runtime_error( "Unable to get pixel format! SDL Error: " + string(SDL_GetError() ));
    //Convert surface to display format
//    WindowSurface=SDL_GetWindowSurface( gWindow );
    //  if( WindowSurface == NULL )
    //  throw runtime_error( "Unable to get window surface! SDL Error: " + string(SDL_GetError() ));
    formattedSurface = SDL_ConvertSurfaceFormat( image, format, 0 );

    if( formattedSurface == NULL )
      throw runtime_error( "Unable to convert loaded surface to display format! SDL Error: " + string(SDL_GetError() ));

    img_texture = SDL_CreateTexture( renderer, SDL_GetWindowPixelFormat( gWindow ), SDL_TEXTUREACCESS_STREAMING , formattedSurface->w, formattedSurface->h );
    if( img_texture == NULL )
      throw runtime_error("Unable to create blank texture! SDL Error: " +string(SDL_GetError() ));

    //Lock texture for manipulation
    check_error_sdl(SDL_LockTexture( img_texture, NULL, &mPixels, &mPitch ),"SDL_LockTexture");
    //Copy loaded/formatted surface pixels
    memcpy( mPixels, formattedSurface->pixels, formattedSurface->pitch * formattedSurface->h );
    //Unlock texture to update
    SDL_UnlockTexture( img_texture );
    //img_texture = SDL_CreateTextureFromSurface(renderer, image);
    //check_error_sdl_img(img_texture == NULL, "Unable to create a texture from the image");
  }
  catch (exception & Err)
  {
    if (image!=NULL)
      SDL_FreeSurface(image);
    if( formattedSurface != NULL )
      SDL_FreeSurface(formattedSurface);
    if (pImage!=NULL)
      delete[] pImage;
    throw;
  }
  SDL_FreeSurface(image);
  SDL_FreeSurface(formattedSurface);
  delete[] pImage;
  return img_texture;
}

// Load an image from "fname" and return an SDL_Texture with the content of the image
vector<SDL_Texture*> LoadTextureStripes(const std::string aFileName, SDL_Renderer *renderer)
{
  int Height=0;
  int Width=0;
  int RawDataLength=0;
  unsigned char* pImage=NULL;
  SDL_Surface *image=NULL;
  vector<SDL_Texture *>img_textures;
  SDL_Texture *img_texture;

  int mPitch=0;

  try
  {
    pImage=GetImage(aFileName,Width,Height,RawDataLength);
    // Ist das Bild größer als die maximale Texturgröße (2048x2048)?
    if (Width>2048 || Height>2048)
      throw runtime_error(strprintf("Image dimensions exceeding max. texture size (2048x2048). Width=%d Height=%d",Width,Height));
    mPitch=4*Width;
    for (int y=0; y<Height-gSegmentHeight; y+=gSegmentHeight)
    {
      image = SDL_CreateRGBSurfaceFrom(pImage+y*mPitch,
                                       Width,
                                       gSegmentHeight,
                                       32,
                                       mPitch,
                                       0,
                                       0,
                                       0,
                                       0);

      check_error_sdl(image == NULL, "SDL_CreateRGBSurfaceFrom failed");

      img_texture = SDL_CreateTextureFromSurface(renderer, image);
      check_error_sdl(img_texture == NULL, "SDL_CreateTextureFromSurface failed.");
      SDL_FreeSurface(image);
      image=NULL;
      img_textures.push_back(img_texture);
    }
  }
  catch (exception & Err)
  {
    if (image!=NULL)
      SDL_FreeSurface(image);
    if (pImage!=NULL)
      delete[] pImage;
    throw;
  }
  if (image!=NULL)
    SDL_FreeSurface(image);
  delete[] pImage;
  return img_textures;
}

unsigned char* GetImage(const std::string aFileName, int & width, int & height, int & aRawDataLength)
{
  unsigned int _jpegSize=0;        // Größe der JPEG-Datei
  unsigned char* _compressedImage=NULL; // Speicherblock mit dem komprimierten Image

  int COLOR_COMPONENTS=4;
  int jpegSubsamp;
  width=0;
  height=0;
  unsigned char *buffer=NULL;
  tjhandle _jpegDecompressor=NULL;
  FILE *fp=NULL;
  unsigned int s=0;

  try
  {
    fp = fopen(aFileName.c_str(), "rb");
    if (fp==NULL)
    {
      int error=errno;
      std::string ErrorMsg=GetOsErrString(error);
      throw std::runtime_error(strprintf( "Could not load image file \"%s\": %s.",aFileName.c_str(),ErrorMsg.c_str()));
    }
    fseek(fp, 0L, SEEK_END);
    _jpegSize = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    printf ("Komprimierte Bildgröße: %d\r\n",_jpegSize);
    _compressedImage = new unsigned char[_jpegSize];
    assert(_compressedImage != NULL);
    s = fread(_compressedImage, 1, _jpegSize, fp);
    assert(s == _jpegSize);
    fclose(fp);
    fp=NULL;

    _jpegDecompressor = tjInitDecompress();
    tjDecompressHeader2(_jpegDecompressor, _compressedImage, _jpegSize, &width, &height, &jpegSubsamp);
    printf("Bildbreite: %d\r\n",width);
    printf("Bildhöhe: %d\r\n",height);
    printf ("Unkomprimierte Bildgröße: %d\r\n",width*height*COLOR_COMPONENTS);
    aRawDataLength=width*height*COLOR_COMPONENTS;
    buffer=new unsigned char[aRawDataLength];
    tjDecompress2(_jpegDecompressor, _compressedImage, _jpegSize, buffer, width, 0/*pitch*/, height, TJPF_BGRA, TJFLAG_FASTDCT);
    printf("Bild %s erfolgreich geladen.\r\n",aFileName.c_str());
  }
  catch (std::exception & err)
  {

  }
  if (fp!=NULL)
    fclose(fp);
  if (_jpegDecompressor!=NULL)
    tjDestroy(_jpegDecompressor);
  //delete[] buffer;
  delete[] _compressedImage;
  return buffer;
}

void DoBlendEffect(SDL_Renderer* aRenderer, BlendEffect aEffect, SDL_Texture* CurrentTexture, SDL_Texture* OldTexture)
{
  // Größe der beiden Texturen
  int CurrentTextureWidth=0;
  int CurrentTextureHeight=0;
  int OldTextureWidth=0;
  int OldTextureHeight=0;
  SDL_QueryTexture(CurrentTexture, NULL, NULL, &CurrentTextureWidth, &CurrentTextureHeight);
  if (OldTexture!=NULL)
    SDL_QueryTexture(OldTexture, NULL, NULL, &OldTextureWidth, &OldTextureHeight);
  /*
    int CurrentXOffset=(ScreenWidth-CurrentTextureWidth)/2;
    int CurrentYOffset=(ScreenHeight-CurrentTextureHeight)/2;
    int OldXOffset=(ScreenWidth-OldTextureWidth)/2;
    int OldYOffset=(ScreenHeight-OldTextureHeight)/2;

    SDL_Rect OldRect;
    OldRect.x = OldXOffset;
    OldRect.y = OldYOffset;
    OldRect.w = min(ScreenWidth,OldTextureWidth);
    OldRect.h = min(ScreenHeight,OldTextureHeight);
    SDL_Rect CurrentRect;
    CurrentRect.x = CurrentXOffset;
    CurrentRect.y = CurrentYOffset;
    CurrentRect.w = min(ScreenWidth,CurrentTextureWidth);
    CurrentRect.h = min(ScreenHeight,CurrentTextureHeight);
  */
  double CurrentTextureAspectRatio=double(CurrentTextureWidth)/CurrentTextureHeight;
  double OldTextureAspectRatio=double(OldTextureWidth)/OldTextureHeight;
  SDL_Rect OldRect;
  SDL_Rect CurrentRect;
  bool CurrentPortraitMode=CurrentTextureHeight>CurrentTextureWidth;
  bool OldPortraitMode=OldTextureHeight>OldTextureWidth;

  // Unterscheidung: Bild hat einen größeren (breiteren) AspectRation als der Monitor:
  if (CurrentTextureAspectRatio>gScreenAspectRatio)
  {
    CurrentRect.w = gScreenWidth;
    CurrentRect.h = double(gScreenWidth)/CurrentTextureAspectRatio;
    CurrentRect.x = 0;
    CurrentRect.y = (gScreenHeight-CurrentRect.h)/2;
  }
  else
  {
    // Monitor ist breiter als das Bild
    CurrentRect.w = gScreenHeight*CurrentTextureAspectRatio;
    CurrentRect.h = gScreenHeight;
    CurrentRect.x = (gScreenWidth-CurrentRect.w)/2;
    CurrentRect.y = 0;
  }

  // Unterscheidung: Bild hat einen größeren (breiteren) AspectRation als der Monitor:
  if (OldTextureAspectRatio>gScreenAspectRatio)
  {
    OldRect.w = gScreenWidth;
    OldRect.h = double(gScreenWidth)/OldTextureAspectRatio;
    OldRect.x = 0;
    OldRect.y = (gScreenHeight-OldRect.h)/2;
  }
  else
  {
    // Monitor ist breiter als das Bild
    OldRect.w = gScreenHeight*OldTextureAspectRatio;
    OldRect.h = gScreenHeight;
    OldRect.x = (gScreenWidth-OldRect.w)/2;
    OldRect.y = 0;
  }


  //bool quit=false;

  switch(aEffect)
  {
    case ZoomInOut:
      {
        const int NumSteps=300;
        for (int i=0; i<=NumSteps; i++)
        {
          CurrentRect.w = CurrentTextureWidth*i/double(NumSteps);
          CurrentRect.h = CurrentTextureHeight*i/double(NumSteps);

          OldRect.w = OldTextureWidth*(1-i/double(NumSteps));/*+1920*10/double(NumSteps)*/;
          OldRect.h = OldTextureHeight*(1-i/double(NumSteps));/*+1200*10/double(NumSteps)*/;
          OldRect.x = OldTextureWidth*(i/double(NumSteps))-1;
          OldRect.y = OldTextureHeight*(i/double(NumSteps))-1;

          // Clear the window content (using the default renderer color)
          //SDL_RenderClear(aRenderer);

          // Copy the texture on the renderer
          if (OldTexture!=NULL)
            SDL_RenderCopy(aRenderer, OldTexture, NULL, &OldRect);
          // Copy the texture on the renderer
          SDL_RenderCopy(aRenderer, CurrentTexture, NULL, &CurrentRect);

          // Update the window surface (show the renderer)
          SDL_RenderPresent(aRenderer);

          SDL_Delay(10);
        }
        break;
      }
    case AlphaBlending:
      {
        check_error_sdl(SDL_SetTextureBlendMode(CurrentTexture, SDL_BLENDMODE_BLEND),"Setting alpha blend mode current texture");
        if (OldTexture!=NULL)
          check_error_sdl(SDL_SetTextureBlendMode(OldTexture, SDL_BLENDMODE_BLEND),"Setting alpha blend mode current texture");
        const int NumSteps=255;
        for (int i=0; i<=NumSteps; i+=2)
        {
          SDL_SetTextureAlphaMod( CurrentTexture, i );

          if (OldTexture!=NULL)
            SDL_SetTextureAlphaMod( OldTexture, NumSteps-i );

          if (OldTexture!=NULL)
            SDL_RenderCopy(aRenderer, OldTexture, NULL, &OldRect);
          // Copy the texture on the renderer
          SDL_RenderCopy(aRenderer, CurrentTexture, NULL, &CurrentRect);
          // Update the window surface (show the renderer)
          SDL_RenderPresent(aRenderer);

          SDL_Delay(5);
        }
        break;
      }
    case AlphaMoving:
      {

        check_error_sdl(SDL_SetTextureBlendMode(CurrentTexture, SDL_BLENDMODE_BLEND),"Setting alpha blend mode current texture");
        if (OldTexture!=NULL)
          check_error_sdl(SDL_SetTextureBlendMode(OldTexture, SDL_BLENDMODE_BLEND),"Setting alpha blend mode current texture");

        unsigned char * aPixelsCurrent=NULL;
        int mPitchCurrent=0;
        check_error_sdl(SDL_LockTexture(CurrentTexture,NULL, (void**)&aPixelsCurrent, &mPitchCurrent),"SDL_LockTexture");
        const int NumBytesCurrent=mPitchCurrent*CurrentTextureHeight;
        const int Loops=NumBytesCurrent/2;

        for (int i=0; i<Loops; i+=4)
          aPixelsCurrent[i+3]=127;


        SDL_UnlockTexture(CurrentTexture);

        long long start=GetTime_us();
        /*
        for (int y=0;y<ScreenHeight;y++)
          for (int x=0;x<ScreenWidth;x++)
            SDL_RenderDrawPoint(aRenderer,x,y);
            */
        long long ende=GetTime_us();

        unsigned char * aPixelsOld=NULL;
        if (OldTexture!=NULL)
        {
          int mPitchOld=0;
          check_error_sdl(SDL_LockTexture(OldTexture,NULL, (void**)&aPixelsOld, &mPitchOld),"SDL_LockTexture");
          const int NumBytesOld=OldTextureHeight*mPitchOld;
          for (int i=0; i<NumBytesOld/2; i+=4)
            aPixelsOld[i+3]=127;
          SDL_UnlockTexture(OldTexture);
        }

        printf("Effektdauer :%.2f ms\r\n",double(ende-start)/1000.0);
        // Copy the texture on the renderer
        if (OldTexture!=NULL)
          SDL_RenderCopy(aRenderer, OldTexture, NULL, &OldRect);
        SDL_RenderCopy(aRenderer, CurrentTexture, NULL, &CurrentRect);

        // Update the window surface (show the renderer)
        SDL_RenderPresent(aRenderer);
        break;
      }
    case MovingToRight:
      {
        vector<SDL_Texture*> CurrentTextureStripes=LoadTextureStripes(gCurrentFilename, aRenderer);
        vector<SDL_Texture*> OldTextureStripes;
        for (Uint32 i=0; i<CurrentTextureStripes.size(); i++)
          check_error_sdl(SDL_SetTextureBlendMode(CurrentTextureStripes[i], SDL_BLENDMODE_BLEND),"Setting alpha blend mode current texture");

        if (!gOldFilename.empty())
        {
          OldTextureStripes=LoadTextureStripes(gOldFilename, aRenderer);
          for (Uint32 i=0; i<OldTextureStripes.size(); i++)
            check_error_sdl(SDL_SetTextureBlendMode(OldTextureStripes[i], SDL_BLENDMODE_BLEND),"Setting alpha blend mode old texture");
        }

        vector<unsigned char> AlphaVecCurrent(CurrentTextureStripes.size());
        vector<unsigned char> AlphaVecOld(OldTextureStripes.size());
        const Uint32 AlphaSteps=40;
        const Uint32 HalfAlphaSteps=AlphaSteps/2;
        double AlphaStart,AlphaEnd;

        for (Uint32 a=0; a<AlphaSteps; a++)
        {
          // Berechnung der Textur-Alphawerte
          if (a<HalfAlphaSteps)
          {
            AlphaStart=0;
            AlphaEnd=double(a)/HalfAlphaSteps*255;
          }
          else
          {
            AlphaStart=double(a-HalfAlphaSteps)/HalfAlphaSteps*255;
            AlphaEnd=255;
          }

          double da=double(a)/AlphaSteps;

          for (Uint32 i=0; i<CurrentTextureStripes.size(); i++)
          {
            if (double(i)/CurrentTextureStripes.size()<da+0.1)
            {
              AlphaVecCurrent.at(i)=255;
            }
            else if (double(i)/CurrentTextureStripes.size()>da+0.3)
            {
              AlphaVecCurrent.at(i)=0;
            }
            else
            {
              AlphaVecCurrent.at(i)=127;
            }
            //AlphaVecCurrent.at(i)=AlphaStart+(AlphaEnd-AlphaStart)*double(i)/CurrentTextureStripes.size();
          }
          for (Uint32 i=0; i<OldTextureStripes.size(); i++)
          {
            if (double(i)/OldTextureStripes.size()<da+0.1)
            {
              AlphaVecOld.at(i)=0;
            }
            else if (double(i)/OldTextureStripes.size()>da+0.3)
            {
              AlphaVecOld.at(i)=255;
            }
            else
            {
              AlphaVecOld.at(i)=127;
            }
            //AlphaVecCurrent.at(i)=AlphaStart+(AlphaEnd-AlphaStart)*double(i)/CurrentTextureStripes.size();
          }
          //for (Uint32 i=0; i<OldTextureStripes.size(); i++)
          //AlphaVecOld.at(i)=255-(AlphaStart+(AlphaEnd-AlphaStart)*double(i)/OldTextureStripes.size());


          for (Uint32 i=0; i<CurrentTextureStripes.size(); i++)
            SDL_SetTextureAlphaMod( CurrentTextureStripes[i], AlphaVecCurrent[i]/*double(a)/40.0*double(i)/double(CurrentTextureStripes.size())*256*/ );

          for (Uint32 i=0; i<OldTextureStripes.size(); i++)
            SDL_SetTextureAlphaMod( OldTextureStripes[i], AlphaVecOld[i]/*double(a)/40.0*double(i)/double(OldTextureStripes.size())*256*/ );

          for (Uint32 i=0; i<OldTextureStripes.size(); i++)
          {
            OldRect.x = 0;
            OldRect.y = i*gSegmentHeight;
            OldRect.w = gScreenWidth;
            OldRect.h = gSegmentHeight;

            SDL_RenderCopy(aRenderer, OldTextureStripes[i], NULL, &OldRect);
          }

          for (Uint32 i=0; i<CurrentTextureStripes.size(); i++)
          {
            CurrentRect.x = 0;
            CurrentRect.y = i*gSegmentHeight;
            CurrentRect.w = gScreenWidth;
            CurrentRect.h = gSegmentHeight;

            SDL_RenderCopy(aRenderer, CurrentTextureStripes[i], NULL, &CurrentRect);
          }
          SDL_RenderPresent(aRenderer);
          SDL_Delay(50);
        }
        //sleep(5);
        for (Uint32 i=0; i<CurrentTextureStripes.size(); i++)
          SDL_DestroyTexture(CurrentTextureStripes[i]);
        for (Uint32 i=0; i<OldTextureStripes.size(); i++)
          SDL_DestroyTexture( OldTextureStripes[i]);
        break;
      }
    case MovingToLeft:
      {
        break;
      }
    case Mosaic:
      {
        break;
      }
  }
}
