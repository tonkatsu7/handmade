# Getting started

## If cloning the repo

1. `mkdir c:\dev`
2. `cd c:\dev`
3. `git clone https://github.com/tonkatsu7/handmade.git`

## Windows Startup folder

[How to Access the Windows 10 Startup Folder](https://www.howtogeek.com/754239/how-to-access-the-windows-10-startup-folder/)

1. Locate personal startup folder
   1.  Run... `shell:startup`
   2.  For example, on my Razer this was `C:\Users\sipha\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\Startup`
2.  Create `startup.bat` inside
    1.  Paste the contents
        ```cmd
        @echo off
        subst w: c:\dev
        ```

## CMD shortcut that executes shell.bat

1. Copy CMD shortcut to desktop
   1. Right click shortcut properties
   2. Target = `%windir%\system32\cmd.exe /k w:\handmade\misc\shell.bat`