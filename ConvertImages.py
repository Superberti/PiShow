#!/usr/bin/python
# -*- coding: utf-8 -*-

# Hier kommen die unbearbeiteten Bilder an
rootdir="/samba/public/"
convdir="~/Pictures/conv/"
#print ("Hallo, Welt!üü")
import subprocess
import os

#MyOutput = subprocess.check_output('ls -l '+rootdir, shell=True, stderr=subprocess.STDOUT)
#for line in p.stdout.readlines():
#    print (line),
#retval = p.wait()
#print(MyOutput)

import glob
ImageList = glob.glob(rootdir+"*.JPG")
ImageList2=glob.glob(rootdir+"*.jpg")
ImageList.extend(ImageList2)

#print (ImageList)

for ImageFileName in ImageList:
    #print("Bearbeite Bild: "+ImageFileName)

    ExifArgsString=subprocess.check_output('exiftool -s -s -s -n -ImageWidth -ImageHeight -Orientation -ImageDescription '+ImageFileName, shell=True, stderr=subprocess.STDOUT)
    #print (ExifArgs)

    ExifArgs=ExifArgsString.decode("utf-8").splitlines()
    #print(ExifArgs)

    Width=int(ExifArgs[0])
    Height=int(ExifArgs[1])
    Orientation=int(ExifArgs[2])
    Description=ExifArgs[3]

    BaseFileName=os.path.splitext(os.path.basename(ImageFileName))[0]
    #print (BaseFileName)
    #print("w:"+repr(Width)+" h:"+repr(Height)+" o:"+repr(Orientation))
    
    NewFileName=os.path.join(convdir,BaseFileName+"_conv.JPG")
    print ("Konvertiere<"+ImageFileName+"> nach <"+NewFileName+">...", end="")

    if Orientation==1 and Width>=Height:
        print("querformat...",end="")
        ResizeVal="-resize 1920x"
    else:
        print("hochformat...",end="")
        ResizeVal="-resize x1200"
        
    try:
        grepOut=subprocess.check_output("convert "+ImageFileName+" -auto-orient -ordered-dither o8x8,64,64,64 -quality 97 "+ResizeVal+" "+NewFileName, shell=True)
    except subprocess.CalledProcessError as repexc:
        print ("convert failed with code", grepexc.returncode, grepexc.output )

    print("fertig.")

    
