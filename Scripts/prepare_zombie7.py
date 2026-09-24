"""Prepare Tony Flanagan's CC BY 4.0 model; retain original unchanged."""
import struct,json,pathlib
root=pathlib.Path(__file__).resolve().parents[1]
f=(root/'SourceAssets/Zombie7/source/zom_7.glb').read_bytes()
n=struct.unpack_from('<I',f,12)[0];d=json.loads(f[20:20+n]);off=20+n;size=struct.unpack_from('<I',f,off)[0];b=bytearray(f[off+8:off+8+size])
scaled=set()
def scale_accessor(i):
 if i in scaled:return
 scaled.add(i);a=d['accessors'][i];v=d['bufferViews'][a['bufferView']];assert a['componentType']==5126
 components={'VEC3':3,'MAT4':16}[a['type']];stride=v.get('byteStride',components*4)
 for j in range(a['count']):
  start=v.get('byteOffset',0)+a.get('byteOffset',0)+j*stride
  for k in (range(3) if components==3 else (0,1,2,4,5,6,8,9,10,12,13,14)):
   pos=start+k*4;struct.pack_into('<f',b,pos,struct.unpack_from('<f',b,pos)[0]*.01)
 for key in ['min','max']:
  if key in a:a[key]=[x*.01 for x in a[key]]
# Mesh positions are already meters; only the armature and inverse bind matrices used centimeters.
for skin in d['skins']:scale_accessor(skin['inverseBindMatrices'])
for a in d['animations']:
 for c in a['channels']:
  if c['target']['path']=='translation':scale_accessor(a['samplers'][c['sampler']]['output'])
for node in d['nodes']:
 if 'translation' in node:node['translation']=[x*.01 for x in node['translation']]
d['nodes'][68]['scale']=[1,1,1]
d['meshes'][0]['primitives']=[p for m in d['meshes'] for p in m['primitives']]
d['meshes']=[d['meshes'][0]]
d['meshes'][0]['name']='Zombie7';d['nodes'][65]['name']='Zombie7';d['nodes'][68]['children']=[65,64]
# Remove unused mesh nodes so Interchange cannot import detached clothes as extra assets.
for i in (66,67):d['nodes'][i].pop('mesh');d['nodes'][i].pop('skin')
# Navigation owns translation: remove accumulated planar travel while retaining sway/height.
for anim in d['animations']:
 if anim['name'] not in ('Walk','Run'):continue
 for channel in anim['channels']:
  if channel['target']['node']!=64 or channel['target']['path']!='translation':continue
  sampler=anim['samplers'][channel['sampler']];ac=d['accessors'][sampler['output']];v=d['bufferViews'][ac['bufferView']]
  start=v.get('byteOffset',0)+ac.get('byteOffset',0);stride=v.get('byteStride',12);count=ac['count']
  t=d['accessors'][sampler['input']];tv=d['bufferViews'][t['bufferView']];ts=tv.get('byteOffset',0)+t.get('byteOffset',0)
  times=[struct.unpack_from('<f',b,ts+i*4)[0] for i in range(count)]
  first=struct.unpack_from('<fff',b,start);last=struct.unpack_from('<fff',b,start+(count-1)*stride)
  for i in range(count):
   alpha=(times[i]-times[0])/(times[-1]-times[0])
   for k in (0,1):
    at=start+i*stride+k*4;value=struct.unpack_from('<f',b,at)[0]-(last[k]-first[k])*alpha;struct.pack_into('<f',b,at,value)
  ac.pop('min',None);ac.pop('max',None)
j=json.dumps(d,separators=(',',':')).encode();j+=b' '*((-len(j))%4);b+=b'\0'*((-len(b))%4)
out=root/'SourceAssets/Zombie7/Zombie7.glb';out.write_bytes(struct.pack('<III',0x46546c67,2,28+len(j)+len(b))+struct.pack('<II',len(j),0x4e4f534a)+j+struct.pack('<II',len(b),0x004e4942)+b)
print(out)
