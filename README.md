# PiShow
Dieses Programm wurde zur Anzeige von Bildern auf einem selbstgebauten TFT-Bilderrahmen geschrieben. Die Bilder können direkt vom Handy oder vom Computer in ein exportiertes Samba-Verzeichnis übertragen werden, wo ein Konverter-Skript läuft, welches die Bilder in das optimale Format für den Rahmen konvertiert.
Die PiShow basiert auf der libsdl2 (2.0.3) und benötigt kein X-Windows, aerbeitet also direkt mit dem Framebuffer. 

# Schritte zur Installation

- Aktuelles Raspberry OS installieren
- sudo apt-get install python3 lsof git scons libsdl2-dev cmake automake samba
- sudo apt-get install libasound2-dev libpulse-dev libturbojpeg-dev imagemagick exiftool
- sudo apt-get install libsdl2-dev 
- git clone https://github.com/Superberti/PiShow

- Sourcecodes runterladen, kompilieren und installieren:
  - SDL2_gfx:
    - http://www.ferzkopp.net/Software/SDL2_gfx/SDL2_gfx-1.0.4.tar.gz
    - herunterladen, entpacken und ins Verzeichnis wechseln
    - ```cp /usr/share/automake-1.16/config.guess .``` (veraltetes config.guess ersetzen)
    - ```./configure --disable-mmx```
    - ```make -j 4```
    - ```sudo make install```
  
  - SDL2_image:
    - https://www.libsdl.org/projects/SDL_image/release/SDL2_image-2.0.5.tar.gz
    - herunterladen, entpacken und ins Verzeichnis wechseln
    - ```./configure```
    - ```make -j 4```
    - ```sudo make install```

  - SDL2_ttf:
    - https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-2.0.18.tar.gz
    - herunterladen, entpacken und ins Verzeichnis wechseln
    - ```./configure```
    - ```make -j 4```
    - ```sudo make install```

  - SDL_mixer:
    - https://github.com/libsdl-org/SDL_mixer/releases/download/release-2.8.1/SDL2_mixer-2.8.1.tar.gz
    - herunterladen, entpacken und ins Verzeichnis wechseln
    - ```./configure```
    - ```make -j 4```
    - ```sudo make install```

  - WiringPi:
    - https://github.com/WiringPi/WiringPi/releases/download/3.14/wiringpi_3.14_arm64.deb
    - ```sudo apt install ./wiringpi-3.0-1.deb```
  - ```sudo ldconfig``` (damit die neu compilierten Dlls auch gefunden werden)


- Jetzt kann man in das Verzeichnis PiShow wechseln und mit "scons" die PiShow kompilieren.
- Infrarot-Fernbedienungs-Support: sudo apt-get install lirc
- sudo nano /boot/config.txt, Zeile "dtoverlay=lirc-rpi,gpio_out_pin=17,gpio_in_pin=18,gpio_in_pull=up" einfügen
- dann /etc/lirc/lirc_options.conf bearbeiten und "driver = default" und "device = /dev/lirc0" setzen

- Samba konfigurieren (exportiert das SMB-Verzeichnis für das Konversationsskript)
  - ```[global]
    workgroup = WORKGROUP
    security = user
    encrypt passwords = yes
    client min protocol = SMB2
    client max protocol = SMB3

    [Bilder]
    comment = Bilder zur Konversion
    path = /samba/public
    read only = no

- Autostart der PiShow:
    - /etc/rc.local ergänzen:
      - ```# Software für das Anzeigen der Bilder auf dem Bilderrahmen starten
            /home/pi/PiShow/PiShow -r -t 10 -l /home/pi/Pictures/Anzeige/conv &
            # Python-Script für das automatische Konvertieren der Bilder starten
            (/home/pi/PiShow/ConvertImages.py) &>> /var/log/convert_images.log
      - nicht vergessen die Pfade für den aktuellen Nutzer (hier "pi") und das Verzeichnis der durch das Skript konvertierten Bilder (hier "Pictures/Anzeige/conv") anzupassen
