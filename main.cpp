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
const int PiShowParams::SegmentHeight=10;
PiShowParams::PiShowParams()
  : ScreenWidth(0)
  , ScreenHeight(0)
  , CurrentTextureWidth(0)
  , CurrentTextureHeight(0)
  , OldTextureWidth(0)
  , OldTextureHeight(0)
  , ScreenAspectRatio(0.0)
  , CurrentTextureAspectRatio(0.0)
  , OldTextureAspectRatio(0.0)
  , Window(NULL)
  , CurrentTexture(NULL)
  , OldTexture(NULL)
  , CurrentStripe1Texture(NULL)
  , CurrentStripe2Texture(NULL)
  , OldStripe1Texture(NULL)
  , OldStripe2Texture(NULL)
  , Renderer(NULL)
  , CurrentPortraitMode(false)
  , OldPortraitMode(false)
{
}

// Berechnungsgrößen für die Monitoranzeige berechnen
void PiShowParams::CalcBorders()
{
  if (CurrentTexture==NULL)
    return;
  // Größe der beiden Texturen
  SDL_QueryTexture(CurrentTexture, NULL, NULL, &CurrentTextureWidth, &CurrentTextureHeight);
  if (OldTexture!=NULL)
    SDL_QueryTexture(OldTexture, NULL, NULL, &OldTextureWidth, &OldTextureHeight);

  CurrentTextureAspectRatio=double(CurrentTextureWidth)/CurrentTextureHeight;
  OldTextureAspectRatio=double(OldTextureWidth)/OldTextureHeight;

  CurrentPortraitMode=CurrentTextureHeight>CurrentTextureWidth;
  OldPortraitMode=OldTextureHeight>OldTextureWidth;

  // Unterscheidung: Bild hat einen größeren (breiteren) AspectRatio als der Monitor: => Streifen oben und unten
  if (CurrentTextureAspectRatio>ScreenAspectRatio)
  {
    CurrentRect.w = ScreenWidth;
    CurrentRect.h = double(ScreenWidth)/CurrentTextureAspectRatio;
    CurrentRect.x = 0;
    CurrentRect.y = (ScreenHeight-CurrentRect.h)/2;

    CurrentRectStripe1.w=ScreenWidth;
    CurrentRectStripe1.h=(ScreenHeight-CurrentRect.h)/2;
    CurrentRectStripe1.x=0;
    CurrentRectStripe1.y=0;

    CurrentRectStripe2.w=ScreenWidth;
    CurrentRectStripe2.h=(ScreenHeight-CurrentRect.h)/2;
    CurrentRectStripe2.x=0;
    CurrentRectStripe2.y=CurrentRect.y+CurrentRect.h;
  }
  else
  {
    // Monitor ist breiter als das Bild => Streifen links und rechts
    CurrentRect.w = double(ScreenHeight)*CurrentTextureAspectRatio;
    CurrentRect.h = ScreenHeight;
    CurrentRect.x = (ScreenWidth-CurrentRect.w)/2;
    CurrentRect.y = 0;

    CurrentRectStripe1.w=(ScreenWidth-CurrentRect.w)/2;
    CurrentRectStripe1.h=ScreenHeight;
    CurrentRectStripe1.x=0;
    CurrentRectStripe1.y=0;

    CurrentRectStripe2.w=(ScreenWidth-CurrentRect.w)/2;
    CurrentRectStripe2.h=ScreenHeight;
    CurrentRectStripe2.x=CurrentRect.x+CurrentRect.w;
    CurrentRectStripe2.y=0;
  }

  // Unterscheidung: Bild hat einen größeren (breiteren) AspectRatio als der Monitor: => Streifen oben und unten
  if (OldTextureAspectRatio>ScreenAspectRatio)
  {
    OldRect.w = ScreenWidth;
    OldRect.h = double(ScreenWidth)/OldTextureAspectRatio;
    OldRect.x = 0;
    OldRect.y = (ScreenHeight-OldRect.h)/2;

    OldRectStripe1.w=ScreenWidth;
    OldRectStripe1.h=(ScreenHeight-OldRect.h)/2;
    OldRectStripe1.x=0;
    OldRectStripe1.y=0;

    OldRectStripe2.w=ScreenWidth;
    OldRectStripe2.h=(ScreenHeight-OldRect.h)/2;
    OldRectStripe2.x=0;
    OldRectStripe2.y=OldRect.y+OldRect.h;

  }
  else
  {
    // Monitor ist breiter als das Bild => Streifen links und rechts
    OldRect.w = double(ScreenHeight)*OldTextureAspectRatio;
    OldRect.h = ScreenHeight;
    OldRect.x = (ScreenWidth-OldRect.w)/2;
    OldRect.y = 0;

    OldRectStripe1.w=(ScreenWidth-OldRect.w)/2;
    OldRectStripe1.h=ScreenHeight;
    OldRectStripe1.x=0;
    OldRectStripe1.y=0;

    OldRectStripe2.w=(ScreenWidth-OldRect.w)/2;
    OldRectStripe2.h=ScreenHeight;
    OldRectStripe2.x=OldRect.x+OldRect.w;
    OldRectStripe2.y=0;
  }
}

int main(int argc, char** argv)
{
  PiShowParams gParams;
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
      gParams.ScreenWidth=current.w;
      gParams.ScreenHeight=current.h;
      gParams.ScreenAspectRatio=double(gParams.ScreenWidth)/double(gParams.ScreenHeight);
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
  gParams.Window = SDL_CreateWindow("Test SDL 2", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                             gParams.ScreenWidth, gParams.ScreenHeight, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
  check_error_sdl(gParams.Window == NULL, "Unable to create window");

  // Create and initialize a hardware accelerated renderer that will be refreshed in sync with your monitor (at approx. 60 Hz)
  gParams.Renderer = SDL_CreateRenderer(gParams.Window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  check_error_sdl(gParams.Renderer == NULL, "Unable to create a renderer");

  // Set the default renderer color to corn blue
  //SDL_SetRenderDrawColor(renderer, 100, 149, 237, 255);
  // Query renderer info
  SDL_RendererInfo info;
  check_error_sdl( SDL_GetRendererInfo(gParams.Renderer, &info),"SDL_GetRendererInfo failed.");
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

  bool quit=false;
  do
  {
    ErrCount=0;
    for (int i = 0; i < parse.nonOptionsCount(); ++i)
    {
      try
      {
        gParams.CurrentFilename=parse.nonOption(i);

        LoadTextures(gParams.CurrentFilename,gParams);

        DoBlendEffect((BlendEffect)MyBlendEffect, gParams);

        quit= WaitAndCheckForQuit(SleepTime_s*1000);
        //sleep(SleepTime_s);

        if (gParams.OldTexture!=NULL)
          SDL_DestroyTexture(gParams.OldTexture);

        if (gParams.OldStripe1Texture!=NULL)
          SDL_DestroyTexture(gParams.OldStripe1Texture);

        if (gParams.OldStripe2Texture!=NULL)
          SDL_DestroyTexture(gParams.OldStripe2Texture);

        gParams.OldTexture=gParams.CurrentTexture;
        gParams.OldStripe1Texture=gParams.CurrentStripe1Texture;
        gParams.OldStripe2Texture=gParams.CurrentStripe2Texture;
        gParams.OldFilename=gParams.CurrentFilename;



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


  if (gParams.OldTexture!=NULL)
    SDL_DestroyTexture(gParams.OldTexture);
  SDL_DestroyRenderer(gParams.Renderer);
  SDL_DestroyWindow(gParams.Window);
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
void LoadTextures(const std::string aFileName, PiShowParams& aParams)
{
  int Height=0;
  int Width=0;
  int RawDataLength=0;
  unsigned char* pImage=NULL;
  SDL_Surface *image=NULL;
  SDL_Surface* formattedSurface = NULL;
  SDL_Surface *Stripe1=NULL;
  SDL_Surface *Stripe2=NULL;

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
    Uint32 format = SDL_GetWindowPixelFormat(aParams.Window);
    if (format==SDL_PIXELFORMAT_UNKNOWN)
      throw runtime_error( "Unable to get pixel format! SDL Error: " + string(SDL_GetError() ));
    //Convert surface to display format
//    WindowSurface=SDL_GetWindowSurface( gWindow );
    //  if( WindowSurface == NULL )
    //  throw runtime_error( "Unable to get window surface! SDL Error: " + string(SDL_GetError() ));
    formattedSurface = SDL_ConvertSurfaceFormat( image, format, 0 );

    if( formattedSurface == NULL )
      throw runtime_error( "Unable to convert loaded surface to display format! SDL Error: " + string(SDL_GetError() ));

    aParams.CurrentTexture = SDL_CreateTexture( aParams.Renderer, SDL_GetWindowPixelFormat( aParams.Window ), SDL_TEXTUREACCESS_STREAMING , formattedSurface->w, formattedSurface->h );
    if( aParams.CurrentTexture == NULL )
      throw runtime_error("Unable to create aParams.CurrentTexture texture! SDL Error: " +string(SDL_GetError() ));

    //Lock texture for manipulation
    check_error_sdl(SDL_LockTexture( aParams.CurrentTexture, NULL, &mPixels, &mPitch ),"SDL_LockTexture");
    //Copy loaded/formatted surface pixels
    memcpy( mPixels, formattedSurface->pixels, formattedSurface->pitch * formattedSurface->h );
    //Unlock texture to update
    SDL_UnlockTexture( aParams.CurrentTexture );
    //aParams.CurrentTexture = SDL_CreateTextureFromSurface(renderer, image);
    //check_error_sdl_img(aParams.CurrentTexture == NULL, "Unable to create a texture from the image");

    // Bildränder berechnen
    aParams.CalcBorders();
    // Texturen für die Ränder anlegen, die dann mit blur unscharf dargestellt werden
    Stripe1 = SDL_CreateRGBSurface(0, aParams.CurrentRectStripe1.w, aParams.CurrentRectStripe1.h, 32,  0, 0, 0, 0);
    check_error_sdl(Stripe1 == NULL, "SDL_CreateRGBSurface stripe1 failed");
    Stripe2 = SDL_CreateRGBSurface(0, aParams.CurrentRectStripe2.w, aParams.CurrentRectStripe2.h, 32,  0, 0, 0, 0);
    check_error_sdl(Stripe2 == NULL, "SDL_CreateRGBSurface stripe2 failed");

    // Textur oben bzw. links
    SDL_Rect SrcRectStripe1;
    if (aParams.CurrentPortraitMode)
    {
      SrcRectStripe1.w=min(formattedSurface->w,int(formattedSurface->h*double(aParams.CurrentRectStripe1.w)/double(aParams.CurrentRectStripe1.h)));
      SrcRectStripe1.h=formattedSurface->h;
      SrcRectStripe1.x=0;
      SrcRectStripe1.y=0;
    }
    else
    {
      SrcRectStripe1.w=formattedSurface->w;
      SrcRectStripe1.h=min(formattedSurface->h,int(formattedSurface->w*double(aParams.CurrentRectStripe1.h)/double(aParams.CurrentRectStripe1.w)));
      SrcRectStripe1.x=0;
      SrcRectStripe1.y=0;
    }
    SDL_BlitScaled(formattedSurface, &SrcRectStripe1, Stripe1, NULL);
    BlurSurface(Stripe1, 10);
    aParams.CurrentStripe1Texture = SDL_CreateTextureFromSurface(aParams.Renderer, Stripe1);
    check_error_sdl(aParams.CurrentStripe1Texture == NULL, "SDL_CreateTextureFromSurface aParams.CurrentStripe1Texture failed.");

    // Textur unten bzw. rechts
    SDL_Rect SrcRectStripe2;
    if (aParams.CurrentPortraitMode)
    {
      SrcRectStripe2.w=min(formattedSurface->w,int(formattedSurface->h*double(aParams.CurrentRectStripe2.w)/double(aParams.CurrentRectStripe2.h)));
      SrcRectStripe2.h=formattedSurface->h;
      SrcRectStripe2.x=formattedSurface->w-SrcRectStripe2.w;
      SrcRectStripe2.y=0;
    }
    else
    {
      SrcRectStripe2.w=formattedSurface->w;
      SrcRectStripe2.h=min(formattedSurface->h,int(formattedSurface->w*double(aParams.CurrentRectStripe2.h)/double(aParams.CurrentRectStripe2.w)));
      SrcRectStripe2.x=0;
      SrcRectStripe2.y=formattedSurface->h-SrcRectStripe2.h;
    }
    SDL_BlitScaled(formattedSurface, &SrcRectStripe2, Stripe2, NULL);
    BlurSurface(Stripe2, 10);
    aParams.CurrentStripe2Texture = SDL_CreateTextureFromSurface(aParams.Renderer, Stripe2);
    check_error_sdl(aParams.CurrentStripe2Texture == NULL, "SDL_CreateTextureFromSurface aParams.CurrentStripe2Texture failed.");

  }
  catch (exception & Err)
  {
    if (image!=NULL)
      SDL_FreeSurface(image);
    if( formattedSurface != NULL )
      SDL_FreeSurface(formattedSurface);
    if( Stripe1 != NULL )
      SDL_FreeSurface(Stripe1);
    if( Stripe2 != NULL )
      SDL_FreeSurface(Stripe2);
    if (pImage!=NULL)
      delete[] pImage;
    throw;
  }
  SDL_FreeSurface(image);
  SDL_FreeSurface(formattedSurface);
  delete[] pImage;
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

void DoBlendEffect(BlendEffect aEffect, PiShowParams &aParams)
{
  switch(aEffect)
  {
    case ZoomInOut:
      {
        const int NumSteps=300;
        for (int i=0; i<=NumSteps; i++)
        {
          aParams.CurrentRect.w = aParams.CurrentTextureWidth*i/double(NumSteps);
          aParams.CurrentRect.h = aParams.CurrentTextureHeight*i/double(NumSteps);

          aParams.OldRect.w = aParams.OldTextureWidth*(1-i/double(NumSteps));/*+1920*10/double(NumSteps)*/;
          aParams.OldRect.h = aParams.OldTextureHeight*(1-i/double(NumSteps));/*+1200*10/double(NumSteps)*/;
          aParams.OldRect.x = aParams.OldTextureWidth*(i/double(NumSteps))-1;
          aParams.OldRect.y = aParams.OldTextureHeight*(i/double(NumSteps))-1;

          // Clear the window content (using the default renderer color)
          //SDL_RenderClear(aRenderer);

          // Copy the texture on the renderer
          if (aParams.OldTexture!=NULL)
            SDL_RenderCopy(aParams.Renderer, aParams.OldTexture, NULL, &aParams.OldRect);
          // Copy the texture on the renderer
          SDL_RenderCopy(aParams.Renderer, aParams.CurrentTexture, NULL, &aParams.CurrentRect);

          // Update the window surface (show the renderer)
          SDL_RenderPresent(aParams.Renderer);

          SDL_Delay(10);
        }
        break;
      }
    case AlphaBlending:
      {
        check_error_sdl(SDL_SetTextureBlendMode(aParams.CurrentTexture, SDL_BLENDMODE_BLEND),"Setting alpha blend mode current texture");
        check_error_sdl(SDL_SetTextureBlendMode(aParams.CurrentStripe1Texture, SDL_BLENDMODE_BLEND),"Setting alpha blend mode CurrentStripe1Texture");
        check_error_sdl(SDL_SetTextureBlendMode(aParams.CurrentStripe2Texture, SDL_BLENDMODE_BLEND),"Setting alpha blend mode CurrentStripe2Texture");
        if (aParams.OldTexture!=NULL)
          check_error_sdl(SDL_SetTextureBlendMode(aParams.OldTexture, SDL_BLENDMODE_BLEND),"Setting alpha blend mode old texture");
        if (aParams.OldStripe1Texture!=NULL)
          check_error_sdl(SDL_SetTextureBlendMode(aParams.OldStripe1Texture, SDL_BLENDMODE_BLEND),"Setting alpha blend mode OldStripe1Texture");
        if (aParams.OldStripe2Texture!=NULL)
          check_error_sdl(SDL_SetTextureBlendMode(aParams.OldStripe2Texture, SDL_BLENDMODE_BLEND),"Setting alpha blend mode OldStripe2Texture");

        const int NumSteps=255;
        for (int i=0; i<=NumSteps; i+=2)
        {
          SDL_SetTextureAlphaMod( aParams.CurrentTexture, i );
          SDL_SetTextureAlphaMod( aParams.CurrentStripe1Texture, i );
          SDL_SetTextureAlphaMod( aParams.CurrentStripe2Texture, i );

          if (aParams.OldTexture!=NULL)
            SDL_SetTextureAlphaMod( aParams.OldTexture, NumSteps-i );
          if (aParams.OldStripe1Texture!=NULL)
            SDL_SetTextureAlphaMod( aParams.OldStripe1Texture, NumSteps-i );
          if (aParams.OldStripe2Texture!=NULL)
            SDL_SetTextureAlphaMod( aParams.OldStripe2Texture, NumSteps-i );

          if (aParams.OldTexture!=NULL)
            SDL_RenderCopy(aParams.Renderer, aParams.OldTexture, NULL, &aParams.OldRect);
          if (aParams.OldStripe1Texture!=NULL)
            SDL_RenderCopy(aParams.Renderer, aParams.OldStripe1Texture, NULL, &aParams.OldRectStripe1);
          if (aParams.OldStripe2Texture!=NULL)
            SDL_RenderCopy(aParams.Renderer, aParams.OldStripe2Texture, NULL, &aParams.OldRectStripe2);
          // Copy the texture on the renderer
          SDL_RenderCopy(aParams.Renderer, aParams.CurrentTexture, NULL, &aParams.CurrentRect);
          SDL_RenderCopy(aParams.Renderer, aParams.CurrentStripe1Texture, NULL, &aParams.CurrentRectStripe1);
          SDL_RenderCopy(aParams.Renderer, aParams.CurrentStripe2Texture, NULL, &aParams.CurrentRectStripe2);
          // Update the window surface (show the renderer)
          SDL_RenderPresent(aParams.Renderer);

          SDL_Delay(5);
        }
        break;
      }
    case AlphaMoving:
      {

        check_error_sdl(SDL_SetTextureBlendMode(aParams.CurrentTexture, SDL_BLENDMODE_BLEND),"Setting alpha blend mode current texture");
        if (aParams.OldTexture!=NULL)
          check_error_sdl(SDL_SetTextureBlendMode(aParams.OldTexture, SDL_BLENDMODE_BLEND),"Setting alpha blend mode current texture");

        unsigned char * aPixelsCurrent=NULL;
        int mPitchCurrent=0;
        check_error_sdl(SDL_LockTexture(aParams.CurrentTexture,NULL, (void**)&aPixelsCurrent, &mPitchCurrent),"SDL_LockTexture");
        const int NumBytesCurrent=mPitchCurrent*aParams.CurrentTextureHeight;
        const int Loops=NumBytesCurrent/2;

        for (int i=0; i<Loops; i+=4)
          aPixelsCurrent[i+3]=127;


        SDL_UnlockTexture(aParams.CurrentTexture);

        long long start=GetTime_us();
        /*
        for (int y=0;y<ScreenHeight;y++)
          for (int x=0;x<ScreenWidth;x++)
            SDL_RenderDrawPoint(aRenderer,x,y);
            */
        long long ende=GetTime_us();

        unsigned char * aPixelsOld=NULL;
        if (aParams.OldTexture!=NULL)
        {
          int mPitchOld=0;
          check_error_sdl(SDL_LockTexture(aParams.OldTexture,NULL, (void**)&aPixelsOld, &mPitchOld),"SDL_LockTexture");
          const int NumBytesOld=aParams.OldTextureHeight*mPitchOld;
          for (int i=0; i<NumBytesOld/2; i+=4)
            aPixelsOld[i+3]=127;
          SDL_UnlockTexture(aParams.OldTexture);
        }

        printf("Effektdauer :%.2f ms\r\n",double(ende-start)/1000.0);
        // Copy the texture on the renderer
        if (aParams.OldTexture!=NULL)
          SDL_RenderCopy(aParams.Renderer, aParams.OldTexture, NULL, &aParams.OldRect);
        SDL_RenderCopy(aParams.Renderer, aParams.CurrentTexture, NULL, &aParams.CurrentRect);

        // Update the window surface (show the renderer)
        SDL_RenderPresent(aParams.Renderer);
        break;
      }
    case MovingToRight:
      {
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
