"""Copy audited GASP packages into this project without overwriting existing assets.
Run audit_gasp.py in the source UE project first. This script uses that saved registry
manifest and preflights all collisions before copying any file.
"""
from pathlib import Path
import argparse,json,shutil
root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser()
parser.add_argument('--source-content',type=Path,required=True)
args=parser.parse_args()
manifest=json.loads((root/'Saved/GASP-dependencies.json').read_text(encoding='utf-8'))
files=[]
for package in manifest['dependencies']:
    if not package.startswith('/Game/') or '..' in package:raise ValueError(package)
    for ext in ('.uasset','.uexp','.ubulk'):
        rel=Path(package[6:]+ext)
        src=args.source_content/rel;dest=root/'Content'/rel
        if not src.exists():continue
        if dest.exists() and dest.read_bytes()!=src.read_bytes():
            raise RuntimeError(f'Existing asset differs; no files copied: {dest}')
        files.append((src,dest))
for src,dest in files:
    dest.parent.mkdir(parents=True,exist_ok=True)
    if not dest.exists():shutil.copy2(src,dest)
(root/'Docs/Portfolio/GASP-import-manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print(f'Copied or verified {len(files)} files; existing assets preserved.')
