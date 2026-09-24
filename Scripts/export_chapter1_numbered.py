from pathlib import Path
import csv, html, math
from PIL import Image,ImageDraw,ImageFont
root=Path(__file__).resolve().parents[1]
out=root/'Docs/Portfolio';out.mkdir(exist_ok=True)
W,H=2800,2020
im=Image.new('RGB',(W,H),'#f7f6f2');d=ImageDraw.Draw(im)
svg=[f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}"><rect width="100%" height="100%" fill="#f7f6f2"/>']
ink='#253746';wall='#394955';teal='#187e80';red='#bd5140';purple='#7658a5'
def box(x,y,w,h,c,stroke=None):
 d.rectangle((x,y,x+w,y+h),fill=c,outline=stroke,width=2);svg.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" fill="{c}" stroke="{stroke or "none"}" stroke-width="2"/>')
def line(points,c=ink,width=3,dash=False):
 for a,b in zip(points,points[1:]):
  dist=math.dist(a,b)
  if dash:
   for st in range(0,int(dist),20):
    en=min(st+11,dist);d.line((a[0]+(b[0]-a[0])*st/dist,a[1]+(b[1]-a[1])*st/dist,a[0]+(b[0]-a[0])*en/dist,a[1]+(b[1]-a[1])*en/dist),fill=c,width=width)
  else:d.line((a,b),fill=c,width=width)
 svg.append(f'<polyline points="{" ".join(f"{x},{y}" for x,y in points)}" fill="none" stroke="{c}" stroke-width="{width}" stroke-dasharray="{"11 9" if dash else "none"}"/>')
def txt(x,y,t,size=25,c=ink,center=False):
 f=ImageFont.truetype('C:/Windows/Fonts/msyh.ttc',size)
 d.text((x,y),t,font=f,fill=c,anchor='mt' if center else 'lt')
 svg.append(f'<text x="{x}" y="{y+size}" text-anchor="{"middle" if center else "start"}" font-family="Microsoft YaHei,Arial" font-size="{size}" fill="{c}">{html.escape(t)}</text>')
def circ(x,y,r,c):
 d.ellipse((x-r,y-r,x+r,y+r),fill=c);svg.append(f'<circle cx="{x}" cy="{y}" r="{r}" fill="{c}"/>')
# North is player-left when facing +X toward the usable elevator: UE -Y.
def P(x,y):return 980+x*.23,750+y*.23
def rect(x,y,w,h,c,stroke=None):
 a,b=P(x-w/2,y-h/2);box(a,b,w*.23,h*.23,c,stroke)
def mark(x,y,t,c=teal):
 a,b=P(x,y);circ(a,b,18,c);txt(a,b-13,t,19,'#ffffff',True)
def label(x,y,t,size=25,c=ink):
 a,b=P(x,y);txt(a,b,t,size,c,True)
txt(100,65,'封锁区：零号大厦',46)
txt(100,132,'第一关 · 当前布局 / B 门听觉潜入与配电间捷径',33)
txt(100,192,'朝向基准：站在出生点面向可用电梯，左侧为北、右侧为南。上北下南、左西右东；A 安全绕行；B 刷卡；C 内侧解锁；D 普通门。',23)
rect(0,0,7600,4200,'#e8edf0',wall)
rooms=[('01','出生主办公室',-3150,0,1260,1460,'#dae6d9'),('02','原接待区 / 已清空',-2900,1500,1700,1100,'#dce7ef'),('03','开放办公区',400,0,5700,1740,'#e8edf0'),('04','主管办公室',-1125,1700,890,640,'#e3dfef'),('05','资料室',-450,1700,340,640,'#e3dfef'),('06','黑暗档案室',675,1700,1790,640,'#d1d4dc'),('07','配电间',2975,1500,1490,1100,'#eee0c5'),('08','卫生间',-3225,-1625,990,790,'#dbe7e3'),('09','保洁间',-2450,-1625,440,790,'#dbe7e3'),('10','会议室',-1425,-1625,1490,790,'#dbe7e3'),('11','黑暗茶水间',250,-1625,1740,790,'#dbe7e3'),('12','IT 机房',2150,-1625,1940,790,'#eee0c5')]
for num,name,x,y,w,h,c in rooms:rect(x,y,w,h,c)
# Outer shell, wake room, and lift enclosure from current source.
for r in [(0,2100,7600,100),(0,-2100,7600,100),(-3800,0,100,4200),(3800,1250,100,1700),(3800,-1250,100,1700),(-3150,750,1300,80),(-2510,825,60,210),(-3150,-750,1300,80),(-2520,652,40,216),(-2520,-400,40,190),(3500,-270,40,300),(3500,270,40,300),(3850,270,700,60),(3850,-270,700,60),(4200,0,60,600)]:rect(*r,wall)
rows=list(csv.DictReader((root/'Saved/LZOfficeLayout.csv').open(encoding='utf-8-sig')))
for r in rows:
 if r['kind']=='wall':rect(*[float(r[k]) for k in ['x','y','width','depth']],wall)
rect(-2520,120,30,810,'#57b7d3')
for r in rows:
 if r['kind']=='window':rect(float(r['x']),float(r['y']),float(r['width']),20,'#57b7d3')
rect(-3740,-1040,30,220,'#8c939a')
# All door openings; red are current physical gates, amber is a low vault obstacle.
doors=[('D01','出生门 / 斧破锁',-2520,-610,40,230,'lock'),('D02','主管室门洞',-1100,1350,200,60,'open'),('D03','资料室 / 撬锁或内开',-450,1350,200,60,'lock'),('D04','档案室普通门',500,1350,240,60,'open'),('D05','行政走廊西门洞',400,900,240,60,'open'),('D06','行政走廊东门洞',1900,900,240,60,'open'),('D07','配电间门洞',3000,900,240,60,'open'),('D08','后勤走廊入口',-2250,-900,240,60,'open'),('D09','卫生间门洞',-3200,-1200,200,60,'open'),('D10','保洁间门洞',-2450,-1200,200,60,'open'),('D11','会议室翻越口 / 高 0.95 m',-1850,-1200,200,60,'vault'),('D12','会议室东侧门洞',-950,-1200,240,60,'open'),('D13','茶水间门洞',300,-1200,240,60,'open'),('D14','机房南门洞',1850,-900,240,60,'open'),('D15','机房西侧门 / 当前实体阻挡',1150,-1100,60,240,'lock'),('D16','机房东门洞',3150,-1540,60,240,'open'),('D17','可用电梯门 / 供电 + QTE',3500,0,40,240,'lock'),('D18','新接待入口 / 绕经主管室',-1100,900,240,60,'open')]
# Superseded office portals are solid walls in the current layout.
doors=[row for row in doors if row[0] not in {'D05','D06','D07','D08','D11','D14','D15','D18'}]
doors.extend([('A','03 → 北侧后勤走廊',300,-750,480,60,'open'),('B','门禁卡 / 自动开关门',300,900,480,60,'lock'),('C','配电间捷径 / 仅内侧解锁',3380,900,240,60,'lock'),('D','配电间西侧普通门',2200,1150,60,240,'open')])
for num,name,x,y,w,h,state in doors:
 c=red if num in ['A','B'] or state=='lock' else '#b7852e' if state=='vault' else teal
 rect(x,y,w,h,c)
 a,b=P(x,y)
 dy=-37 if num in ['D01','D02','D03','D05','D06','D07','D08','D14','D18'] else 16
 dx=-35 if num=='D15' else -70 if num=='D17' else -55 if num in ['D16','D'] else 55 if num=='D01' else 0
 txt(a+dx,b+dy,num,20,c,True)
# Room labels, with the narrow records room identified by number only.
for num,name,x,y,w,h,c in rooms:
 if num=='01':label(-3150,-670,'01 出生主办公室',24)
 elif num in ['04','05']:label(x,1870,num+(' 主管室' if num=='04' else ''),22)
 elif num=='03':label(-600,-370,'03 开放办公区 / 中央主通道',30)
 else:label(x,-1890 if num in ['08','09','10','11','12'] else 1870 if num=='06' else 1920 if num=='07' else y+(350 if num=='02' else 0),num+' '+name,24 if num!='09' else 19)
label(100,1170,'15 行政走廊',22);label(-650,-980,'16 后勤走廊',21);label(3470,-1050,'13',24)
label(3860,-90,'14 电梯',23);label(3860,65,'→ B1',22)
# Spawn and gameplay points. Small point numbers have their own legend.
mark(-3300,0,'S','#317c45');label(-3320,-130,'出生 · 面向东侧电梯',20)
for x,y,n in [(-3280,-360,'1'),(-3090,445,'2'),(-3650,340,'3'),(420,1850,'4'),(2900,1720,'6'),(3470,230,'8')]:mark(x,y,n)
# A single overhead route connects meeting entry to archive exit.
line([P(-1950,-1350),P(-1150,-1350),P(-1150,-1650),P(800,-1650),P(800,1550),P(200,1550),P(200,1850)],purple,7,True)
rect(-1950,-1350,150,160,'#65a39d',teal)
label(-1740,-1570,'V1 唯一入口',18,purple)
label(-1775,-670,'双窗视线 → 10 会议室',18,teal)
a,b=P(200,1850);circ(a,b,8,'#b7852e')
label(200,1690,'V2 唯一出口 ↓',18,purple)
# Six infected gather on the office side of breach door B.
for x,y in [(-40,620),(180,680),(400,580)]:
 a,b=P(x,y);circ(a,b,10,red)
label(70,380,'B · 3 个感染者',18,red)
mark(747,635,'卡');label(1120,650,'保安 / 卡',17,teal)
mark(700,1240,'声');label(1320,1190,'可关对讲机',17,teal)
mark(650,-20,'观');label(1100,-70,'安全观察',17,teal)
mark(-600,0,'瓶');label(-1000,80,'建议落点',17,teal)
line([P(650,-20),P(-600,0)],teal,3,True)
line([P(850,100),P(850,625),P(590,725),P(350,810),P(350,1120)],teal,3,True)
line([P(3477,230),P(3380,230),P(3380,868)],'#b7852e',4)
# Main-map annotations and scale.
line([P(-2520,260),P(-2280,480)],'#57b7d3',2);label(-2130,560,'观察玻璃',20,teal)
# Blocked stair is the grey portal on the west wall.
line([(100,1280),(330,1280)],ink,5);line([(100,1272),(100,1288)],ink,3);line([(330,1272),(330,1288)],ink,3);txt(100,1300,'10 m  |  主体外壳约 76 × 42 m',23)
txt(1350,1300,'坐标对应：北 = −Y，东 = +X',23)
# Explicit cardinal compass and player-facing arrow.
line([(1960,370),(1960,285)],teal,4)
line([(1960,285),(1952,300)],teal,4);line([(1960,285),(1968,300)],teal,4)
line([(1915,325),(2005,325)],teal,3)
txt(1960,248,'北 N',25,teal,True);txt(1960,375,'南 S',22,teal,True)
txt(1910,316,'西',22,teal,True);txt(2020,316,'东',22,teal,True)
line([P(-3180,0),P(-2780,0)],'#317c45',4)
line([P(-2850,-45),P(-2780,0),P(-2850,45)],'#317c45',4)

# Right legend.
box(2050,250,650,1090,'#ffffff','#d8dee1')
txt(2090,282,'编号与当前功能',30)
legend=['01 出生主办公室：撬棍 → 钥匙 → 斧','02 原接待区：家具与装饰已清空','03 开放办公区：主通道 / 感染者','04 主管办公室：经 B 向西绕行','05 资料室：原检修口已封闭','06 档案室：风管出口 / 亮灯手电','07 配电间：保险丝 / C-D 回路','08 卫生间    09 保洁间','10 会议室：推柜入口    11 暗室补给','12 IT 机房：不再放保险丝','13 东侧机电通道','14 可用电梯：下至地下车库','15 行政走廊    16 后勤走廊']
for i,t in enumerate(legend):txt(2090,340+i*43,t,24)
txt(2090,940,'交互点（绿色圆点）',27)
for i,t in enumerate(['1 撬棍    2 钥匙抽屉    3 消防斧柜','4 亮着的可拾取手电筒','6 配电间保险丝（唯一供给）','8 冒火星电闸（在电梯旁）']):txt(2090,987+i*43,t,24)
txt(2090,1190,'蓝绿：门洞    红：A/B、门体或锁',23)
txt(2090,1232,'紫虚线：唯一风管路线    金点：出口',23)
txt(2090,1274,'地面 +5.55 m；净高 1.25 m；柔光灯缝',22)
# Door index, two columns.
txt(100,1405,'门与通路索引',30)
for i,(num,name,x,y,w,h,state) in enumerate(doors):
 col=i//9;row=i%9;txt(100+col*685,1462+row*48,num+'  '+name,24)
# Vent inset schematic, height references from native geometry.
box(1540,1410,1160,510,'#eeeaf4')
txt(1580,1440,'V1 会议室入口（高度以本层地面为 0）',28,purple)
line([(1580,1810),(2640,1810)],wall,6)
box(1610,1700,130,110,'#65a39d');line([(1740,1700),(1810,1700),(1810,1645),(1950,1645),(2100,1600),(2350,1600)],purple,7)
line([(1980,1520),(2410,1520),(2410,1600)],purple,7)
line([(2120,1560),(2320,1560),(2320,1780)],purple,4,True)
line([(2060,1810),(2060,1610)],wall,10)
txt(1585,1630,'柜顶 → 抓管口 → 蹲行爬坡',23)
txt(1920,1537,'蹲行',22,purple)
txt(2200,1640,'只下至 06，无法反向进入',23)
txt(1580,1838,'带脚轮柜 / 单轴推拉',23);txt(2300,1838,'10 → 06',23)
txt(1580,1880,'柜顶约 +1.90 m → 管口 +3.60 m → 干线 +5.55 m',24,purple)
txt(100,1950,'批注建议：直接写“移动 06 / 拆 D04 / 把 V1 接到 12”。本图展示当前实装，不代表已经完成布局优化。',25)
svg.append('</svg>')
(out/'Chapter1NumberedPlan.svg').write_text(''.join(svg),encoding='utf-8')
im.save(out/'Chapter1NumberedPlan.png')
print(out/'Chapter1NumberedPlan.png')
