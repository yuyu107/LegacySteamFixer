@echo off
zig cc -target x86-windows-gnu -O2 -Wall -Wextra -Werror -ffreestanding -fno-builtin -fno-stack-protector -c unified_i18n.c -o unified.o
if errorlevel 1 exit /b 1
zig cc -target x86-windows-gnu -nostdlib -Wl,--subsystem,windows -Wl,--entry,WinMain@16 -Wl,--major-os-version,6 -Wl,--minor-os-version,1 -Wl,--major-subsystem-version,6 -Wl,--minor-subsystem-version,1 unified.o -lkernel32 -luser32 -ladvapi32 -o OldSteam_VSZa_Launcher.exe
