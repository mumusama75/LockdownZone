"""Encode chronological UE viewport captures; never substitute a rendered mockup."""
from pathlib import Path
import csv,subprocess,wave,array,shutil
import imageio_ffmpeg
root=Path(__file__).resolve().parents[1]
out=root/'Docs/Portfolio/ElevatorFinale';out.mkdir(parents=True,exist_ok=True)
rows=[(float(t),Path(p)) for t,p in csv.reader((root/'Saved/ElevatorFinaleFrames.csv').open(encoding='utf-8-sig'))]
manifest=out/'frames.ffconcat'
parts=['ffconcat version 1.0']
for i,(t,p) in enumerate(rows):
 if not p.exists():raise FileNotFoundError(p)
 parts.append("file '"+p.as_posix().replace("'","'\\''")+"'")
 parts.append(f'duration {max(.016,(rows[i+1][0]-t) if i+1<len(rows) else .15):.6f}')
parts.append("file '"+rows[-1][1].as_posix()+"'")
manifest.write_text('\n'.join(parts),encoding='utf-8')
video_duration=rows[-1][0]-rows[0][0]+.15
wav=root/'Saved/ElevatorFinale.wav'
audio_ok=False
if wav.exists():
 with wave.open(str(wav),'rb') as w:
  audio_duration=w.getnframes()/w.getframerate();peak=max(map(abs,array.array('h',w.readframes(w.getnframes()))),default=0)
 audio_ok=peak>0 and abs(audio_duration-video_duration)<1
 print('Audio seconds/peak:',audio_duration,peak,'Video seconds:',video_duration,'Use audio:',audio_ok)
cmd=[imageio_ffmpeg.get_ffmpeg_exe(),'-y','-loglevel','error','-f','concat','-safe','0','-i',str(manifest)]
if audio_ok:cmd+=['-i',str(wav),'-c:a','aac','-af','apad']
cmd+=['-vf','fps=30','-c:v','libx264','-crf','20','-pix_fmt','yuv420p','-movflags','+faststart','-t',str(video_duration),str(out/'ElevatorFinale.mp4')]
subprocess.run(cmd,check=True)
for src in ['ElevatorFinaleQA.txt','ElevatorFinaleFrames.csv'] :shutil.copy2(root/'Saved'/src,out/src)
for i,tag in [(0,'Start'),(len(rows)//3,'Pry'),(int(len(rows)*.53),'Hand'),(int(len(rows)*.7),'Descent'),(-1,'Garage')]:shutil.copy2(rows[i][1],out/(tag+'.png'))
(out/'Capture.txt').write_text(f'Real UE runtime frames, approximately 10 captures/sec encoded at 30 fps using captured wall-clock intervals. No scene cuts. Duration {video_duration:.2f}s. Master audio valid and included: {audio_ok}. If false this is a SILENT capture; background audio recording was incomplete and has not been time-stretched or replaced.\n',encoding='utf-8')
print(out/'ElevatorFinale.mp4')
