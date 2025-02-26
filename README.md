# PiShow
Dieses Programm wurde zur Anzeige von Bildern auf einem selbstgebauten TFT-Bilderrahmen geschrieben. Die Bilder können direkt vom Handy oder vom Computer in ein exportiertes Samba-Verzeichnis übertragen werden, wo ein Konverter-Skript läuft, welches die Bilder in das optimale Format für den Rahmen konvertiert.
Die PiShow basiert auf der libsdl2 (2.0.3) und benötigt kein X-Windows, aerbeitet also direkt mit dem Framebuffer. 

# Schritte zur Installation

- Aktuelles Raspberry OS installieren
- sudo apt-get install lsof git scons libsdl2-dev cmake automake
- sudo apt-get install libasound2-dev libpulse-dev libturbojpeg-dev imagemagick exiftool
- sudo apt-get install libsdl2-dev 
- git clone https://github.com/Superberti/PiShow

- Sourcecodes runterladen:
-- SDL2_gfx:
  --- https://www.ferzkopp.net/wordpress/2016/01/02/sdl_gfx-sdl2_gfx/
  --- heruterladen und entpacken
  --- ins Verzeichnis wechseln
-- SDL2_image:
-- SDL2_ttf:
-- SDL_mixer:
- Sourcen entpacken und nach Anleitung kompilieren und installieren. "make -j 4" benutzt alle Kerne.
- Bei SDL2_gfx mit "./configure --disable-mmx" konfigurieren
- Jetzt kann man in das Verzeichnis PiShow wechseln und mit "scons" die PiShow kompilieren.
- Infrarot-Fernbedienungs-Support: sudo apt-get install lirc
- sudo nano /boot/config.txt, Zeile "dtoverlay=lirc-rpi,gpio_out_pin=17,gpio_in_pin=18,gpio_in_pull=up" einfügen
- dann /etc/lirc/lirc_options.conf bearbeiten und "driver = default" und "device = /dev/lirc0" setzen
