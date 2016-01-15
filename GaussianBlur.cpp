#include "GaussianBlur.h"

using namespace std;

void gaussBlur_4 (unsigned char *scl, unsigned char* tcl,int scl_length, int w,int h,int r)
{
  vector<int> bxs = boxesForGauss(r, 3);
  boxBlur_4 (scl, tcl, scl_length, w, h, (bxs[0]-1)/2);
  boxBlur_4 (tcl, scl, scl_length, w, h, (bxs[1]-1)/2);
  boxBlur_4 (scl, tcl, scl_length, w, h, (bxs[2]-1)/2);
}

vector<int> boxesForGauss(int sigma, int n)  // standard deviation, number of boxes
{
  double wIdeal = sqrt((12*sigma*sigma/n)+1);  // Ideal averaging filter width
  int wl = floor(wIdeal);
  if(wl%2==0)
    wl--;
  int wu = wl+2;

  double mIdeal = (12*sigma*sigma - n*wl*wl - 4*n*wl - 3*n)/(-4*wl - 4);
  int m = int (mIdeal+0.5);
  vector<int> sizes;
  for(int i=0; i<n; i++)
    sizes.push_back(i<m?wl:wu);
  return sizes;
}

void boxBlur_4 (unsigned char *scl, unsigned char *tcl,int scl_length,int w,int h,int r)
{
  for(int i=0; i<scl_length; i++)
    tcl[i] = scl[i];
  boxBlurH_4(tcl, scl, scl_length, w, h, r);
  boxBlurT_4(scl, tcl, scl_length, w, h, r);
}

void boxBlurH_4 (unsigned char *scl, unsigned char *tcl, int scl_length,int w,int h,int r)
{
  double iarr = 1 / (r+r+1);
  for(int i=0; i<h; i++)
  {
    int ti = i*w, li = ti, ri = ti+r;
    int fv = scl[ti], lv = scl[ti+w-1], val = (r+1)*fv;
    for(int j=0; j<r; j++)
      val += scl[ti+j];
    for(int j=0; j<=r; j++)
    {
      val += scl[ri++] - fv;      ;
      tcl[ti++] = (unsigned char)(val*iarr+0.5);
    }
    for(int j=r+1; j<w-r; j++)
    {
      val += scl[ri++] - scl[li++];
      tcl[ti++] = (unsigned char)(val*iarr+0.5);
    }
    for(int j=w-r; j<w; j++)
    {
      val += lv - scl[li++];
      tcl[ti++] = (unsigned char)(val*iarr+0.5);
    }
  }
}

void boxBlurT_4 (unsigned char *scl, unsigned char *tcl, int scl_length, int w,int h,int r)
{
  double iarr = 1 / (r+r+1);
  for(int i=0; i<w; i++)
  {
    int ti = i, li = ti, ri = ti+r*w;
    int fv = scl[ti], lv = scl[ti+w*(h-1)], val = (r+1)*fv;
    for(int j=0; j<r; j++)
      val += scl[ti+j*w];
    for(int j=0; j<=r; j++)
    {
      val += scl[ri] - fv;
      tcl[ti] = (unsigned char)(val*iarr+0.5);
      ri+=w;
      ti+=w;
    }
    for(int j=r+1; j<h-r; j++)
    {
      val += scl[ri] - scl[li];
      tcl[ti] = (unsigned char)(val*iarr+0.5);
      li+=w;
      ri+=w;
      ti+=w;
    }
    for(int j=h-r; j<h; j++)
    {
      val += lv - scl[li];
      tcl[ti] = (unsigned char)(val*iarr+0.5);
      li+=w;
      ti+=w;
    }
  }
}
