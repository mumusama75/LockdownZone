"""Read mesh bounds/material slots without modifying or saving any UE asset.

Commandlet:
  UnrealEditor-Cmd.exe LockdownZone.uproject -run=pythonscript \
    -script=Scripts/audit_assets.py -NullRHI -unattended \
    -abslog=<project>/Saved/Logs/AssetAudit.log

Writes only <project>/Saved/AssetAudit.json.
"""
import json
import os

import unreal


LIB = unreal.EditorAssetLibrary
ROOTS = ("/Game/Art/KenneyFurniture", "/Game/Art/ZeroTower")
PISTOL = "/Game/Weapons/Pistol/Meshes/SM_Pistol"
DEFAULT_MATERIAL_NAMES = {
    "DefaultMaterial", "WorldGridMaterial", "BasicShapeMaterial",
}


def vector_values(vector):
    return {axis: round(float(getattr(vector, axis)), 4) for axis in ("x", "y", "z")}


paths = {PISTOL}
for root in ROOTS:
    for path in LIB.list_assets(root, recursive=True, include_folder=False):
        if path.rsplit("/", 1)[-1].startswith("SM_"):
            paths.add(path)

report = {
    "project_directory": os.path.abspath(unreal.Paths.project_dir()),
    "read_only_asset_audit": True,
    "units": "centimetres",
    "meshes": [],
    "errors": [],
    "slot_material_mapping": {},
}
for path in sorted(paths):
    try:
        mesh = LIB.load_asset(path)
        if not isinstance(mesh, unreal.StaticMesh):
            raise RuntimeError("Missing or not a StaticMesh: " + path)
        bounds = mesh.get_bounds()
        entry = {
            "path": mesh.get_path_name(),
            "origin_cm": vector_values(bounds.origin),
            "extent_cm": vector_values(bounds.box_extent),
            "size_cm": {
                axis: round(float(getattr(bounds.box_extent, axis)) * 2.0, 4)
                for axis in ("x", "y", "z")
            },
            "materials": [],
        }
        for index, slot in enumerate(mesh.get_editor_property("static_materials")):
            material = slot.get_editor_property("material_interface")
            slot_name = str(slot.get_editor_property("material_slot_name"))
            material_path = material.get_path_name() if material else None
            is_default = bool(material and (
                material_path.startswith("/Engine/")
                and material.get_name() in DEFAULT_MATERIAL_NAMES
            ))
            material_entry = {
                "index": index,
                "material_slot_name": slot_name,
                "imported_material_slot_name": str(slot.get_editor_property("imported_material_slot_name")),
                "material_interface": material_path,
                "is_null": material is None,
                "is_default": is_default,
                "is_engine_material": bool(material_path and material_path.startswith("/Engine/")),
            }
            entry["materials"].append(material_entry)
            mapping = report["slot_material_mapping"].setdefault(slot_name, [])
            if material_path not in mapping:
                mapping.append(material_path)
            unreal.log("LZ_ASSET_SLOT " + json.dumps(
                {"mesh": mesh.get_name(), **material_entry}, ensure_ascii=True, sort_keys=True
            ))
        report["meshes"].append(entry)
        unreal.log("LZ_ASSET_BOUNDS " + json.dumps({
            "mesh": mesh.get_name(), "origin_cm": entry["origin_cm"], "size_cm": entry["size_cm"]
        }, sort_keys=True))
    except Exception as error:
        report["errors"].append({"path": path, "error": str(error)})
        unreal.log_error("LZ_ASSET_AUDIT_ERROR " + path + ": " + str(error))

all_slots = [slot for mesh in report["meshes"] for slot in mesh["materials"]]
report["summary"] = {
    "mesh_count": len(report["meshes"]),
    "slot_count": len(all_slots),
    "null_slot_count": sum(slot["is_null"] for slot in all_slots),
    "default_slot_count": sum(slot["is_default"] for slot in all_slots),
    "error_count": len(report["errors"]),
}
output = os.path.abspath(os.path.join(unreal.Paths.project_saved_dir(), "AssetAudit.json"))
os.makedirs(os.path.dirname(output), exist_ok=True)
with open(output, "w", encoding="utf-8") as stream:
    json.dump(report, stream, ensure_ascii=False, indent=2)
    stream.write("\n")
unreal.log("LZ_ASSET_AUDIT_SUMMARY " + json.dumps(report["summary"], sort_keys=True))
unreal.log("LZ_ASSET_AUDIT_COMPLETE " + output)
if report["errors"]:
    raise RuntimeError("Asset audit contains load errors; see " + output)
