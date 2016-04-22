#!/usr/bin/python3
# -*- coding: utf-8 -*-

# Hier kommen die unbearbeiteten Bilder an
rootdir="/samba/public/"
convdir="/home/pi/Pictures/conv/"
tempdir="/tmp/"

import subprocess
import os
import glob
import time
import filecmp
import shutil

print ("Raspi-Bilderrahmen-Konverter gestartet...")

while 1==1:
    ImageList = glob.glob(rootdir+"*.JPG")
    ImageList2=glob.glob(rootdir+"*.jpg")
    ImageList.extend(ImageList2)

    if len(ImageList)>0:
        print("Neue Bilder erkannt. Starte Konversion...")
    else:
        print("Nächste Überprüfung in 5 Minuten.")
        time.sleep(30)

    # Bilder konvertieren
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
            OrigNewFileName=NewFileName
            RenameFile=False
            if os.path.isfile(NewFileName)==True:
                print("dublette erkannt")
                NewFileName=os.path.join(tempdir,BaseFileName+"_conv.JPG")
                RenameFile=True
                
            grepOut=subprocess.check_output("convert "+ImageFileName+" -auto-orient -ordered-dither o8x8,64,64,64 -quality 97 "+ResizeVal+" "+NewFileName, shell=True)
            
            if RenameFile==True:
                if filecmp.cmp(NewFileName,OrigNewFileName)==True:
                    print ("Das Bild <"+OrigNewFileName+"> exisitert bereits. Keine Aktion notwendig!")
                else:
                    print ("Der Bildname <"+OrigNewFileName+"> exisitert zwar bereits, aber der Bildinhalt ist unterschiedlich. Benenne Zieldatei um!")
                    ImageCounter=1
                    NewFileName=os.path.join(convdir,BaseFileName+"_"+str(ImageCounter)+"_conv.JPG")
                    while os.path.isfile(NewFileName)==True:
                        ImageCounter+=1
                        NewFileName=os.path.join(convdir,BaseFileName+"_"+str(ImageCounter)+"_conv.JPG")

                    TempFileName=os.path.join(tempdir,BaseFileName+"_conv.JPG")
                    shutil.copy2(TempFileName, NewFileName)
                    os.remove(TempFileName)
                    
            os.remove(ImageFileName)
        except subprocess.CalledProcessError as repexc:
            print ("convert Fehlercode", grepexc.returncode, grepexc.output )

        print("fertig.")

    
    
