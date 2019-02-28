import re
import os
import uuid

def make_inno_setup_script(version):
    app_name = 'PyMOL'
    app_version = short_version = version
    parsed_version = re.match(r'^(?P<main>\d+(\.\d+)*)\+.*\.(?P<build>\d+)(?P<suffix>[A-Z]+)?$', app_version, re.I)
    if parsed_version:
        short_version = '%s+VR.%02i%s' % (parsed_version.group('main'), int(parsed_version.group('build')), parsed_version.group('suffix').lower())
    app_folder = app_name + '-' + app_version
    app_guid = str(uuid.uuid3(uuid.NAMESPACE_URL, app_name + ' ' + app_version)).upper()
    with open(os.path.join('build', app_folder + '.iss'), 'w') as f:
        f.write('''\
#define MyAppName "%(name)s"
#define MyAppVersion "%(version)s"
#define MyAppShortVersion "%(short_version)s"
#define MyAppPublisher "Open-Source"
#define MyAppURL "https://github.com/schrodinger/pymol-open-source"
#define MyAppExeName "pymol.bat"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{%(guid)s}
AppName={#MyAppName}
AppVersion={#MyAppShortVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
SetupIconFile=pymol.ico
UninstallDisplayIcon={app}\\pymol.ico
DefaultDirName={pf}\\{#MyAppName} {#MyAppShortVersion}
DefaultGroupName={#MyAppName} {#MyAppShortVersion}
AllowNoIcons=yes
LicenseFile={#MyAppName}-{#MyAppVersion}\\%(pythonsubdir)s\\Lib\\site-packages\\pymol\\pymol_path\\LICENSE
InfoBeforeFile={#MyAppName}-{#MyAppVersion}\\%(pythonsubdir)s\\Lib\\site-packages\\pymol\\pymol_path\\README-VR
OutputDir=.
OutputBaseFilename={#MyAppName}-{#MyAppVersion}
Compression=lzma
SolidCompression=yes
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "{#MyAppName}-{#MyAppVersion}\\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "pymol.ico"; DestDir: "{app}"; Flags: ignoreversion 

[Icons]
Name: "{group}\\{#MyAppName} {#MyAppShortVersion}"; Filename: "{app}\\%(pythonsubdir)s\\Scripts\\{#MyAppExeName}"; WorkingDir: "{app}\\%(pythonsubdir)s\\Lib\\site-packages\\pymol\\pymol_path\\"; IconFilename: "{app}\\pymol.ico"
Name: "{group}\\{cm:UninstallProgram,{#MyAppName} {#MyAppShortVersion}}"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\\%(pythonsubdir)s\\Scripts\\{#MyAppExeName}"; WorkingDir: "{app}\\%(pythonsubdir)s\\Lib\\site-packages\\pymol\\pymol_path\\"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
''' % {
        'name': app_name,
        'version': app_version,
        'short_version': short_version,
        'guid': app_guid,
        'pythonsubdir': 'python-2.7.13.amd64', # for WinPython-64bit-2.7.13.1Zero distribution
    })
