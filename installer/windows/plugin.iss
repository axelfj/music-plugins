; Inno Setup script shared by every plugin. CI passes the defines:
;   /DPluginName="Clip Ready" /DPluginSlug=ClipReady /DPluginVersion=1.0.0 /DArtefacts=<path to ..._artefacts\Release>
; Installs the VST3 and CLAP into the standard shared folders, where every DAW already looks.

#ifndef PluginName
  #error PluginName not defined
#endif

[Setup]
AppId={{com.rommuser.{#PluginSlug}}
AppName={#PluginName}
AppVersion={#PluginVersion}
AppPublisher=ROMMUSER
AppPublisherURL=https://rommuser.com
AppSupportURL=https://github.com/axelfj/music-plugins
DefaultDirName={autopf}\ROMMUSER\{#PluginName}
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=..\..\dist
OutputBaseFilename={#PluginSlug}-Setup-{#PluginVersion}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName={#PluginName}
LicenseFile=..\..\LICENSE

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Components]
Name: "vst3"; Description: "VST3 (FL Studio, Ableton Live, Cubase, Studio One...)"; Types: full compact custom; Flags: fixed
Name: "clap"; Description: "CLAP (Bitwig, REAPER, FL Studio 21+)"; Types: full

[Files]
Source: "{#Artefacts}\VST3\{#PluginName}.vst3\*"; DestDir: "{commoncf64}\VST3\{#PluginName}.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Artefacts}\CLAP\{#PluginName}.clap"; DestDir: "{commoncf64}\CLAP"; Components: clap; Flags: ignoreversion
Source: "..\..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion

[Messages]
english.FinishedLabel=%1 is installed. Open your DAW and rescan plugins if it does not show up.%n%nFree and open source, by ROMMUSER.
spanish.FinishedLabel=%1 quedó instalado. Abre tu DAW y vuelve a escanear los plugins si no aparece.%n%nGratis y open source, por ROMMUSER.
