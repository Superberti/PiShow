#!/usr/bin/python3
# -*- coding: utf-8 -*-

# Hier kommen die unbearbeiteten Bilder an
rootdir="/samba/public/"
convdir="/home/pi/Pictures/Anzeige/conv/"
tempdir="/tmp/"

DisplayWidth=1920
DisplayHeight=1080

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

if os.path.isdir(rootdir)==False:
    print("Das Verzeichnis <"+rootdir+"> existiert nicht. Beende Konverter.", file=sys.stdout, flush=True)
    exit(1)
else:
    print("Verzeichnis <"+rootdir+"> gefunden...", file=sys.stdout, flush=True)
    
if os.access(rootdir, os.R_OK)==False or os.access(rootdir, os.W_OK)==False:
    print("Fehlende Lese/Schreibberechtigung im Verzeichnis <"+rootdir+">. Beende Konverter.", file=sys.stdout, flush=True)
    exit(1)
else:
    print("Schreib/Leseberechtigung OK: <"+rootdir+">", file=sys.stdout, flush=True)

if os.path.isdir(convdir)==False:
    print("Das Verzeichnis <"+convdir+"> existiert nicht. Beende Konverter.", file=sys.stdout, flush=True)
    exit(1)
else:
    print("Verzeichnis <"+convdir+"> gefunden...", file=sys.stdout, flush=True)
    
if os.access(convdir, os.R_OK)==False or os.access(convdir, os.W_OK)==False:
    print("Fehlende Lese/Schreibberechtigung im Verzeichnis <"+convdir+">. Beende Konverter.", file=sys.stdout, flush=True)
    exit(1)
else:
    print("Schreib/Leseberechtigung OK: <"+convdir+">", file=sys.stdout, flush=True)
    
if os.path.isdir(tempdir)==False:
    print("Das Verzeichnis <"+tempdir+"> existiert nicht. Beende Konverter.", file=sys.stdout, flush=True)
    exit(1)
else:
    print("Verzeichnis <"+tempdir+"> gefunden...", file=sys.stdout, flush=True)
    
if os.access(tempdir, os.R_OK)==False or os.access(tempdir, os.W_OK)==False:
    print("Fehlende Lese/Schreibberechtigung im Verzeichnis <"+tempdir+">. Beende Konverter.", file=sys.stdout, flush=True)
    exit(1)
else:
    print("Schreib/Leseberechtigung OK: <"+tempdir+">", file=sys.stdout, flush=True)
    
print ("Alle Checks OK, Raspi-Bilderrahmen-Konverter wird gestartet...")

while 1==1:
    ImageList = glob.glob(rootdir+"*.JPG")
    ImageList2 =glob.glob(rootdir+"*.jpg")
    ImageList.extend(ImageList2)
    ImageList2 =glob.glob(rootdir+"*.jpeg")
    ImageList.extend(ImageList2)
    ImageList2 =glob.glob(rootdir+"*.JPEG")
    ImageList.extend(ImageList2)
    
    # Liste überprüfen, ob Bilder mit *_error.JPG vorhanden sind. Die sollten nicht noch einmal konvertiert werden...
    for ImageFileName in ImageList:
        if "_error.JPG" in ImageFileName:
            print("Überspringe fehlerhaftes Bild:"+ImageFileName, file=sys.stdout, flush=True)
            ImageList.remove(ImageFileName)

    if len(ImageList)>0:
        print("Neue Bilder erkannt. Starte Konversion...", file=sys.stdout, flush=True)
    else:
#        print("Nächste Überprüfung in 10 Sekunden.", file=sys.stdout, flush=True)
        time.sleep(10)

    # Bilder konvertieren
    for ImageFileName in ImageList:
        # Falls sich Bilder durch Samba noch in der Übertragung befinden, dann wird dieses Bild erst einmal übersprungen.
        # Ist es das einzige Bild in der Liste, dann wird zusätzlich noch Sekunden gewartet
        try:
                lsofOut=subprocess.check_output('lsof -t "'+ImageFileName+'"', shell=True, stderr=subprocess.STDOUT)
                if lsofOut!="":
                        lsofArgs=lsofOut.decode("utf-8")
                        if RepresentsInt(lsofArgs)==True:
                                print ("Bild: "+ImageFileName+" ist gerade im Zugriff (PID="+lsofArgs+"). Überspringe das Bild erst einmal...", file=sys.stdout, flush=True)
                                time.sleep(1)
                                if len(ImageList)==1:
                                        time.sleep(10)
                                continue
                        else:
                                print ("Komischer lsof-output:"+lsofOut, file=sys.stdout, flush=True)

        except subprocess.CalledProcessError as lsofexc:
                # Fehlercode 1 ist normal, falls nichts gefunden werden konnte
                if lsofexc.returncode!=1:
                        print ("lsof Fehlercode", lsofexc.returncode, lsofexc.output, file=sys.stdout, flush=True )
                        print ("Überspringe Bild: "+ImageFileName, file=sys.stdout, flush=True)
                        continue

        BaseFileName=os.path.splitext(os.path.basename(ImageFileName))[0]

        NewFileName=os.path.join(convdir,BaseFileName+"_conv.JPG")

        print ("Konvertiere<"+ImageFileName+"> nach <"+NewFileName+">...", end="", file=sys.stdout, flush=True)

        ExifArgsString=subprocess.check_output('exiftool -s -s -s -n -f -ImageWidth -ImageHeight -Orientation -ImageDescription "'+ImageFileName+'"', shell=True, stderr=subprocess.STDOUT)
        #print (ExifArgs)

        ExifArgs=ExifArgsString.decode("utf-8").splitlines()
        #print(ExifArgs)
        Orientation = -1
        Width=int(ExifArgs[0])
        Height=int(ExifArgs[1])
        if RepresentsInt(ExifArgs[2])==True:
            Orientation=int(ExifArgs[2])
        Description=ExifArgs[3]



        if Height>Width or (Orientation==6 or Orientation==5):
            print("Hochformat...",end="", file=sys.stdout, flush=True)
            ResizeVal="-resize x"+str(DisplayHeight)
        else:
            if DisplayWidth/Width*Height>DisplayHeight:
                print("Querformat in y begrenzt...",end="", file=sys.stdout, flush=True)
                ResizeVal="-resize x"+str(DisplayHeight)
            else:
                print("Querformat in x begrenzt...",end="", file=sys.stdout, flush=True)
                ResizeVal="-resize "+str(DisplayWidth)+"x"


        try:
            OrigNewFileName=NewFileName
            RenameFile=False
            if os.path.isfile(NewFileName)==True:
                print("Dublette erkannt", file=sys.stdout, flush=True)
                NewFileName=os.path.join(tempdir,BaseFileName+"_conv.JPG")
                RenameFile=True

            grepOut=subprocess.check_output('convert "'+ImageFileName+'" -auto-orient -ordered-dither o8x8,64,64,64 -quality 97 '+ResizeVal+' "'+NewFileName+'"', shell=True)

            # Überprüfen, ob jetzt keine Dimension 2048 (max. Texturgröße) überschreitet:
            ExifArgsString=subprocess.check_output('exiftool -s -s -s -n -f -ImageWidth -ImageHeight -Orientation -ImageDescription "'+NewFileName+'"', shell=True, stderr=subprocess.STDOUT)
            ExifArgs=ExifArgsString.decode("utf-8").splitlines()
            Width=int(ExifArgs[0])
            Height=int(ExifArgs[1])
            
            # Zu kleine Bilder wollen wird aber auch nicht (z.B. Thumnails). Mindestens 512 Pixel
            if Width<512 or Height<512:
                print("Bild wird nicht konvertiert, da mindestens eine Seite kleiner als 512 pixel ist.", file=sys.stdout, flush=True)
                os.remove(NewFileName)
            elif Width>2048 or Height>2048:# Überprüfen, ob jetzt keine Dimension 2048 (max. Texturgröße) überschreitet:
                print("Bild wird nicht konvertiert, da eine Seite immer noch größer als 2048 pixel ist.", file=sys.stdout, flush=True)
                os.remove(NewFileName)
                NewFileName=os.path.join(rootdir,BaseFileName+"_error.JPG")
                os.rename(ImageFileName, NewFileName);
            else:
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
