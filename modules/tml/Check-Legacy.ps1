param([string]$Root)
$ErrorActionPreference = 'Stop'
$lines = New-Object System.Collections.Generic.List[string]
$code = 1
try {
 $dll = Join-Path $Root 'Libraries\steamworks.net.anycpu\2025.162.4\runtimes\win-x64\native\steam_api64.dll'
 $env:SteamAppId = '105600'
 $env:SteamGameId = '105600'
 $env:PATH = (Split-Path $dll) + ';' + $env:PATH
 Add-Type -TypeDefinition @'
using System;
using System.Text;
using System.Runtime.InteropServices;
public static class LegacyInterfaceCheck {
 [DllImport("kernel32.dll", CharSet=CharSet.Unicode, SetLastError=true)] public static extern IntPtr LoadLibraryW(string path);
 [DllImport("steam_api64.dll", CallingConvention=CallingConvention.Cdecl)] public static extern int SteamInternal_SteamAPI_Init(IntPtr versions, StringBuilder error);
 [DllImport("steam_api64.dll", CallingConvention=CallingConvention.Cdecl)] public static extern void SteamAPI_Shutdown();
}
'@
 if ([IntPtr]::Size -ne 8) { throw 'Please run the supplied 64-bit launcher.' }
 if ([LegacyInterfaceCheck]::LoadLibraryW($dll) -eq [IntPtr]::Zero) { throw ('LoadLibrary error=' + [Runtime.InteropServices.Marshal]::GetLastWin32Error()) }
 $interfaces = @('SteamUtils010','SteamNetworkingUtils004','STEAMAPPS_INTERFACE_VERSION008','SteamFriends017','SteamMatchGameSearch001','STEAMHTMLSURFACE_INTERFACE_VERSION_005','STEAMHTTP_INTERFACE_VERSION003','SteamInput006','STEAMINVENTORY_INTERFACE_V003','SteamMatchMakingServers002','SteamMatchMaking009','STEAMMUSICREMOTE_INTERFACE_VERSION001','STEAMMUSIC_INTERFACE_VERSION001','SteamNetworkingMessages002','SteamNetworkingSockets012','SteamNetworking006','STEAMPARENTALSETTINGS_INTERFACE_VERSION001','SteamParties002','STEAMREMOTEPLAY_INTERFACE_VERSION002','STEAMREMOTESTORAGE_INTERFACE_VERSION016','STEAMSCREENSHOTS_INTERFACE_VERSION003','STEAMUGC_INTERFACE_VERSION020','STEAMUSERSTATS_INTERFACE_VERSION013','SteamUser023','STEAMVIDEO_INTERFACE_V007')
 $list = ($interfaces -join [string][char]0) + [char]0 + [char]0
 $bytes = [Text.Encoding]::ASCII.GetBytes($list)
 $ptr = [Runtime.InteropServices.Marshal]::AllocHGlobal($bytes.Length)
 try {
  [Runtime.InteropServices.Marshal]::Copy($bytes,0,$ptr,$bytes.Length)
  $errorText = New-Object System.Text.StringBuilder 1024
  $result = [LegacyInterfaceCheck]::SteamInternal_SteamAPI_Init($ptr,$errorText)
  $lines.Add('Full legacy interface list: result=' + $result + '; ' + $errorText.ToString())
  if ($result -eq 0) { [LegacyInterfaceCheck]::SteamAPI_Shutdown(); $code = 0 }
 } finally { [Runtime.InteropServices.Marshal]::FreeHGlobal($ptr) }
} catch { $lines.Add($_.Exception.ToString()) }
$lines | ForEach-Object { Write-Output $_ }
[IO.File]::WriteAllLines((Join-Path $Root 'tML-LegacySteam-Check.txt'),$lines.ToArray(),[Text.Encoding]::UTF8)
exit $code
