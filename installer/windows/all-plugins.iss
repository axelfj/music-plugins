; One installer with every plugin. CI generates all-plugins-files.iss next to this script (one
; component, file entries and renamed-plugin cleanup per plugin) and passes /DBundleVersion=<version>.
; Installs the VST3 and CLAP into the same shared folders as the single-plugin installers.

#ifndef BundleVersion
  #define BundleVersion "0.0.0"
#endif

[Setup]
AppId={{com.rommuser.AllPlugins}
AppName=ROMMUSER Plugins
AppVersion={#BundleVersion}
AppPublisher=ROMMUSER
AppPublisherURL=https://studio.rommuser.com/plugins/
AppSupportURL=https://github.com/axelfj/music-plugins
DefaultDirName={autopf}\ROMMUSER\Plugins
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=..\..\dist
OutputBaseFilename=ROMMUSER-Plugins-Windows-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName=ROMMUSER Plugins
LicenseFile=..\..\LICENSE

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Files]
Source: "..\..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion

#include "all-plugins-files.iss"

[Messages]
english.FinishedLabel=The ROMMUSER plugins are installed. Open your DAW and rescan plugins if they do not show up.%n%nFree and open source, by ROMMUSER.
spanish.FinishedLabel=Los plugins de ROMMUSER quedaron instalados. Abre tu DAW y vuelve a escanear los plugins si no aparecen.%n%nGratis y open source, por ROMMUSER.
