"""Read-only audit of serialized instancing usage on every generated palette material."""
import json
import os
import unreal

root = "/Game/Art/ZeroTower/Materials"
usage = unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES
report = []
for path in unreal.EditorAssetLibrary.list_assets(root, recursive=False, include_folder=False):
    material = unreal.EditorAssetLibrary.load_asset(path)
    if isinstance(material, unreal.Material):
        enabled = unreal.MaterialEditingLibrary.has_material_usage(material, usage)
        serialized_flag = bool(material.get_editor_property("used_with_instanced_static_meshes"))
        report.append({"material": material.get_path_name(), "instanced_static_meshes": enabled, "serialized_flag": serialized_flag})
        unreal.log("ZERO_TOWER_USAGE " + json.dumps(report[-1], sort_keys=True))
output = os.path.abspath(os.path.join(unreal.Paths.project_saved_dir(), "ZeroTowerMaterialUsageAudit.json"))
with open(output, "w", encoding="utf-8") as stream:
    json.dump(report, stream, indent=2)
if len(report) != 10 or not all(row["instanced_static_meshes"] and row["serialized_flag"] for row in report):
    raise RuntimeError("Zero Tower instancing usage audit failed: " + output)
unreal.log("ZERO_TOWER_USAGE_AUDIT_COMPLETE all_10_enabled " + output)
