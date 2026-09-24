"""Export the design diagram from the runtime-authored wall/door CSV (stdlib only)."""
from pathlib import Path
import csv, html
root=Path(__file__).resolve().parents[1]
rows=list(csv.DictReader((root/'Saved/LZOfficeLayout.csv').open(encoding='utf-8-sig')))
parts=['<svg xmlns="http://www.w3.org/2000/svg" width="1440" height="1060" viewBox="0 0 1440 1060"><rect width="1440" height="1060" fill="#f5f3ee"/><style>text{font-family:Microsoft YaHei,Arial,sans-serif;fill:#253446}.room{font-size:17px;font-weight:600}.small{font-size:14px}.note{font-size:17px}</style><defs><marker id="arr" markerWidth="7" markerHeight="7" refX="5" refY="3" orient="auto"><path d="M0,0 L6,3 L0,6" fill="#c47514"/></marker></defs>']
def pos(x,y):return 655+x*.15,505-y*.15
def rect(x,y,w,d,color,stroke='none'):
 a,b=pos(x-w/2,y+d/2);parts.append(f'<rect x="{a}" y="{b}" width="{w*.15}" height="{d*.15}" fill="{color}" stroke="{stroke}"/>')
def text(x,y,t,cls='room'):
 a,b=pos(x,y);parts.append(f'<text x="{a}" y="{b}" text-anchor="middle" class="{cls}">{html.escape(t)}</text>')
def path(points,color,dash=False,width=4):
 pairs=['%g,%g'%pos(x,y) for x,y in points];parts.append(f'<polyline points="{" ".join(pairs)}" fill="none" stroke="{color}" stroke-width="{width}" stroke-linejoin="round" stroke-dasharray="{"10 7" if dash else "none"}"/>')
parts+=['<text x="80" y="60" font-size="32" font-weight="700">封锁区：零号大厦 / 办公层动线迭代</text>','<text x="80" y="97" class="note">2026.09.24 · 功能分区 → 搜索 → 返程回路 · 面试讲解图</text>','<text x="80" y="130" class="small">新增隔墙与门洞取自运行时 CSV；既有外壳按源码坐标绘制，家具仅示意。不是建筑施工图。</text>']
rect(0,0,7600,4200,'#e6e5e0','#253446')
for x,y,w,d,c in [(-3000,1500,1500,1100,'#d4e5ef'),(-1125,1700,950,700,'#d4e5ef'),(-450,1700,400,700,'#ead4e8'),(675,1700,1850,700,'#d4e5ef'),(2975,1500,1550,1200,'#edddbf'),(-3175,-1625,1050,850,'#dbe4dc'),(-2450,-1625,500,850,'#dbe4dc'),(-1425,-1625,1550,850,'#dbe4dc'),(250,-1625,1800,850,'#dbe4dc'),(2150,-1500,2000,1200,'#edddbf'),(-3150,0,1300,1500,'#d7dbe8')]:rect(x,y,w,d,c)
# Existing outer shell and protected start room.
for x,y,w,d in [(0,2100,7600,100),(0,-2100,7600,100),(-3800,0,100,4200),(3800,1250,100,1700),(3800,-1250,100,1700),(-3150,750,1300,80),(-3150,-750,1300,80),(-2520,652,40,216),(-2520,-400,40,190)]:rect(x,y,w,d,'#35404c')
rect(-2520,120,35,810,'#50a6bd')
rect(-2520,-610,40,230,'#f5f3ee')
for r in rows:
 rect(*[float(r[k]) for k in ['x','y','width','depth']], '#35404c' if r['kind']=='wall' else ('#198176' if r['kind']=='vault' else '#f5f3ee'))
text(-1850,-1400,'翻越捷径','small')
# Doors with changing connectivity.
rect(1150,-1100,35,240,'#198176');rect(-450,1350,200,35,'#9e538e')
rect(3850,0,700,540,'#d3e5d9','#35404c')
for y in [-500,500]:
 for i in range(5):rect(-1750+i*480,y,180,90,'#a5afbb')
for y in [-1710,-1350]:
 for i in range(4):rect(1500+i*330,y,80,100,'#667e8c')
# Circulation strokes are schematic, not an autonomous navigation recording.
path([(-3300,0),(-3300,-610),(-2300,-610),(-2300,-200),(1850,-200),(1850,-1050),(2050,-1050)],'#c47514')
path([(1850,-1050),(2800,-1050),(2800,-1540),(3350,-1540),(3350,600),(3000,600),(3000,1150),(2500,1150)],'#c47514')
path([(2500,1150),(3000,1150),(3000,0),(3650,0)],'#c47514')
path([(1850,-1100),(1150,-1100),(-2250,-1100),(-2250,-200)],'#198176',True)
path([(400,200),(400,1100),(500,1100),(500,1580)],'#9e538e',True)
for x,y,t in [(-3050,1550,'接待 / 停运电梯'),(-1100,1810,'主管'),(-450,1870,'资料'),(700,1900,'档案'),(2850,1830,'配电'),(-3150,150,'封控办公室'),(-300,80,'开放办公区 / 主通道'),(-3200,-1650,'卫生间'),(-2450,-1650,'保洁'),(-1450,-1880,'会议'),(300,-1880,'茶水'),(2050,-1910,'IT机房'),(3870,50,'救援')]:text(x,y,t)
for x,y,t in [(0,1190,'行政走廊'),(-700,-1080,'后勤走廊'),(3500,-650,'机电'),(3500,-810,'通道')]:text(x,y,t,'small')
for x,y,t in [(2050,-1250,'保险丝'),(2500,1510,'配电恢复'),(2850,-1750,'500'),(500,1750,'500')]:
 a,b=pos(x,y);parts.append(f'<circle cx="{a}" cy="{b}" r="6" fill="#bd6412"/>');parts.append(f'<text x="{a+10}" y="{b-8}" class="small">{t}</text>')
parts+=['<text x="90" y="870" class="note" fill="#c47514">实线：主线去程 / 东侧回路 / 撤离</text>','<text x="90" y="906" class="note">绿色虚线：取得保险丝后解锁的西侧返程；紫色虚线：可选档案探索。</text>','<text x="90" y="942" class="note">设计重点：去茶水间不穿机房；会议不占主通道；搜索改变返程路径。</text>','<text x="90" y="978" class="small">验证边界：连续胶囊扫掠 + 定点交互回归；路线偏好、迷路率和战斗节奏仍需首次真人试玩。</text>','<text x="1240" y="168" class="room">N ↑</text>']
parts.append('</svg>')
(root/'Docs/Portfolio/LevelPlan.svg').write_text(''.join(parts),encoding='utf-8')
print('Wrote Docs/Portfolio/LevelPlan.svg from',len(rows),'runtime wall/door records')

# Optional raster companion for artifact viewers; the SVG remains the source.
try:
    from PIL import Image, ImageDraw, ImageFont
    import xml.etree.ElementTree as ET
    im=Image.new('RGB',(1440,1060),'#f5f3ee'); draw=ImageDraw.Draw(im)
    for el in ET.fromstring(''.join(parts)):
        tag=el.tag.split('}')[-1]; a=el.attrib
        if tag=='rect':
            x,y,w,h=[float(a.get(k,0)) for k in ['x','y','width','height']]
            draw.rectangle((x,y,x+w,y+h),fill=a.get('fill'),outline=None if a.get('stroke','none')=='none' else a['stroke'])
        elif tag=='polyline':
            pts=[tuple(map(float,v.split(','))) for v in a['points'].split()]
            if a.get('stroke-dasharray','none')=='none':
                draw.line(pts,fill=a.get('stroke'),width=int(a.get('stroke-width',4)))
            else:
                import math
                for (x1,y1),(x2,y2) in zip(pts,pts[1:]):
                    distance=math.hypot(x2-x1,y2-y1)
                    for d in range(0,int(distance),17):
                        e=min(d+10,distance)
                        draw.line((x1+(x2-x1)*d/distance,y1+(y2-y1)*d/distance,x1+(x2-x1)*e/distance,y1+(y2-y1)*e/distance),fill=a['stroke'],width=4)
        elif tag=='circle':
            x,y,r=[float(a[k]) for k in ['cx','cy','r']];draw.ellipse((x-r,y-r,x+r,y+r),fill=a.get('fill'))
        elif tag=='text':
            size=int(a.get('font-size',14 if a.get('class')=='small' else 17))
            font=ImageFont.truetype('C:/Windows/Fonts/msyh.ttc',size)
            draw.text((float(a['x']),float(a['y'])),el.text or '',font=font,fill=a.get('fill','#253446'),anchor='ms' if a.get('text-anchor')=='middle' else 'ls')
    im.save(root/'Docs/Portfolio/LevelPlan.png')
except ImportError:
    pass
