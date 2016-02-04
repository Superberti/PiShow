#ifndef GAUSSIANBLUR_H_INCLUDED
#define GAUSSIANBLUR_H_INCLUDED

#include <math.h>
#include <stdio.h>
#include <vector>
#include <string>

struct SDL_Texture;
struct SDL_Surface;

void BlurTexture(SDL_Texture* pTexture, unsigned int r);

void BlurSurface(SDL_Surface* pSurface, unsigned int r);

void FlipSurface(SDL_Surface* pSurface, bool aFlipHorizontal);

void BlurRGBA(int aWidth, int aHeight, int aPitch, unsigned char * pPixels, unsigned int aPixelFormat, int r);

/// Funktion für den Gaussian-Blur-Effekt auf einem Graukanal
/// Ein Graukanal wird als zusammenhängendes Array ohne Padding definiert
/// \param scl Source-Array mit Grauwerten
/// \param tcl Destination-Array (gleiche Größe wie scl) mit geblurten Graudaten
/// \param scl_length Länge von scl und tcl
/// \param w Bildbreite
/// \param h Bildhöhe
/// \param r Blur-Radius
void gaussBlur (unsigned char *scl, unsigned char* tcl, unsigned int scl_length, unsigned int w, unsigned int h, unsigned int r);

std::vector<unsigned int> boxesForGauss(unsigned int sigma, unsigned int n);
void boxBlur (unsigned char *scl, unsigned char *tcl, unsigned int scl_length, unsigned int w, unsigned int h, unsigned int r);
void boxBlurH (unsigned char *scl, unsigned char *tcl, unsigned int w, unsigned int h, unsigned int r);
void boxBlurT (unsigned char *scl, unsigned char *tcl, unsigned int w, unsigned int h, unsigned int r);

#endif // GAUSSIANBLUR_H_INCLUDED
