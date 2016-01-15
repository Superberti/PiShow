#ifndef GAUSSIANBLUR_H_INCLUDED
#define GAUSSIANBLUR_H_INCLUDED

#include <math.h>
#include <stdio.h>
#include <vector>
#include <string>

/// Funktion für den Gaussian-Blur-Effekt auf einem Graukanal
/// Ein Graukanal wird als zusammenhängendes Array ohne Padding definiert
/// \param scl Source-Array mit Grauwerten
/// \param tcl Destination-Array (gleiche Größe wie scl) mit geblurten Graudaten
/// \param scl_length Länge von scl und tcl
/// \param w Bildbreite
/// \param h Bildhöhe
/// \param r Blur-Radius
void gaussBlur_4 (unsigned char *scl, unsigned char* tcl,int scl_length, int w,int h,int r);

std::vector<int> boxesForGauss(int sigma, int n);
void boxBlur_4 (unsigned char *scl, unsigned char *tcl,int scl_length,int w,int h,int r);
void boxBlurH_4 (unsigned char *scl, unsigned char *tcl, int scl_length,int w,int h,int r);
void boxBlurT_4 (unsigned char *scl, unsigned char *tcl, int scl_length, int w,int h,int r);

#endif // GAUSSIANBLUR_H_INCLUDED
