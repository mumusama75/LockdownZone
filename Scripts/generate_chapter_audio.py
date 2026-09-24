import math,random,wave,struct
from pathlib import Path
random.seed(17)
out=Path('SourceAssets/ChapterAudio');out.mkdir(parents=True,exist_ok=True)
for name,duration,freq in [('Clang',.65,730),('Rattle',.45,390),('Step',.12,120),('Crash',1.6,65),('Motor',4,85),('Relay',.2,850),('Spark',.23,2300)]:
 sr=22050;samples=[]
 for i in range(int(sr*duration)):
  t=i/sr;n=random.uniform(-1,1)
  if name=='Motor':v=(.3*math.sin(2*math.pi*freq*t)+.1*n)*min(1,t*6)*min(1,(duration-t)*3)
  else:v=(.45*math.sin(2*math.pi*freq*t)+.25*math.sin(2*math.pi*freq*1.47*t)+.3*n)*math.exp(-t*6/duration)
  if name=='Step':v*=.2
  samples.append(int(max(-1,min(1,v))*.65*32767))
 with wave.open(str(out/(name+'.wav')),'wb') as w:w.setparams((1,2,sr,0,'NONE','not compressed'));w.writeframes(struct.pack('<'+'h'*len(samples),*samples))
