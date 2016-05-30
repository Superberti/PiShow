# -*- coding: utf-8 -*-

# Build-File für PiShow
import os
import struct
OS_BITSIZE = struct.calcsize("P") * 8
print ("OS bitsize: "+str(OS_BITSIZE))
env = Environment(ENV = {'PATH' : os.environ['PATH']})
if OS_BITSIZE==64:
    static_jpeg_lib = File('/opt/libjpeg-turbo/lib64/libturbojpeg.a')
else:
    static_jpeg_lib = File('/opt/libjpeg-turbo/lib32/libturbojpeg.a')
env.ParseConfig('sdl2-config --cflags --libs')
env.Append(CCFLAGS = '-Wall -std=c++11 -O2')
env['LIBS']=['m','SDL2_image','SDL2_ttf','SDL2',static_jpeg_lib,'SDL2_gfx']
#env['LIBPATH']=['/opt/libjpeg-turbo/lib32/']
env['CPPPATH']=['/opt/libjpeg-turbo/include/','/opt/vc/include']

env.Program('PiShow',['GaussianBlur.cpp','SdlTools.cpp','main.cpp','tools.cpp'])
