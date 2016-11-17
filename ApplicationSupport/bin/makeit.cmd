@echo off
echo !!!!unzip!!!!
ren lwip_lpc\lwip-1.4.0 lwip_lpc\lwip
cd lwip_lpc\contrib-1.4.0\apps\httpserver_raw\makefsdata
cl /EHsc /DWIN32 /I..\..\..\..\lwip\src\include /I ..\..\..\ports\win32\include /I..\..\..\..\lwip\src\include\ipv4 makefsdata.c 
