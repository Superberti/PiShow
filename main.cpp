#include <SDL2/SDL.h>
#include <SDL2/SDL_opengles2.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_ttf.h>
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
#include <dirent.h>
#include "TIRThread.h"

using namespace std;

const int PiShowParams::SegmentHeight=10;
int PiShowParams::ScreenWidth=0;
int PiShowParams::ScreenHeight=0;
double PiShowParams::ScreenAspectRatio=0.0;

PiShowParams::PiShowParams()
  : Window(NULL)
  , Renderer(NULL)
  , CurrentTexture(NULL)
  , OldTexture(NULL)
  , TextTexture(NULL)
{
}

void PiShowParams::Cleanup()
{
  delete CurrentTexture;
  CurrentTexture=NULL;
  delete OldTexture;
  OldTexture=NULL;
  if (TextTexture!=NULL)
  {
    SDL_DestroyTexture(TextTexture);
    TextTexture=NULL;
  }
  SDL_DestroyRenderer(Renderer);
  Renderer=NULL;
  SDL_DestroyWindow(Window);
  Window=NULL;

}


ImageTexture::ImageTexture()
  : TextureWidth(0)
  , TextureHeight(0)
  , TextureAspectRatio(0.0)
  , Texture(NULL)
  , Stripe1Texture(NULL)
  , Stripe2Texture(NULL)
  , PortraitMode(false)
{

}

ImageTexture::~ImageTexture()
{
  SDL_DestroyTexture(Texture);
  SDL_DestroyTexture(Stripe1Texture);
  SDL_DestroyTexture(Stripe2Texture);
}

// Berechnungsgrößen für die Monitoranzeige berechnen
void ImageTexture::CalcBorders()
{
  if (Texture==NULL)
    return;
  // Größe der beiden Texturen
  SDL_QueryTexture(Texture, NULL, NULL, &TextureWidth, &TextureHeight);

  TextureAspectRatio=double(TextureWidth)/TextureHeight;

  PortraitMode=TextureAspectRatio<PiShowParams::ScreenAspectRatio;

  // Unterscheidung: Bild hat einen größeren (breiteren) AspectRatio als der Monitor: => Streifen oben und unten
  if (TextureAspectRatio>PiShowParams::ScreenAspectRatio)
  {
    ScreenRect.w = PiShowParams::ScreenWidth;
    ScreenRect.h = double(PiShowParams::ScreenWidth)/TextureAspectRatio;
    ScreenRect.x = 0;
    ScreenRect.y = (PiShowParams::ScreenHeight-ScreenRect.h)/2;

    ScreenRectStripe1.w=PiShowParams::ScreenWidth;
    ScreenRectStripe1.h=(PiShowParams::ScreenHeight-ScreenRect.h)/2;
    ScreenRectStripe1.x=0;
    ScreenRectStripe1.y=0;

    ScreenRectStripe2.w=PiShowParams::ScreenWidth;
    ScreenRectStripe2.h=(PiShowParams::ScreenHeight-ScreenRect.h)/2;
    ScreenRectStripe2.x=0;
    ScreenRectStripe2.y=ScreenRect.y+ScreenRect.h;
  }
  else
  {
    // Monitor ist breiter als das Bild => Streifen links und rechts
    ScreenRect.w = double(PiShowParams::ScreenHeight)*TextureAspectRatio;
    ScreenRect.h = PiShowParams::ScreenHeight;
    ScreenRect.x = (PiShowParams::ScreenWidth-ScreenRect.w)/2;
    ScreenRect.y = 0;

    ScreenRectStripe1.w=(PiShowParams::ScreenWidth-ScreenRect.w)/2;
    ScreenRectStripe1.h=PiShowParams::ScreenHeight;
    ScreenRectStripe1.x=0;
    ScreenRectStripe1.y=0;

    ScreenRectStripe2.w=(PiShowParams::ScreenWidth-ScreenRect.w)/2;
    ScreenRectStripe2.h=PiShowParams::ScreenHeight;
    ScreenRectStripe2.x=ScreenRect.x+ScreenRect.w;
    ScreenRectStripe2.y=0;
  }

}

SDL_Texture * TestTexture=NULL;
std::auto_ptr<TIRThread> IRThread;

int main(int argc, char** argv)
{
  IRThread.reset(new TIRThread());
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
  bool DoRand=options[RANDOM];
  VerboseLogging=options[VERBOSE];

  for (option::Option* opt = options[UNKNOWN]; opt; opt = opt->next())
    std::cout << "Unknown option: " << opt->name << "\n";

  vector<string> iDirsOrFiles;

  for (int i = 0; i < parse.nonOptionsCount(); ++i)
    iDirsOrFiles.push_back(parse.nonOption(i));

  //vector<string> iFilesAlreadyDisplayed;
  //printf("Anzahl anzuzeigender Dateien: %d\r\n",iFilesToDisplay.size());
  //for (unsigned int i=0;i<iFilesToDisplay.size();i++)
  //printf("%s\r\n",iFilesToDisplay[i].c_str());
  //std::cout << "Non-option #" << i << ": " << parse.nonOption(i) << "\n";

  // Initialize SDL
  check_error_sdl(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0, "Unable to initialize SDL");

  SDL_DisplayMode current;

  // Get current display mode of all displays.
  for(int i = 0; i < SDL_GetNumVideoDisplays(); ++i)
  {

    check_error_sdl(SDL_GetCurrentDisplayMode(i, &current),"SDL_GetCurrentDisplayMode failed.");

    // On success, print the current display mode.
    DebugOut(strprintf("Display #%d: current display mode is %dx%dpx @ %dhz. \n", i, current.w, current.h, current.refresh_rate));
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

  SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "1" );
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
  DebugOut(strprintf("Renderer info:\r\n"));
  DebugOut(strprintf("Renderer is a software fallback: %s\r\n",info.flags & SDL_RENDERER_SOFTWARE ? "YES" : "NO"));
  DebugOut(strprintf("Renderer uses hardware acceleration: %s\r\n",info.flags & SDL_RENDERER_ACCELERATED ? "YES" : "NO"));
  DebugOut(strprintf("Present is synchronized with the refresh rate: %s\r\n",info.flags & SDL_RENDERER_PRESENTVSYNC ? "YES" : "NO"));
  DebugOut(strprintf("Renderer supports rendering to texture: %s\r\n",info.flags & SDL_RENDERER_TARGETTEXTURE ? "YES" : "NO"));

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

  int Action=0;
  //CreateTextTexture(gParams);
  vector<string> iAllFilesToDisplay;
  vector<string> iFilesToDisplay;
  //vector<string> iDisplayedFiles;
  do
  {

    iAllFilesToDisplay=ExpandFileNames(iDirsOrFiles);
    iFilesToDisplay.clear();
    srand (time(NULL));
    ErrCount=0;
    // Liste mit zufälliger Reihenfolge der Bilder aufbauen
    if (DoRand)
    {
      vector<string> iTmpDisplayedFiles;
      iTmpDisplayedFiles=iAllFilesToDisplay;
      while (iTmpDisplayedFiles.size()>0)
      {
        int i=rand() % iTmpDisplayedFiles.size();
        iFilesToDisplay.push_back(iTmpDisplayedFiles.at(i));
        iTmpDisplayedFiles.erase(iTmpDisplayedFiles.begin() + i);
      }
    }
    else
      iFilesToDisplay=iAllFilesToDisplay;

    bool DoPause=false;
    int CurrentImageNumber=0;
    bool DoNothing=false;
    printf("Zeige %d Bilder an.\r\n",iFilesToDisplay.size());

    while (CurrentImageNumber<(int)iFilesToDisplay.size())
    {
      try
      {
        if (!DoPause && !DoNothing)
        {
          printf("Lade Bildnummer: %d (%s)...",CurrentImageNumber,iFilesToDisplay.at(CurrentImageNumber).c_str());fflush(stdout);
          gParams.CurrentTexture=new ImageTexture();
          gParams.CurrentTexture->ImageFilename=iFilesToDisplay.at(CurrentImageNumber);

          LoadTextures(gParams);

          DoBlendEffect((BlendEffect)MyBlendEffect, gParams);

          if (gParams.OldTexture!=NULL)
          {
            delete gParams.OldTexture;
          }
          gParams.OldTexture=gParams.CurrentTexture;
          gParams.CurrentTexture=NULL;
          printf("OK\r\n");fflush(stdout);
        }


        fflush(stdout);

        bool DoNotTouchImageNumber=false;
        Action= WaitAndCheckForQuit(SleepTime_s*1000);
        DoNothing=false;
        if (Action==2)
        {
          // Befehl der IR-Fernbedienung empfangen
          TCritGuard cg(IRThread->IRCommandQueue.GetCritSec());
          while (IRThread->IRCommandQueue.GetUnsafe().size()>0)
          {
            IRCode NewCode=IRThread->IRCommandQueue.GetUnsafe().front();
            IRThread->IRCommandQueue.GetUnsafe().pop_front();
            if (NewCode.Repeat==0 && (NewCode.Code==KEY_PREVIOUS || NewCode.Code==KEY_REWIND))
            {
              DebugOut(strprintf("Ein Bild zurück...\r\n"));
              CurrentImageNumber--;
              CurrentImageNumber=max(0,CurrentImageNumber);
              DoNotTouchImageNumber=true;
            }
            else if (NewCode.Repeat==0 && (NewCode.Code==KEY_NEXT || NewCode.Code==KEY_FASTFORWARD))
            {
              DebugOut(strprintf("Ein Bild vor...\r\n"));
              CurrentImageNumber++;
              CurrentImageNumber=min(int(iFilesToDisplay.size())-1,CurrentImageNumber);
              DoNotTouchImageNumber=true;
            }
            else if (NewCode.Repeat==0 && (NewCode.Code==KEY_PLAY || NewCode.Code==KEY_STOP))
            {
              DoPause=!DoPause;
              if (DoPause)
                printf("Pause an...\r\n");
              else
                printf("Pause aus...\r\n");

              DoNotTouchImageNumber=true;
            }
            else
              DoNothing=true;
          }

        }
        if (!DoNotTouchImageNumber && !DoNothing)
          CurrentImageNumber++;


      }
      catch (exception& aErr)
      {
        // Im Fehlerfall: Ein Bild weiter!
        CurrentImageNumber++;
        ErrCount++;
        fprintf(stderr,"Error: %s\r\n",aErr.what());
      }
      if (ErrCount>=MaxErrCount)
      {
        fprintf(stderr,"Too many errors, leaving program...\r\n");
        break;  // zu viele Fehler...
      }

      if (Action==1)
        break;
    }
    if (ErrCount>=MaxErrCount)
      break;  // zu viele Fehler...
    if (Action==1)
      break;

    DebugOut(strprintf("Neuer Durchlauf...\r\n"));
    //iFilesToDisplay=iFilesAlreadyDisplayed;
    //iFilesAlreadyDisplayed.clear();

  }
  while (DoLoop && Action!=1 && iAllFilesToDisplay.size()>0);

  gParams.Cleanup();
  SDL_Quit();
  IRThread.reset();
  DebugOut(strprintf("Programm normal beendet!\r\n"));
  return 0;
}

int WaitAndCheckForQuit(Uint32 aWaitTime_ms)
{
  int ExitAction=0;
  Uint32 start=SDL_GetTicks();
  while(SDL_GetTicks()-start < aWaitTime_ms)
  {
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      switch (event.type)
      {
      case SDL_QUIT:
        /* Quit */
        ExitAction = 1;
        break;
      case SDL_KEYDOWN:
        ExitAction=event.key.keysym.sym==SDLK_q;
        break;
      }
    }
    if (ExitAction==0)
    {
      TCritGuard cg(IRThread->IRCommandQueue.GetCritSec());
      if (IRThread->IRCommandQueue.GetUnsafe().size()>0)
      {
        ExitAction=2;
      }
    }
    if (ExitAction==0)
      SDL_Delay(10);
    else
      break;  // IR-Kommando oder Abbruch sofort zurückmelden
  }
  return ExitAction;
}

// Load an image from "fname" and return an SDL_Texture with the content of the image
void LoadTextures(PiShowParams& aParams)
{
  if (aParams.CurrentTexture==NULL || aParams.CurrentTexture->ImageFilename.empty())
    return;
  long long start=GetTime_us();
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
  Uint32 rmask, gmask, bmask, amask;

  /* SDL interprets each pixel as a 32-bit number, so our masks must depend
     on the endianness (byte order) of the machine */
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
  rmask = 0xff000000;
  gmask = 0x00ff0000;
  bmask = 0x0000ff00;
  amask = 0x000000ff;
#else
  rmask = 0x000000ff;
  gmask = 0x0000ff00;
  bmask = 0x00ff0000;
  amask = 0xff000000;
#endif

  try
  {
    pImage=GetImage(aParams.CurrentTexture->ImageFilename,Width,Height,RawDataLength);
    // Ist das Bild größer als die maximale Texturgröße (2048x2048)?
    if (Width>2048 || Height>2048)
      throw runtime_error(strprintf("Image dimensions exceeding max. texture size (2048x2048). Width=%d Height=%d",Width,Height));
    image = SDL_CreateRGBSurfaceFrom(pImage, Width, Height, 32, 4*Width, bmask, gmask, rmask, amask);

    check_error_sdl(image == NULL, "SDL_CreateRGBSurfaceFrom failed");
    Uint32 format = SDL_GetWindowPixelFormat(aParams.Window);
    if (format==SDL_PIXELFORMAT_UNKNOWN)
      throw runtime_error( "Unable to get pixel format! SDL Error: " + string(SDL_GetError() ));
    DebugOut(strprintf("Window pixel format: %s\r\n",SDL_GetPixelFormatName(format)));
    //Convert surface to display format
//    WindowSurface=SDL_GetWindowSurface( gWindow );
    //  if( WindowSurface == NULL )
    //  throw runtime_error( "Unable to get window surface! SDL Error: " + string(SDL_GetError() ));
    formattedSurface = SDL_ConvertSurfaceFormat( image, format, 0 );

    if( formattedSurface == NULL )
      throw runtime_error( "Unable to convert loaded surface to display format! SDL Error: " + string(SDL_GetError() ));

    aParams.CurrentTexture->Texture = SDL_CreateTexture( aParams.Renderer, SDL_GetWindowPixelFormat( aParams.Window ), SDL_TEXTUREACCESS_STREAMING , formattedSurface->w, formattedSurface->h );
    if( aParams.CurrentTexture->Texture == NULL )
      throw runtime_error("Unable to create aParams.CurrentTexture texture! SDL Error: " +string(SDL_GetError() ));

    //Lock texture for manipulation
    check_error_sdl(SDL_LockTexture( aParams.CurrentTexture->Texture, NULL, &mPixels, &mPitch ),"SDL_LockTexture");
    //Copy loaded/formatted surface pixels
    memcpy( mPixels, formattedSurface->pixels, formattedSurface->pitch * formattedSurface->h );
    //Unlock texture to update
    SDL_UnlockTexture( aParams.CurrentTexture->Texture );
    //aParams.CurrentTexture = SDL_CreateTextureFromSurface(renderer, image);
    //check_error_sdl_img(aParams.CurrentTexture == NULL, "Unable to create a texture from the image");

    // Bildränder berechnen
    aParams.CurrentTexture->CalcBorders();
    if (aParams.CurrentTexture->ScreenRectStripe1.w>0 && aParams.CurrentTexture->ScreenRectStripe1.h>0)
    {
      // Texturen für die Ränder anlegen, die dann mit blur unscharf dargestellt werden
      Stripe1 = SDL_CreateRGBSurface(0, aParams.CurrentTexture->ScreenRectStripe1.w, aParams.CurrentTexture->ScreenRectStripe1.h, 32,  bmask, gmask, rmask, amask);
      check_error_sdl(Stripe1 == NULL, "SDL_CreateRGBSurface stripe1 failed");

      // Textur oben bzw. links
      SDL_Rect SrcRectStripe1;
      if (aParams.CurrentTexture->PortraitMode)
      {
        SrcRectStripe1.w=min(formattedSurface->w,int(formattedSurface->h*double(aParams.CurrentTexture->ScreenRectStripe1.w)/double(aParams.CurrentTexture->ScreenRectStripe1.h)));
        SrcRectStripe1.h=formattedSurface->h;
        SrcRectStripe1.x=0;
        SrcRectStripe1.y=0;
      }
      else
      {
        SrcRectStripe1.w=formattedSurface->w;
        SrcRectStripe1.h=min(formattedSurface->h,int(formattedSurface->w*double(aParams.CurrentTexture->ScreenRectStripe1.h)/double(aParams.CurrentTexture->ScreenRectStripe1.w)));
        SrcRectStripe1.x=0;
        SrcRectStripe1.y=0;
      }
      SDL_BlitScaled(formattedSurface, &SrcRectStripe1, Stripe1, NULL);
      BlurSurface(Stripe1, 10);
      FlipSurface(Stripe1,aParams.CurrentTexture->PortraitMode);
      aParams.CurrentTexture->Stripe1Texture = SDL_CreateTextureFromSurface(aParams.Renderer, Stripe1);
      check_error_sdl(aParams.CurrentTexture->Stripe1Texture == NULL, "SDL_CreateTextureFromSurface aParams.CurrentStripe1Texture failed.");
    }


    if (aParams.CurrentTexture->ScreenRectStripe2.w>0 && aParams.CurrentTexture->ScreenRectStripe2.h>0)
    {
      Stripe2 = SDL_CreateRGBSurface(0, aParams.CurrentTexture->ScreenRectStripe2.w, aParams.CurrentTexture->ScreenRectStripe2.h, 32,  bmask, gmask, rmask, amask);
      check_error_sdl(Stripe2 == NULL, "SDL_CreateRGBSurface stripe2 failed");

      // Textur unten bzw. rechts
      SDL_Rect SrcRectStripe2;
      if (aParams.CurrentTexture->PortraitMode)
      {
        SrcRectStripe2.w=min(formattedSurface->w,int(formattedSurface->h*double(aParams.CurrentTexture->ScreenRectStripe2.w)/double(aParams.CurrentTexture->ScreenRectStripe2.h)));
        SrcRectStripe2.h=formattedSurface->h;
        SrcRectStripe2.x=formattedSurface->w-SrcRectStripe2.w;
        SrcRectStripe2.y=0;
      }
      else
      {
        SrcRectStripe2.w=formattedSurface->w;
        SrcRectStripe2.h=min(formattedSurface->h,int(formattedSurface->w*double(aParams.CurrentTexture->ScreenRectStripe2.h)/double(aParams.CurrentTexture->ScreenRectStripe2.w)));
        SrcRectStripe2.x=0;
        SrcRectStripe2.y=formattedSurface->h-SrcRectStripe2.h;
      }
      SDL_BlitScaled(formattedSurface, &SrcRectStripe2, Stripe2, NULL);
      BlurSurface(Stripe2, 10);
      FlipSurface(Stripe2,aParams.CurrentTexture->PortraitMode);
      aParams.CurrentTexture->Stripe2Texture = SDL_CreateTextureFromSurface(aParams.Renderer, Stripe2);
      check_error_sdl(aParams.CurrentTexture->Stripe2Texture == NULL, "SDL_CreateTextureFromSurface aParams.CurrentStripe2Texture failed.");
    }

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
  SDL_FreeSurface(Stripe1);
  SDL_FreeSurface(Stripe2);
  delete[] pImage;
  long long ende=GetTime_us();
  DebugOut(strprintf("Ladezeit des Bildes :%.2f ms\r\n",double(ende-start)/1000.0));
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
  int status;
  struct stat st_buf;

  try
  {

    // Get the status of the file system object.
    status = stat (aFileName.c_str(), &st_buf);
    if (status != 0)
    {
      int error=errno;
      std::string ErrorMsg=GetOsErrString(error);
      throw std::runtime_error(strprintf( "Could not stat image file \"%s\": %s.",aFileName.c_str(),ErrorMsg.c_str()));
    }


    if (S_ISDIR (st_buf.st_mode))
      throw std::runtime_error(strprintf( "File \"%s\" is a directory!",aFileName.c_str()));

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
    DebugOut(strprintf ("Komprimierte Bildgröße: %d\r\n",_jpegSize));
    _compressedImage = new unsigned char[_jpegSize];
    assert(_compressedImage != NULL);
    s = fread(_compressedImage, 1, _jpegSize, fp);
    assert(s == _jpegSize);
    fclose(fp);
    fp=NULL;

    _jpegDecompressor = tjInitDecompress();
    tjDecompressHeader2(_jpegDecompressor, _compressedImage, _jpegSize, &width, &height, &jpegSubsamp);
    DebugOut(strprintf("Bildbreite: %d\r\n",width));
    DebugOut(strprintf("Bildhöhe: %d\r\n",height));
    DebugOut(strprintf ("Unkomprimierte Bildgröße: %d\r\n",width*height*COLOR_COMPONENTS));
    aRawDataLength=width*height*COLOR_COMPONENTS;
    buffer=new unsigned char[aRawDataLength];
    tjDecompress2(_jpegDecompressor, _compressedImage, _jpegSize, buffer, width, 0/*pitch*/, height, TJPF_BGRA, TJFLAG_FASTDCT);
    DebugOut(strprintf("Bild %s erfolgreich geladen.\r\n",aFileName.c_str()));
  }
  catch (std::exception & err)
  {
    if (fp!=NULL)
      fclose(fp);
    if (_jpegDecompressor!=NULL)
      tjDestroy(_jpegDecompressor);
    delete[] _compressedImage;
    throw;
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
      aParams.CurrentTexture->ScreenRect.w = aParams.CurrentTexture->TextureWidth*i/double(NumSteps);
      aParams.CurrentTexture->ScreenRect.h = aParams.CurrentTexture->TextureHeight*i/double(NumSteps);

      aParams.OldTexture->ScreenRect.w = aParams.OldTexture->TextureWidth*(1-i/double(NumSteps));/*+1920*10/double(NumSteps)*/;
      aParams.OldTexture->ScreenRect.h = aParams.OldTexture->TextureHeight*(1-i/double(NumSteps));/*+1200*10/double(NumSteps)*/;
      aParams.OldTexture->ScreenRect.x = aParams.OldTexture->TextureWidth*(i/double(NumSteps))-1;
      aParams.OldTexture->ScreenRect.y = aParams.OldTexture->TextureHeight*(i/double(NumSteps))-1;

      // Clear the window content (using the default renderer color)
      //SDL_RenderClear(aRenderer);

      // Copy the texture on the renderer
      if (aParams.OldTexture!=NULL && aParams.OldTexture->Texture!=NULL)
        SDL_RenderCopy(aParams.Renderer, aParams.OldTexture->Texture, NULL, &aParams.OldTexture->ScreenRect);
      // Copy the texture on the renderer
      SDL_RenderCopy(aParams.Renderer, aParams.CurrentTexture->Texture, NULL, &aParams.CurrentTexture->ScreenRect);

      // Update the window surface (show the renderer)
      SDL_RenderPresent(aParams.Renderer);

      SDL_Delay(10);
    }
    break;
  }
  case AlphaBlending:
  {
    check_error_sdl(SDL_SetTextureBlendMode(aParams.CurrentTexture->Texture, SDL_BLENDMODE_BLEND),"Setting alpha blend mode current texture");
    if (aParams.TextTexture!=NULL)
      check_error_sdl(SDL_SetTextureBlendMode(aParams.TextTexture, SDL_BLENDMODE_BLEND),"Setting alpha blend mode text texture");
    if (aParams.CurrentTexture->Stripe1Texture!=NULL)
      check_error_sdl(SDL_SetTextureBlendMode(aParams.CurrentTexture->Stripe1Texture, SDL_BLENDMODE_BLEND),"Setting alpha blend mode CurrentStripe1Texture");
    if(aParams.CurrentTexture->Stripe2Texture!=NULL)
      check_error_sdl(SDL_SetTextureBlendMode(aParams.CurrentTexture->Stripe2Texture, SDL_BLENDMODE_BLEND),"Setting alpha blend mode CurrentStripe2Texture");
    if (aParams.OldTexture!=NULL && aParams.OldTexture->Texture!=NULL)
    {
      check_error_sdl(SDL_SetTextureBlendMode(aParams.OldTexture->Texture, SDL_BLENDMODE_BLEND),"Setting alpha blend mode old texture");
      if(aParams.OldTexture->Stripe1Texture!=NULL)
        check_error_sdl(SDL_SetTextureBlendMode(aParams.OldTexture->Stripe1Texture, SDL_BLENDMODE_BLEND),"Setting alpha blend mode OldStripe1Texture");
      if(aParams.OldTexture->Stripe2Texture!=NULL)
        check_error_sdl(SDL_SetTextureBlendMode(aParams.OldTexture->Stripe2Texture, SDL_BLENDMODE_BLEND),"Setting alpha blend mode OldStripe2Texture");
    }
    const int NumSteps=255;
    for (int i=0; i<=NumSteps; i+=2)
    {
      SDL_SetTextureAlphaMod( aParams.CurrentTexture->Texture, i );
      if (aParams.CurrentTexture->Stripe1Texture!=NULL)
        SDL_SetTextureAlphaMod( aParams.CurrentTexture->Stripe1Texture, i );
      if (aParams.CurrentTexture->Stripe2Texture!=NULL)
        SDL_SetTextureAlphaMod( aParams.CurrentTexture->Stripe2Texture, i );

      if (aParams.OldTexture!=NULL && aParams.OldTexture->Texture!=NULL)
      {
        SDL_SetTextureAlphaMod( aParams.OldTexture->Texture, NumSteps-i );
        if(aParams.OldTexture->Stripe1Texture!=NULL)
          SDL_SetTextureAlphaMod( aParams.OldTexture->Stripe1Texture, NumSteps-i );
        if(aParams.OldTexture->Stripe2Texture!=NULL)
          SDL_SetTextureAlphaMod( aParams.OldTexture->Stripe2Texture, NumSteps-i );

        if(aParams.OldTexture->Stripe1Texture!=NULL)
          SDL_RenderCopy(aParams.Renderer, aParams.OldTexture->Stripe1Texture, NULL, &aParams.OldTexture->ScreenRectStripe1);
        if(aParams.OldTexture->Stripe2Texture!=NULL)
          SDL_RenderCopy(aParams.Renderer, aParams.OldTexture->Stripe2Texture, NULL, &aParams.OldTexture->ScreenRectStripe2);
        SDL_RenderCopy(aParams.Renderer, aParams.OldTexture->Texture, NULL, &aParams.OldTexture->ScreenRect);
      }
      // Copy the texture on the renderer
      if (aParams.CurrentTexture->Stripe1Texture!=NULL)
        SDL_RenderCopy(aParams.Renderer, aParams.CurrentTexture->Stripe1Texture, NULL, &aParams.CurrentTexture->ScreenRectStripe1);
      if (aParams.CurrentTexture->Stripe2Texture!=NULL)
        SDL_RenderCopy(aParams.Renderer, aParams.CurrentTexture->Stripe2Texture, NULL, &aParams.CurrentTexture->ScreenRectStripe2);
      SDL_RenderCopy(aParams.Renderer, aParams.CurrentTexture->Texture, NULL, &aParams.CurrentTexture->ScreenRect);

      //roundedBoxRGBA(aParams.Renderer,aParams.ScreenWidth/2-200,aParams.ScreenHeight/2-200,aParams.ScreenWidth/2+200,aParams.ScreenHeight/2+200,20,255,80,30,127);
      SDL_Rect TextRect;
      if (aParams.TextTexture!=NULL)
      {
        SDL_QueryTexture(aParams.TextTexture, NULL, NULL, &TextRect.w, &TextRect.h);
        TextRect.w+=i;
        TextRect.h+=i;
        TextRect.x=aParams.ScreenWidth/2-TextRect.w/2+i;
        TextRect.y=aParams.ScreenHeight/2-TextRect.h/2+i;
        SDL_SetTextureAlphaMod( aParams.TextTexture, i );
        SDL_RenderCopy(aParams.Renderer, aParams.TextTexture, NULL, &TextRect);
      }
      // Update the window surface (show the renderer)
      SDL_RenderPresent(aParams.Renderer);
      // Schleife verlassen, falls jemand auf die IR-Fernbedienung gedrück hat
      if (CheckForIRImageCommand())
        break;
      SDL_Delay(5);
    }
    break;
  }
  case AlphaMoving:
  {
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
  default:
    break;
  }
}

bool CheckForIRImageCommand()
{
  bool res=false;
  // Gibt es in der FB-Queue ein Kommando, welches Bildbezogen ist? (Vor, Zurück, Pause etc.)
  TCritGuard cg(IRThread->IRCommandQueue.GetCritSec());
  if (IRThread->IRCommandQueue.GetUnsafe().size()>0)
  {
    deque<IRCode>::iterator it;
    for (it=IRThread->IRCommandQueue.GetUnsafe().begin(); it!=IRThread->IRCommandQueue.GetUnsafe().end(); ++it)
    {
      if (it->Code==KEY_PREVIOUS || it->Code==KEY_NEXT || it->Code==KEY_PLAY || it->Code==KEY_STOP || it->Code==KEY_FASTFORWARD || it->Code==KEY_REWIND)
      {
        res=true;
        break;
      }
    }
  }
  return res;
}

vector<string> ExpandFileNames(const vector<string> & aDirsOrFileNames)
{
  vector<string> iFileNames;
  struct stat st_buf;
  int status;
  for (unsigned int i=0; i<aDirsOrFileNames.size(); i++)
  {
    // Feststellen: Datei oder Verzeichnis?
    try
    {
      // Get the status of the file system object.
      status = stat (aDirsOrFileNames[i].c_str(), &st_buf);
      if (status != 0)
      {
        int error=errno;
        std::string ErrorMsg=GetOsErrString(error);
        throw std::runtime_error(strprintf( "Could not stat image file \"%s\": %s.",aDirsOrFileNames[i].c_str(),ErrorMsg.c_str()));
      }

      if (S_ISREG (st_buf.st_mode))
      {
        // normale Datei, mit in die Liste
        iFileNames.push_back(aDirsOrFileNames[i]);
      }
      else if (S_ISDIR (st_buf.st_mode))
      {
        // Verzeichnis
        vector<string>iFiles=FindFilesInDir(aDirsOrFileNames[i]);
        for (unsigned int t=0; t<iFiles.size(); t++)
          iFileNames.push_back(iFiles[t]);
      }
    }
    catch (exception & err)
    {
      fprintf(stderr, "Kann Datei oder Verzeichnis nicht mit in die Liste aufnehmen: %s : %s\r\n",aDirsOrFileNames[i].c_str(),err.what());
    }
  }
  return iFileNames;
}

vector<string> FindFilesInDir(const string & aDir)
{
  vector<string> iFiles;
  DIR *d;
  struct dirent *dir;
  d = opendir(aDir.c_str());
  if (d)
  {
    while ((dir = readdir(d)) != NULL)
    {
      if (dir->d_type == DT_REG)
        iFiles.push_back(string(aDir)+"/"+string(dir->d_name));
    }
    closedir(d);
  }
  return iFiles;
}

void CreateTextTexture(PiShowParams& aParams)
{
  try
  {
    TTF_Font *font=NULL;
    SDL_Surface *surface=NULL;

    if (aParams.TextTexture!=NULL)
    {
      SDL_DestroyTexture(aParams.TextTexture);
      aParams.TextTexture=NULL;
    }

    // TTF initialisieren
    if(TTF_Init()==-1)
      throw runtime_error( "TTF_Init failed! TTF Error: " + string(TTF_GetError() ));


    font=TTF_OpenFont("/usr/share/fonts/truetype/freefont/FreeSansBold.ttf", 64);
    if(!font)
      throw runtime_error( "TTF_OpenFont failed! TTF Error: " + string(TTF_GetError() ));

    SDL_Color color= {0,0,0};
    if(!(surface=TTF_RenderText_Blended(font,"Oliver Rutsch!",color)))
      throw runtime_error( "TTF_RenderText_Blended failed! TTF Error: " + string(TTF_GetError() ));

    aParams.TextTexture = SDL_CreateTextureFromSurface(aParams.Renderer, surface);
    check_error_sdl(aParams.TextTexture == NULL, "SDL_CreateTextureFromSurface texture failed.");

    finally cleanup([&]
    {
      if (font!=NULL)
        TTF_CloseFont(font);
      if (surface!=NULL)
        SDL_FreeSurface(surface);

      TTF_Quit();
    });

  }
  catch (...)
  {
    throw;
  }
  return;
}

