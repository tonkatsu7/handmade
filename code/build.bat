@echo off

mkdir ..\..\build
pushd ..\..\build
cl -Zi ..\handmade\code\win32_handmade.cpp /LINK user32.lib
popd