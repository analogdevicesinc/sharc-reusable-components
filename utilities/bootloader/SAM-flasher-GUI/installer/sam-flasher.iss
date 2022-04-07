; Installation of the SAM Flasher HMI

; SEE THE DOCUMENTATION FOR DETAILS ON CREATING .ISS SCRIPT FILES!

#define APP_Name     "SAM Flasher"
#define APP_Version  "0.0.5"

[Setup]
AppId={{DE7DE958-623E-4083-A3F4-0AD9594A664D}
AppName=SAM Flasher
AppPublisher=Analog Devices
AppVersion={#APP_Version}
DefaultDirName={pf}\{#APP_Name}
DefaultGroupName={#APP_Name}
UninstallDisplayIcon={app}\sam-flasher.exe
Compression=lzma2
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64
OutputBaseFilename=sam-flasher_v{#APP_Version}_setup
SetupIconFile=..\mingw\flash_drive_usb_icon_0.ico

; Inno pascal functions for determining the processor type.
; you can use these to use (in an inno "check" parameter for example) to
; customize the installation depending on the architecture.


[Code]
function IsX64: Boolean;
begin
  Result := Is64BitInstallMode and (ProcessorArchitecture = paX64);
end;

function IsI64: Boolean;
begin
  Result := Is64BitInstallMode and (ProcessorArchitecture = paIA64);
end;

function IsX86: Boolean;
begin
  Result := not IsX64 and not IsI64;
end;

[Files]
; architecture dependent files
Source: "..\bin\mingw-pkg\sam-flasher.exe"; DestDir: "{app}"; DestName: "sam-flasher.exe"; Flags: 64bit; Check: IsX64
Source: "..\bin\mingw-pkg\libgcc_s_seh-1.dll"; DestDir: "{app}"; Check: IsX64
Source: "..\bin\mingw-pkg\libjpeg-8.dll"; DestDir: "{app}"; Check: IsX64
Source: "..\bin\mingw-pkg\liblzma-5.dll"; DestDir: "{app}"; Check: IsX64
Source: "..\bin\mingw-pkg\libpng16-16.dll"; DestDir: "{app}"; Check: IsX64
Source: "..\bin\mingw-pkg\libstdc++-6.dll"; DestDir: "{app}"; Check: IsX64
Source: "..\bin\mingw-pkg\libtiff-5.dll"; DestDir: "{app}"; Check: IsX64
Source: "..\bin\mingw-pkg\libwinpthread-1.dll"; DestDir: "{app}"; Check: IsX64
Source: "..\bin\mingw-pkg\libzstd.dll"; DestDir: "{sys}"; Check: IsX64
Source: "..\bin\mingw-pkg\wxbase30u_gcc_custom.dll"; DestDir: "{app}"; Check: IsX64
Source: "..\bin\mingw-pkg\wxmsw30u_core_gcc_custom.dll"; DestDir: "{app}"; Check: IsX64
Source: "..\bin\mingw-pkg\zlib1.dll"; DestDir: "{sys}"; Check: IsX64

[Icons]
Name: "{group}\SAM Flasher"; Filename: "{app}\sam-flasher.exe"; WorkingDir: "{app}"
