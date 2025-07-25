; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define MyAppName "Partiels"
#define MyAppPublisher "Ircam"
#define MyAppURL "www.ircam.fr"
#define MyAppExeName "Partiels.exe"

[Setup]
; NOTE: The value of AppId uniquely identifies this application. Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{2BE88D38-04D3-44AE-B6F6-2D78BD410D58}}
AppName={#MyAppName}
AppVerName={#MyAppVerName}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf64}\{#MyAppName}
DisableProgramGroupPage=yes
DisableDirPage=no
OutputDir=.
OutputBaseFilename={#MyAppName}-Windows
InfoBeforeFile={#MyBinaryDir}\Install.txt
Compression=lzma
SolidCompression=yes
WizardStyle=modern
WizardImageFile={#MyBinaryDir}\Ircam-logo-noir-RS.bmp

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#MyBinaryDir}\Partiels.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#MyBinaryDir}\PlugIns\partiels-vamp-plugins.dll"; DestDir: "{app}\PlugIns"; Flags: ignoreversion
Source: "{#MyBinaryDir}\PlugIns\partiels-vamp-plugins.cat"; DestDir: "{app}\PlugIns"; Flags: ignoreversion
Source: "{#MyBinaryDir}\Templates\FactoryTemplate.ptldoc"; DestDir: "{app}\Templates"; Flags: ignoreversion
Source: "{#MyBinaryDir}\Translations\*"; DestDir: "{app}\Translations"; Flags: ignoreversion
Source: "{#MyBinaryDir}\Scripts\*"; DestDir: "{app}\Scripts"; Flags: ignoreversion
Source: "{#MyBinaryDir}\About.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#MyBinaryDir}\ChangeLog.txt"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
