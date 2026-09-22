import os
import unreal

project_root = os.path.abspath(unreal.Paths.project_dir())
source_root = os.path.abspath(os.path.expandvars(os.path.expanduser(
    os.environ.get("LZ_KENNEY_SOURCE") or os.path.join(
        project_root, "ExternalAssets", "KenneyFurniture", "Models", "FBX format"
    )
)))
destination = "/Game/Art/KenneyFurniture"
names = [
    "desk", "deskCorner", "chairDesk", "chairModernFrameCushion",
    "computerScreen", "computerKeyboard", "computerMouse", "laptop",
    "bookcaseOpen", "bookcaseClosed", "books", "cardboardBoxClosed",
    "cardboardBoxOpen", "loungeSofa", "loungeChair", "tableCoffee",
    "plantSmall1", "pottedPlant", "lampSquareCeiling", "trashcan",
]

if not os.path.isdir(source_root):
    raise RuntimeError(
        "Kenney furniture source directory does not exist: " + source_root
        + ". Set LZ_KENNEY_SOURCE to the directory containing the source FBX files."
    )

# Validate every missing asset's source before starting any import. Existing
# project assets remain authoritative and are never replaced by this script.
pending = []
for name in names:
    asset_path = destination + "/SM_" + name
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        unreal.log("Keeping existing furniture asset: " + asset_path)
        continue
    filename = os.path.join(source_root, name + ".fbx")
    if not os.path.isfile(filename):
        raise RuntimeError("Missing furniture FBX for " + asset_path + ": " + filename)
    pending.append((name, filename))

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
tasks = []
for name, filename in pending:
    task = unreal.AssetImportTask()
    task.filename = filename
    task.destination_path = destination
    task.destination_name = "SM_" + name
    task.automated = True
    task.replace_existing = False
    task.save = True
    task.options = unreal.FbxImportUI()
    task.options.import_mesh = True
    task.options.import_as_skeletal = False
    task.options.import_materials = True
    task.options.import_textures = True
    task.options.static_mesh_import_data.combine_meshes = True
    task.options.static_mesh_import_data.generate_lightmap_u_vs = True
    tasks.append(task)

if tasks:
    asset_tools.import_asset_tasks(tasks)
    failed = [name for name, _ in pending if not unreal.EditorAssetLibrary.does_asset_exist(
        destination + "/SM_" + name
    )]
    if failed:
        raise RuntimeError("Furniture import failed for: " + ", ".join(failed))
unreal.log("Furniture assets imported: " + str(len(tasks)))
unreal.log("LOCKDOWN_ZONE_FURNITURE_IMPORT_COMPLETE")
