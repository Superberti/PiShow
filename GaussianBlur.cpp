#include "GaussianBlur.h"
#include <SDL2/SDL.h>
#include "SdlTools.h"

using namespace std;

/// wendet den Blur-Effekt auf eine Textur an
void BlurTexture(SDL_Texture* pTexture, unsigned int r)
{
  int CurrentTextureWidth=0;
  int CurrentTextureHeight=0;
  unsigned char * aPixelsCurrent=NULL;
  int mPitchCurrent=0;
  Uint32 mFormat=SDL_PIXELFORMAT_UNKNOWN;
  check_error_sdl(SDL_QueryTexture(pTexture, &mFormat, NULL, &CurrentTextureWidth, &CurrentTextureHeight),"SDL_QueryTexture");
  check_error_sdl(SDL_LockTexture(pTexture,NULL, (void**)&aPixelsCurrent, &mPitchCurrent),"SDL_LockTexture");
  const int NumBytesCurrent=mPitchCurrent*CurrentTextureHeight;
  // Aufteilung in einzelne Farbkanäle
  const int ChannelSize=NumBytesCurrent/4;
  unsigned char * rc=new unsigned char[ChannelSize];
  unsigned char * gc=new unsigned char[ChannelSize];
  unsigned char * bc=new unsigned char[ChannelSize];

  unsigned char * rc_d=new unsigned char[ChannelSize];
  unsigned char * gc_d=new unsigned char[ChannelSize];
  unsigned char * bc_d=new unsigned char[ChannelSize];
  int cc=0;
  for (int i=0; i<NumBytesCurrent; i+=4)
  {
    bc[cc]=aPixelsCurrent[i];
    gc[cc]=aPixelsCurrent[i+1];
    rc[cc]=aPixelsCurrent[i+2];

    bc_d[cc]=bc[cc];
    gc_d[cc]=gc[cc];
    rc_d[cc]=rc[cc];
    cc++;
  }
  gaussBlur(bc,bc_d,ChannelSize,CurrentTextureWidth,CurrentTextureHeight,r);
  gaussBlur(gc,gc_d,ChannelSize,CurrentTextureWidth,CurrentTextureHeight,r);
  gaussBlur(rc,rc_d,ChannelSize,CurrentTextureWidth,CurrentTextureHeight,r);
  cc=0;
  for (int i=0; i<NumBytesCurrent; i+=4)
  {
    aPixelsCurrent[i]=bc_d[cc];
    aPixelsCurrent[i+1]=gc_d[cc];
    aPixelsCurrent[i+2]=rc_d[cc];
    cc++;
  }
  SDL_UnlockTexture(pTexture);
  delete[] rc;
  delete[] gc;
  delete[] bc;
  delete[] rc_d;
  delete[] gc_d;
  delete[] bc_d;
}

/// Einen Farbkanal blurren
void gaussBlur (unsigned char *scl, unsigned char* tcl, unsigned int scl_length, unsigned int w, unsigned int h, unsigned int r)
{
  vector<unsigned int> bxs = boxesForGauss(r, 3);
  boxBlur (scl, tcl, scl_length, w, h, (bxs[0]-1)/2);
  boxBlur (tcl, scl, scl_length, w, h, (bxs[1]-1)/2);
  boxBlur (scl, tcl, scl_length, w, h, (bxs[2]-1)/2);
}

vector<unsigned int> boxesForGauss(unsigned int sigma, unsigned int n)  // standard deviation, number of boxes
{
  double wIdeal = sqrt((12*sigma*sigma/n)+1);  // Ideal averaging filter width
  unsigned int wl = floor(wIdeal);
  if(wl%2==0)
    wl--;
  int wu = wl+2;

  double mIdeal = (12*sigma*sigma - n*wl*wl - 4*n*wl - 3*n)/(-4*wl - 4);
  unsigned int m = (unsigned int) (mIdeal+0.5);
  vector<unsigned int> sizes;
  for(unsigned int i=0; i<n; i++)
    sizes.push_back(i<m?wl:wu);
  return sizes;
}

void boxBlur (unsigned char *scl, unsigned char *tcl, unsigned int scl_length, unsigned int w, unsigned int h, unsigned int r)
{
  memcpy(tcl,scl,scl_length);
  boxBlurH(tcl, scl, w, h, r);
  boxBlurT(scl, tcl, w, h, r);
}

void boxBlurH (unsigned char *scl, unsigned char *tcl, unsigned int w, unsigned int h, unsigned int r)
{
  double iarr = 1 / double(r+r+1);
  for(unsigned int i=0; i<h; i++)
  {
    unsigned int ti = i*w, li = ti, ri = ti+r;
    unsigned int fv = scl[ti], lv = scl[ti+w-1], val = (r+1)*fv;
    for(unsigned int j=0; j<r; j++)
      val += scl[ti+j];
    for(unsigned int j=0; j<=r; j++)
    {
      val += scl[ri++] - fv;
      tcl[ti++] = (unsigned char)(val*iarr+0.5);
    }
    for(unsigned int j=r+1; j<w-r; j++)
    {
      val += scl[ri++] - scl[li++];
      tcl[ti++] = (unsigned char)(val*iarr+0.5);
    }
    for(unsigned int j=w-r; j<w; j++)
    {
      val += lv - scl[li++];
      tcl[ti++] = (unsigned char)(val*iarr+0.5);
    }
  }
}

void boxBlurT (unsigned char *scl, unsigned char *tcl, unsigned int w, unsigned int h, unsigned int r)
{
  double iarr = 1 / double(r+r+1);
  for(unsigned int i=0; i<w; i++)
  {
    unsigned int ti = i, li = ti, ri = ti+r*w;
    unsigned int fv = scl[ti], lv = scl[ti+w*(h-1)], val = (r+1)*fv;
    for(unsigned int j=0; j<r; j++)
      val += scl[ti+j*w];
    for(unsigned int j=0; j<=r; j++)
    {
      val += scl[ri] - fv;
      tcl[ti] = (unsigned char)(val*iarr+0.5);
      ri+=w;
      ti+=w;
    }
    for(unsigned int j=r+1; j<h-r; j++)
    {
      val += scl[ri] - scl[li];
      tcl[ti] = (unsigned char)(val*iarr+0.5);
      li+=w;
      ri+=w;
      ti+=w;
    }
    for(unsigned int j=h-r; j<h; j++)
    {
      val += lv - scl[li];
      tcl[ti] = (unsigned char)(val*iarr+0.5);
      li+=w;
      ti+=w;
    }
  }
}
