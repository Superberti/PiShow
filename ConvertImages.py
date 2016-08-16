#!/usr/bin/python3
# -*- coding: utf-8 -*-

# Hier kommen die unbearbeiteten Bilder an
rootdir="/samba/public/"
convdir="/home/pi/Pictures/Anzeige/conv/"
tempdir="/tmp/"

import subprocess
import os
import glob
import time
import filecmp
import shutil
import sys

def RepresentsInt(s):
        try: 
                int(s)
                return True
        except ValueError:
                return False

print ("Raspi-Bilderrahmen-Konverter gestartet...")
DisplayWidth=1920
DisplayHeight=1080

while 1==1:
    ImageList = glob.glob(rootdir+"*.JPG")
    ImageList2=glob.glob(rootdir+"*.jpg")
    ImageList.extend(ImageList2)

    if len(ImageList)>0:
        print("Neue Bilder erkannt. Starte Konversion...", file=sys.stdout, flush=True)
    else:
        #print("Nächste Überprüfung in 60 Sekunden.", file=sys.stdout, flush=True)
        time.sleep(60)

    # Bilder konvertieren
    for ImageFileName in ImageList:
        BaseFileName=os.path.splitext(os.path.basename(ImageFileName))[0]
      
        NewFileName=os.path.join(convdir,BaseFileName+"_conv.JPG")
        
        print ("Konvertiere<"+ImageFileName+"> nach <"+NewFileName+">...", end="", file=sys.stdout, flush=True)       

        ExifArgsString=subprocess.check_output('exiftool -s -s -s -n -f -ImageWidth -ImageHeight -Orientation -ImageDescription "'+ImageFileName+'"', shell=True, stderr=subprocess.STDOUT)
        #print (ExifArgs)

        ExifArgs=ExifArgsString.decode("utf-8").splitlines()
        #print(ExifArgs)

        Width=int(ExifArgs[0])
        Height=int(ExifArgs[1])
        
        Description=ExifArgs[3]

        

        if Height>Width:
            print("hochformat...",end="", file=sys.stdout, flush=True)
            ResizeVal="-resize x"+str(DisplayHeight)
        else:
            if DisplayWidth/Width*Height>DisplayHeight:
                print("querformat in y begrenzt...",end="", file=sys.stdout, flush=True)
                ResizeVal="-resize x"+str(DisplayHeight)
            else:
                print("querformat in x begrenzt...",end="", file=sys.stdout, flush=True)
                ResizeVal="-resize "+str(DisplayWidth)+"x"
        
            
        try:
            OrigNewFileName=NewFileName
            RenameFile=False
            if os.path.isfile(NewFileName)==True:
                print("dublette erkannt", file=sys.stdout, flush=True)
                NewFileName=os.path.join(tempdir,BaseFileName+"_conv.JPG")
                RenameFile=True
                
            grepOut=subprocess.check_output('convert "'+ImageFileName+'" -auto-orient -ordered-dither o8x8,64,64,64 -quality 97 '+ResizeVal+' "'+NewFileName+'"', shell=True)
            
            if RenameFile==True:
                if filecmp.cmp(NewFileName,OrigNewFileName)==True:
                    print ("Das Bild <"+OrigNewFileName+"> exisitert bereits. Keine Aktion notwendig!", file=sys.stdout, flush=True)
                else:
                    print ("Der Bildname <"+OrigNewFileName+"> exisitert zwar bereits, aber der Bildinhalt ist unterschiedlich. Benenne Zieldatei um!", file=sys.stdout, flush=True)
                    ImageCounter=1
                    NewFileName=os.path.join(convdir,BaseFileName+"_"+str(ImageCounter)+"_conv.JPG")
                    while os.path.isfile(NewFileName)==True:
                        ImageCounter+=1
                        NewFileName=os.path.join(convdir,BaseFileName+"_"+str(ImageCounter)+"_conv.JPG")

                    TempFileName=os.path.join(tempdir,BaseFileName+"_conv.JPG")
                    shutil.copy2(TempFileName, NewFileName)
                    os.remove(TempFileName)
                    
            os.remove(ImageFileName)
        except subprocess.CalledProcessError as grepexc:
            print ("convert Fehlercode", grepexc.returncode, grepexc.output, file=sys.stdout, flush=True )

        print("fertig.", file=sys.stdout, flush=True)
        sys.stdout.flush()


    
    
