import winreg
import os

uvs = r"C:\Program Files\Epic Games\Launcher\Engine\Binaries\Win64\UnrealVersionSelector.exe"
hkcu = winreg.HKEY_CURRENT_USER

def set_val(path, val, name=""):
    # ensure parent keys exist
    parts = path.split('\\')
    cur = ""
    for part in parts:
        cur = f"{cur}\\{part}" if cur else part
        k = winreg.CreateKey(hkcu, cur)
        winreg.CloseKey(k)
    k = winreg.OpenKey(hkcu, path, 0, winreg.KEY_SET_VALUE)
    winreg.SetValueEx(k, name, 0, winreg.REG_SZ, val)
    winreg.CloseKey(k)

try:
    set_val(r"Software\Classes\.uproject", "Unreal.ProjectFile")
    set_val(r"Software\Classes\Unreal.ProjectFile", "Unreal Engine Project File")
    set_val(r"Software\Classes\Unreal.ProjectFile\DefaultIcon", f'"{uvs}",0')
    
    set_val(r"Software\Classes\Unreal.ProjectFile\shell\open", "Open with Unreal Engine")
    set_val(r"Software\Classes\Unreal.ProjectFile\shell\open\command", f'"{uvs}" /editor "%1"')
    
    set_val(r"Software\Classes\Unreal.ProjectFile\shell\run", "Launch Game")
    set_val(r"Software\Classes\Unreal.ProjectFile\shell\run\command", f'"{uvs}" /game "%1"')
    
    set_val(r"Software\Classes\Unreal.ProjectFile\shell\rungenproj", "Generate Visual Studio project files")
    set_val(r"Software\Classes\Unreal.ProjectFile\shell\rungenproj\command", f'"{uvs}" /projectfiles "%1"')
    
    set_val(r"Software\Classes\Unreal.ProjectFile\shell\switchversion", "Switch Unreal Engine version...")
    set_val(r"Software\Classes\Unreal.ProjectFile\shell\switchversion\command", f'"{uvs}" /switchversion "%1"')
    
    print("HKCU shell associations written successfully!")
except Exception as e:
    print("Error setting registry:", e)
