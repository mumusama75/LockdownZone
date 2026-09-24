import math,random,wave,struct
from pathlib import Path
random.seed(104)
out=Path('SourceAssets/ChapterAudio');out.mkdir(exist_ok=True)
for name,duration in [('RadioCall',2.5),('CardBeep',.18),('SoftDoor',.55),('DoorTap',.25),('BottleBreak',.7)]:
 sr=22050;data=[];filtered=0
 for i in range(int(sr*duration)):
  t=i/sr;n=random.uniform(-1,1);filtered=.7*filtered+.3*n
  if name=='RadioCall':
   burst=(.1<t<.65 or 1.05<t<1.85 or 2.15<t<2.35)
   v=(filtered*.4+math.sin(t*math.tau*440)*.03)*burst
  elif name=='CardBeep':v=.18*math.sin(t*math.tau*780)*math.sin(math.pi*t/duration)
  elif name=='SoftDoor':v=.12*filtered*math.sin(math.pi*t/duration)
  elif name=='DoorTap':v=(.2*math.sin(t*math.tau*180)+.08*n)*math.exp(-t*22)
  else:v=(.6*n+.2*math.sin(t*math.tau*2450)+.15*math.sin(t*math.tau*3710))*math.exp(-t*7)
  data.append(int(max(-1,min(1,v))*32767))
 with wave.open(str(out/(name+'.wav')),'wb') as w:w.setparams((1,2,sr,0,'NONE','not compressed'));w.writeframes(struct.pack('<'+'h'*len(data),*data))
