import wave,math,random,struct
from pathlib import Path
rng=random.Random(29);sr=22050;duration=.85;samples=[]
for i in range(int(sr*duration)):
 t=i/sr;phase=2*math.pi*(150*t-35*t*t)
 envelope=min(1,t*15)*max(0,1-t/duration)
 v=(.3*math.sin(phase)+.15*math.sin(2.03*phase)+.08*rng.uniform(-1,1))*envelope
 samples.append(int(v*32767))
p=Path(__file__).resolve().parents[1]/'SourceAssets/ChapterAudio/Howl.wav'
with wave.open(str(p),'wb') as w:w.setparams((1,2,sr,0,'NONE','not compressed'));w.writeframes(struct.pack('<'+'h'*len(samples),*samples))
