; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
AppName=Graphite Compiler
AppVerName=Graphite Compiler 2.4
AppPublisher=SIL International
AppPublisherURL=http://graphite.sil.org/
AppSupportURL=http://graphite.sil.org/
AppUpdatesURL=http://graphite.sil.org/
DefaultDirName={pf}\Graphite Compiler
; Start Menu item name:
DefaultGroupName=Graphite Compiler 2.4
; allows them to say they don't want a start menu item:
AllowNoIcons=yes
; installer file name:
OutputBaseFilename=grcompiler_setup_2_4
Compression=lzma
SolidCompression=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "C:\silgraphite_2_0\grcompiler\release\GrCompiler.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\silgraphite_2_0\grcompiler\release\icuuc36.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\silgraphite_2_0\grcompiler\gdlpp.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\silgraphite_2_0\grcompiler\installer\readme.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\silgraphite_2_0\grcompiler\compiler\stddef.gdh"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\silgraphite_2_0\grcompiler\installer\gr_buildbat.bat"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\silgraphite_2_0\grcompiler\doc\CompilerDebugFiles.pdf"; DestDir: "{app}\doc"; Flags: ignoreversion
Source: "C:\silgraphite_2_0\grcompiler\doc\CppDoc.pdf"; DestDir: "{app}\doc"; Flags: ignoreversion
Source: "C:\silgraphite_2_0\grcompiler\doc\GDL.pdf"; DestDir: "{app}\doc"; Flags: ignoreversion
Source: "C:\silgraphite_2_0\grcompiler\doc\GTF_3_0.rtf"; DestDir: "{app}\doc"; Flags: ignoreversion
Source: "C:\silgraphite_2_0\grcompiler\doc\GDL_BNF.rtf"; DestDir: "{app}\doc"; Flags: ignoreversion
Source: "C:\silgraphite_2_0\grcompiler\doc\GraphiteOverview.pdf"; DestDir: "{app}\doc"; Flags: ignoreversion
Source: "C:\silgraphite_2_0\grcompiler\doc\StackMachineCommands.rtf"; DestDir: "{app}\doc"; Flags: ignoreversion
Source: "C:\silgraphite_2_0\grcompiler\doc\TransductionLog.rtf"; DestDir: "{app}\doc"; Flags: ignoreversion
Source: "C:\silgraphite_2_0\grcompiler\installer\example\stddr.ttf"; DestDir: "{app}\example"; Flags: ignoreversion
Source: "C:\silgraphite_2_0\grcompiler\installer\example\allcaps.gdl"; DestDir: "{app}\example"; Flags: ignoreversion
Source: "C:\silgraphite_2_0\grcompiler\installer\example\makecaps.wpx"; DestDir: "{app}\example"; Flags: ignoreversion
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{group}\Run GrCompiler"; Filename: "{app}\GrCompiler.exe"
Name: "{group}\Read-Me"; Filename: "{app}\readme.txt"
Name: "{group}\GDL documentation"; Filename: "{app}\doc\GDL.pdf"
Name: "{group}\Compiler Debug Files Doc"; Filename: "{app}\doc\CompilerDebugFiles.pdf"
Name: "{commondesktop}\Graphite Compiler"; Filename: "{app}\GrCompiler.exe"; Tasks: desktopicon

