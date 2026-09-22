import os
import re
import json

def update_editor_settings():
    ini_path = os.path.expandvars(r"%LOCALAPPDATA%\UnrealEngine\5.8\Saved\Config\WindowsEditor\EditorSettings.ini")
    if os.path.exists(ini_path):
        with open(ini_path, 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()
        
        filtered = []
        for line in lines:
            # remove old non-repo LockdownZone
            if 'RecentlyOpenedProjectFiles' in line and 'LockdownZone/LockdownZone' in line:
                continue
            if 'RecentlyOpenedProjectFiles' in line and 'LockdownZoneRepo' in line:
                continue
            filtered.append(line.rstrip('\r\n'))
        
        # Insert our LockdownZoneRepo as the top recently opened project under [/Script/UnrealEd.EditorSettings]
        output = []
        for line in filtered:
            output.append(line)
            if line.strip() == '[/Script/UnrealEd.EditorSettings]':
                output.append('RecentlyOpenedProjectFiles=(ProjectName="C:/UEProjects/LockdownZoneRepo/LockdownZone.uproject",LastOpenTime=2026.09.23-04.00.00)')
                output.append('CreatedProjectPaths=C:/UEProjects')
                output.append('CreatedProjectPaths=C:/UEProjects/LockdownZoneRepo')
        
        with open(ini_path, 'w', encoding='utf-8') as f:
            f.write('\n'.join(output) + '\n')
        print(f"Updated {ini_path}")

def update_game_user_settings(subfolder):
    ini_path = os.path.expandvars(rf"%LOCALAPPDATA%\EpicGamesLauncher\Saved\Config\{subfolder}\GameUserSettings.ini")
    if os.path.exists(ini_path):
        with open(ini_path, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
        
        # Ensure [Launcher] section exists and has CreatedProjectPaths
        if '[Launcher]' in content:
            # clean up any corrupted lines
            lines = content.splitlines()
            cleaned = []
            for l in lines:
                if 'CreatedProjectPaths' in l:
                    continue
                cleaned.append(l)
            
            output = []
            for l in cleaned:
                output.append(l)
                if l.strip() == '[Launcher]':
                    output.append('CreatedProjectPaths=C:/UEProjects')
                    output.append('CreatedProjectPaths=C:/UEProjects/LockdownZoneRepo')
                    docs_path = os.path.expandvars(r"%USERPROFILE%\Documents\Unreal Projects").replace('\\', '/')
                    output.append(f'CreatedProjectPaths={docs_path}')
            with open(ini_path, 'w', encoding='utf-8') as f:
                f.write('\n'.join(output) + '\n')
            print(f"Updated {ini_path}")

def update_project_editor_records():
    json_path = os.path.expandvars(r"%LOCALAPPDATA%\UnrealEngine\Editor\ProjectEditorRecords.json")
    os.makedirs(os.path.dirname(json_path), exist_ok=True)
    engine_exe = r"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
    base_dir = r"C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/"
    
    data = {"Projects": {}}
    if os.path.exists(json_path):
        try:
            with open(json_path, 'r', encoding='utf-8') as f:
                data = json.load(f)
        except Exception:
            data = {"Projects": {}}
    
    projects = data.setdefault("Projects", {})
    projects["LastAccessed"] = "2026.09.23-04.00.00"
    projects["C:/UEProjects/LockdownZoneRepo/LockdownZone.uproject"] = {
        "EngineLocation": engine_exe,
        "BaseDir": base_dir,
        "LastAccessed": "2026.09.23-04.00.00"
    }
    
    # Remove old deprecated folder if present
    projects.pop("C:/UEProjects/LockdownZone/LockdownZone.uproject", None)
    
    with open(json_path, 'w', encoding='utf-8') as f:
        json.dump(data, f, indent=4)
    print(f"Updated {json_path}")

def update_uprojectdirs():
    path = r"C:\UEProjects\.uprojectdirs"
    with open(path, 'w', encoding='utf-8') as f:
        f.write("LockdownZoneRepo/\n./\n")
    print(f"Updated {path}")

if __name__ == '__main__':
    update_editor_settings()
    update_game_user_settings('Windows')
    update_game_user_settings('WindowsEditor')
    update_project_editor_records()
    update_uprojectdirs()
    print("All Epic / Unreal project records updated successfully!")
