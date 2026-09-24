"""Editable master: ReportLab PDF + selectable SVG/HTML; no game-image retouching.
Run with Python (reportlab, Pillow). Uses installed Microsoft YaHei fonts.
Source assets and CSV are relative to this file; does not require Unreal.
"""
from pathlib import Path
import csv, html, json, math, shutil, re
from PIL import Image
from reportlab.pdfgen import canvas
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.lib.colors import HexColor

ROOT=Path(__file__).resolve().parent
OUT=ROOT.parent
A=ROOT/'assets'
W,H=960,675
INK='#223945'; MUTED='#526771'; BG='#F4F3EE'; WHITE='#FFFFFF'
TEAL='#157E7C'; GOLD='#AB712B'; RED='#B45342'; PURPLE='#7958A0'; PALE='#E0E9E6'
pdfmetrics.registerFont(TTFont('CN','C:/Windows/Fonts/msyh.ttc',subfontIndex=0))
pdfmetrics.registerFont(TTFont('CNB','C:/Windows/Fonts/msyhbd.ttc',subfontIndex=0))
PDF=OUT/'Portfolio-V3.pdf'
c=canvas.Canvas(str(PDF),pagesize=(W,H),pageCompression=1)
c.setTitle('林顺｜封锁区：零号大厦｜关卡设计与玩法机制作品集')
c.setAuthor('林顺');c.setSubject('两关可玩原型 · 招聘补充材料 · 2026.09.25')
pages=[]; svg=[]; texts=[]; page_no=0
def esc(t):return html.escape(str(t),quote=True)
def box(x,y,w,h,fill=WHITE,stroke=None,sw=1):
 c.setFillColor(HexColor(fill));c.setStrokeColor(HexColor(stroke or fill));c.setLineWidth(sw)
 c.rect(x,H-y-h,w,h,fill=1,stroke=bool(stroke))
 svg.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" fill="{fill}" stroke="{stroke or "none"}" stroke-width="{sw}"/>')
def line(points,color=TEAL,width=2,dash=False):
 c.setStrokeColor(HexColor(color));c.setLineWidth(width);c.setDash([5,4] if dash else [])
 p=c.beginPath();p.moveTo(points[0][0],H-points[0][1])
 for x,y in points[1:]:p.lineTo(x,H-y)
 c.drawPath(p);c.setDash([])
 svg.append(f'<polyline points="{" ".join(f"{x},{y}" for x,y in points)}" fill="none" stroke="{color}" stroke-width="{width}" {"stroke-dasharray=\"5 4\"" if dash else ""}/>')
def arrow(points,color=TEAL,width=2,dash=False):
 line(points,color,width,dash);a,b=points[-2:];ang=math.atan2(b[1]-a[1],b[0]-a[0]);l=7
 line([(b[0]-l*math.cos(ang-.45),b[1]-l*math.sin(ang-.45)),b,(b[0]-l*math.cos(ang+.45),b[1]-l*math.sin(ang+.45))],color,width)
def dot(x,y,r=4,color=TEAL):
 c.setFillColor(HexColor(color));c.circle(x,H-y,r,fill=1,stroke=0)
 svg.append(f'<circle cx="{x}" cy="{y}" r="{r}" fill="{color}"/>')
def text(x,y,t,size=14,w=None,color=INK,bold=False,leading=None):
 font='CNB' if bold else 'CN';leading=leading or size*1.55
 rows=[]
 for para in str(t).split('\n'):
  if not w: rows.append(para);continue
  s=''
  for ch in re.findall(r'[A-Za-z0-9]+(?:[./_-][A-Za-z0-9]+)*|.',para):
   if s and pdfmetrics.stringWidth(s+ch,font,size)>w:
    if ch in '，。；：！？、）》”％' and len(s)>1:
     rows.append(s[:-1]);s=s[-1]+ch
    else:rows.append(s.rstrip());s=ch.lstrip()
   else:s+=ch
  rows.append(s)
 for i,s in enumerate(rows):
  yy=y+i*leading
  assert yy+size<H-13,(page_no,t,yy)
  c.setFont(font,size);c.setFillColor(HexColor(color));c.drawString(x,H-yy-size*.92,s)
  svg.append(f'<text x="{x}" y="{yy+size*.92}" font-family="Microsoft YaHei, sans-serif" font-size="{size}" fill="{color}" font-weight="{700 if bold else 400}">{esc(s)}</text>')
 texts.append(str(t));return y+len(rows)*leading
def image(name,x,y,w,h=None):
 p=A/name;im=Image.open(p);iw,ih=im.size
 h=h or w*ih/iw
 # Preserve original aspect ratio; no color or illumination editing.
 k=min(w/iw,h/ih);dw,dh=iw*k,ih*k;xx=x+(w-dw)/2;yy=y+(h-dh)/2
 c.drawImage(str(p),xx,H-yy-dh,dw,dh)
 svg.append(f'<image href="assets/{esc(name)}" x="{xx}" y="{yy}" width="{dw}" height="{dh}"/>')
 return y+h
def link(x,y,label,url,size=12):
 text(x,y,label,size,color=TEAL)
 ww=pdfmetrics.stringWidth(label,'CN',size)
 c.linkURL(url,(x,H-y-size*1.3,x+ww,H-y),relative=0,thickness=0)
 svg.append(f'<a href="{esc(url)}"><rect x="{x}" y="{y}" width="{ww}" height="{size*1.4}" fill="transparent"/></a>')
def small(x,y,t,w=880):return text(x,y,t,10.5,w,MUTED,leading=15)
def title(kicker,heading,sub):
 global svg,texts,page_no
 page_no+=1;svg=[];texts=[];box(0,0,W,H,BG);box(38,32,28,4,TEAL)
 text(77,25,kicker,11,color=TEAL,bold=True)
 text(38,56,heading,29,bold=True)
 text(38,100,sub,13,w=885,color=MUTED)
 c.bookmarkPage(f'p{page_no}');c.addOutlineEntry(heading,f'p{page_no}',level=0)
def finish(evidence=''):
 line([(38,627),(922,627)],'#C8D1D0',.7)
 small(38,638,'林顺  /  封锁区：零号大厦  /  关卡设计与玩法机制')
 text(886,636,f'{page_no:02d} / 10',10,color=MUTED)
 if evidence:small(38,603,evidence)
 body='\n'.join(svg)
 doc=f'<svg xmlns="http://www.w3.org/2000/svg" width="960" height="675" viewBox="0 0 960 675">{body}</svg>'
 (ROOT/f'page-{page_no:02d}.svg').write_text(doc,encoding='utf-8')
 pages.append((body,list(texts)));c.showPage()
def card(x,y,w,heading,body,color=TEAL):
 box(x,y,w,126,WHITE);box(x,y,3,126,color)
 text(x+15,y+13,heading,16,bold=True,color=color)
 text(x+15,y+43,body,13,w=w-30,leading=20)
def node(x,y,w,head,body,color=TEAL):
 box(x,y,w,90,WHITE);box(x,y,w,3,color)
 text(x+11,y+13,head,15,bold=True,color=color)
 text(x+11,y+42,body,12,w=w-22,leading=18)


# 01


title('PORTFOLIO / 01','封锁区：零号大厦','林顺  ·  关卡设计与玩法机制作品集  ·  招聘补充材料 V3 / 2026.09.25')
image('opening.jpg',38,142,560,315)
small(38,467,'第一关出生实机：透过观察玻璃建立东侧电梯目标。',560)
text(626,146,'把逃生目标放在眼前，\n把通路选择交给玩家。',22,w=297,bold=True,leading=33)
text(626,230,'感染暴发后被封锁的医院后勤行政楼。设施维护人员从维修值班室醒来，利用撬棍与声音诱导恢复电梯，再驾驶维修皮卡离开。',14,w=292,leading=22)
text(626,348,'两关可玩原型',17,bold=True,color=TEAL)
text(626,381,'Windows / UE 5.8.2\n第一关：双路线、回访与电梯收尾\n第二关：自由驾驶灰盒，迭代中',13,w=294,leading=22)
box(38,511,884,79,PALE)
text(52,521,'本人负责',14,bold=True,color=TEAL)
text(158,521,'关卡布局、动线与引导、玩法规则、节奏和迭代决策；代码部分使用 AI 辅助实现。',12.5,w=746)
text(52,550,'玩法证据',14,bold=True,color=TEAL)
text(158,550,'P2-6：路线、实机观察与声音规则；补充影像 ElevatorFinale.mp4（连续片段，无声）。',12.5,w=746)
finish('重点阅读：双路线的风险交换（P2-3）→ 风道收束（P5）→ 声音规则与关卡节奏（P6-8）。')
# 03 - Source-aligned schematic; furniture omitted, never called an in-engine top view.
title('LEVEL 01 / 02','两条路线，交换的是临场风险与操作成本','平面示意按当前墙体 CSV 与交互坐标重绘；上北下南，面向电梯时左侧为北。非实机俯视图。')
mx,my,k=40,150,.071
def P(x,y):return mx+(x+3850)*k,my+(y+2150)*k
def room(x,y,w,h,col):
 a,b=P(x-w/2,y-h/2);box(a,b,w*k,h*k,col)
room(0,0,7600,4200,'#E6EBED')
for x,y,w,h,col in [(-3150,0,1260,1460,'#D6E5D6'),(-2900,1500,1700,1100,'#E1E7EB'),(-1125,1700,890,640,'#E1DBEA'),(-450,1700,340,640,'#E1DBEA'),(675,1700,1790,640,'#DDDCE5'),(2975,1500,1490,1100,'#EBDFC8'),(-3225,-1625,990,790,PALE),(-2450,-1625,440,790,PALE),(-1425,-1625,1490,790,PALE),(250,-1625,1740,790,PALE),(2150,-1625,1940,790,'#EBDFC8')]:room(x,y,w,h,col)
for r in csv.DictReader((A/'LZOfficeLayout.csv').open(encoding='utf-8-sig')):
 if r['kind']=='wall':room(*[float(r[q]) for q in ['x','y','width','depth']],INK)
 if r['kind']=='window':room(float(r['x']),float(r['y']),float(r['width']),18,'#55B9CE')
for z in [(0,2100,7600,90),(0,-2100,7600,90),(-3800,0,90,4200),(3800,1250,90,1700),(3800,-1250,90,1700),(-3150,750,1300,80),(-3150,-750,1300,80),(-2520,-400,40,190),(-2520,652,40,216)]:room(*z,INK)
room(-2520,120,30,810,'#55B9CE');room(3850,0,700,570,'#D3DFE0')
def label(x,y,s,sz=10,col=INK):
 xx,yy=P(x,y);text(xx,yy,s,sz,color=col)
for x,y,s in [(-3690,-500,'01 维修值班室'),(-3690,1600,'02 清空区'),(-3500,-1930,'08 卫生间'),(-2650,-1930,'09'),(-2000,-1930,'10 会议室'),(-330,-1930,'11 黑暗茶水间'),(1470,-1930,'12 IT 机房'),(-1550,1730,'04 主管室'),(-540,1920,'05'),(380,1830,'06 档案室'),(2400,1850,'07 配电间'),(-1950,-280,'03 中央办公区'),(3520,-160,'电梯'),(-500,1100,'15 行政走廊')]:label(x,y,s)
for x,y,w,h,s,col in [(300,-750,480,40,'A',TEAL),(300,900,480,40,'B',RED),(3380,900,240,40,'C',GOLD),(2200,1150,40,240,'D',GOLD),(-2520,-610,40,230,'出口',TEAL)]:
 room(x,y,w,h,col)
 lx,ly={'A':(235,-1000),'B':(-110,670),'C':(3470,675),'D':(2320,1020),'出口':(-2160,-550)}[s]
 label(lx,ly,s,11,col)
# Planned traversals drawn through real openings; paths are illustrative, not scripted.
arrow([P(700,350),P(650,650),P(300,820),P(300,1170),P(2070,1170),P(2450,1170),P(2900,1700)],TEAL,2)
arrow([P(-1950,-1350),P(-1150,-1350),P(-1150,-1650),P(800,-1650),P(800,1550),P(200,1550),P(200,1850)],PURPLE,2,True)
arrow([P(2950,1640),P(3380,1500),P(3380,700),P(3380,120)],GOLD,2)
for x,y,col in [(-3300,0,TEAL),(-2700,-350,GOLD),(-1950,-1350,PURPLE),(1020,1560,GOLD),(2900,1720,GOLD)]:dot(*P(x,y),3.5,col)
xx,yy=P(747,635);box(xx-3,yy-3,6,6,GOLD);label(875,535,'门卡',9,GOLD)
label(-180,1510,'D04',9,TEAL)
label(-3570,130,'出生 →',10,TEAL);label(-1890,-1200,'推柜入管',9,PURPLE)
label(1130,1500,'手电',9,GOLD);label(2700,1210,'保险丝',9,GOLD)
label(-920,410,'推荐投瓶落点',9,TEAL);dot(*P(-600,0),4,TEAL)
arrow([P(650,0),P(-600,0)],TEAL,1.3,True)
for x,y in [(-40,620),(180,680),(400,580)]:dot(*P(x,y),3,RED)
xx,yy=P(650,-20);dot(xx,yy,7,WHITE);text(xx-3,yy-7,'①',12,color=TEAL,bold=True)
text(572,132,'N ↑',12,bold=True,color=TEAL)
line([(45,479),(116,479)],INK,2);small(45,485,'10 m · 主体约 76 × 42 m')
small(266,479,'实线：地面 / 金色：C 回程捷径\n紫色虚线：高位风道（非地面通路）',348)
text(650,149,'B 门 · 短路径，现场风险',17,bold=True,color=TEAL)
text(650,180,'从观察点①确认门口威胁，再投瓶引开、靠近取卡、刷门。\n把风险集中在一段可观察的接近过程，考验落点与行动时机。',14,w=272,leading=23)
text(650,295,'通风 · 绕行，操作成本',17,bold=True,color=PURPLE)
text(650,326,'经 A 到 10 会议室，推柜攀入风道，从 06 离开。\n用绕行和操作成本换取避开 B 门聚集区；到行政区后仍需走向配电间。',14,w=272,leading=23)
box(38,535,884,49,PALE)
text(52,544,'汇合后：D 门进 07 取保险丝 → 从内侧解锁 C → 回电梯；C 外侧不能解锁。',14,w=855)
finish('① 对应下一页 B 门实机视角；家具省略。地面玩家可从 D04 进 06，两条路线均经 D 前往配电间。')


# 03
title('PLAYER VIEW / 03','门在眼前，解法需要先观察','把“看见目标”和“能够通过”分开：先留出观察距离，再让玩家选择何时接近门口。')
image('b_infected.jpg',38,145,628,353.25)
# All callouts point at visible pixels; no invented view through walls.
def pin(x,y,n,col=TEAL):
 dot(x,y,10,WHITE);text(x-4,y-8,str(n),12,color=col,bold=True)
line([(405,297),(369,314)],RED,1.2);pin(413,291,1,RED)
line([(145,390),(183,378)],GOLD,1.2);pin(135,395,2,GOLD)
line([(219,292),(255,317)],TEAL,1.2);pin(210,285,3,TEAL)
text(692,147,'观察点 ①',17,bold=True,color=TEAL)
text(692,180,'1  三只感染者聚在门前。\n2  深蓝制服尸体在左侧；门卡需靠近取得。\n3  绿色读卡器贴在门旁。',13,w=226,leading=22)
text(692,286,'先改变占位，再接近',16,bold=True,color=TEAL)
text(692,319,'投瓶区域不在这张画面的视野内。转向西北工位投掷，使敌人离开入口，再从尸体一侧靠近读卡器。',13,w=226,leading=21)
small(692,419,'进入后沿行政走廊向东，经 D 到配电间；回程从 C 接回电梯一侧。详见 P2。',226)
text(38,515,'信息怎样转成行动',17,bold=True)
text(38,548,'尸体提供取卡目标，读卡器说明卡的用途，感染者占位制造时机问题。取卡无需背包界面，减少近敌操作层级；仍需承担脚步、接近与投掷失误的风险。',14,w=884,leading=22)
finish('标注对应实际可见物体；卡片细节在此距离不清晰。尸体与读卡器能否被首次玩家主动关联，仍是重点试玩问题。')
# 04
title('TEACHING / 04','先留下物资线索，再让回访兑现','撬棍承担主线开路，手电服务可选探索；避免把所有奖励都串成离开楼层的必经任务。')
image('pantry_entry.jpg',38,143,431,242.44)
image('pantry_lit_crop.jpg',491,143,431,242.44)
line([(375,170),(405,190)],GOLD,1.2);pin(367,165,1,GOLD)
line([(188,286),(239,286)],TEAL,1.2);pin(178,286,2,TEAL)
line([(855,230),(798,257)],GOLD,1.2);pin(865,226,3,GOLD)
text(38,399,'初见 11：知道里面有东西',17,bold=True,color=TEAL)
text(38,433,'急救标识①、门口血迹与深处制服②建立物资预期。奖励放在更深处，入口不直接展示枪和急救包。',14,w=431,leading=22)
text(491,399,'06 得到手电后：返回确认',17,bold=True,color=TEAL)
text(491,433,'同一茶水间深处，手电照见枪与急救包③。回访把先前的信息转为资源收益；不取这些奖励也可完成保险丝主线。',14,w=431,leading=22)
box(38,516,884,76,WHITE)
text(52,528,'当前取舍与问题',15,bold=True,color=GOLD)
text(223,528,'两件奖励必须“持有且开启手电”才能拾取，是明确的权限条件，不能仅用黑暗解释。若玩家先辨认出物品却无法拿取，反馈可能不合直觉；这一点保留为待优化问题。',13,w=681,leading=20)
finish('左图为入口，右图裁切放大深处奖励，拍摄位置不同；未修改亮度，不是同机位提亮对比。')
# 05
title('ITERATION / 05','风道从“覆盖全层”收束为一条绕行','取舍：让风道解决 B 门的局部阻挡，同时保留进入行政区后的地面探索。')
def vent_schema(x,y,old=False):
 box(x,y,430,208,WHITE)
 for i,name in enumerate(['08-09','10 会议室','11-12']):
  box(x+15+i*137,y+15,126,42,PALE);text(x+27+i*137,y+28,name,12)
 for i,name in enumerate(['02 / 04','06 档案室','07 配电间']):
  box(x+15+i*137,y+146,126,42,'#E4DFE9');text(x+26+i*137,y+158,name,12)
 text(x+126,y+89,'03 中央办公区',13,color=MUTED)
 if old:
  line([(x+40,y+65),(x+384,y+65),(x+384,y+130),(x+40,y+130)],PURPLE,3,True)
  for xx in [x+66,x+205,x+341]:
   line([(xx,y+50),(xx,y+65)],PURPLE,2,True);dot(xx,y+50,4,PURPLE)
   line([(xx,y+130),(xx,y+146)],PURPLE,2,True);dot(xx,y+146,4,PURPLE)
 else:
  arrow([(x+210,y+50),(x+275,y+50),(x+275,y+120),(x+210,y+120),(x+210,y+142)],PURPLE,3,True)
  dot(x+210,y+50,5,PURPLE);dot(x+210,y+146,5,PURPLE)
vent_schema(38,144,True);vent_schema(492,144,False)
text(38,366,'原方案：各区均有风道出入口',17,bold=True,color=MUTED)
text(38,400,'沿全层分布的高位网络，几乎处处可下。它支持广泛绕行，也让安全通路覆盖了多个地面问题。',14,w=430,leading=22)
text(492,366,'当前：10 进入，06 离开',17,bold=True,color=PURPLE)
text(492,400,'保留推柜、攀爬和风道体验，删除其余出入口。玩家绕开 B 门后落到行政区，仍需从 D 去配电间、经 C 返回。',14,w=430,leading=22)
line([(38,480),(922,480)],'#C8D1D0',1)
text(38,497,'为什么不保留全网？',16,bold=True)
text(38,529,'全网提供更多落点，却削弱了两条路线的差异；收束后可选择的落点更少，换来更明确的风险交换与汇合位置。',13,w=430,leading=21)
text(492,497,'付出的代价与下一步',16,bold=True)
text(492,529,'牺牲自由穿行与立体探索范围。现有通行检查支持 10→06；是否因绕行过长而被玩家放弃，尚需与 B 门一起试玩。',13,w=430,leading=21)
finish('两图均为关系示意，非旧实机或等比地图。前后入口方案有迭代记录；路线偏好未做玩家对比测试。')
# 06
title('MECHANICS / 06','诱导窗口由几条同时运行的计时构成','瓶子改写敌人的调查位置；刷门的安全窗口从门全开才开始，不应把所有秒数顺序相加。')
heads=[('瓶子落地','声音被有效接收'),('响应并离门','转向后走向落点'),('接近并取卡','取决于玩家行动'),('刷卡 / 进门','门从关闭到全开'),('门关闭','满足时间与占用条件')]
for i,(h,b) in enumerate(heads):
 node(38+i*179,144,166,h,b,TEAL if i!=2 else GOLD)
 if i<4:arrow([(205+i*179,185),(214+i*179,185)],TEAL)
small(38,245,'横轴仅表示事件顺序，不是等比时轴；离门、接近和进门的耗时随落点、距离、导航与操作变化。')
for x,txt in [(38,'开始计时的事件'),(281,'当前参数'),(450,'设计作用与边界')]:text(x,278,txt,13,bold=True,color=MUTED)
rows=[('新声源触发警觉','转向约 0.65 s','把“听见”与移动分开，让玩家能辨认反应。'),('接收到瓶子等强声','调查承诺 12 s','期间弱电台不覆盖目标；并非 12 秒无敌或保证通路畅通。'),('门已完全打开','保持 7 s','覆盖刷门后的穿越，不包含此前取卡时间；有人占门则继续开着。'),('进入声源附近搜索','B 群搜索 8 s','没有新线索才结束；电台按自身 14 s 周期呼叫，可能再次吸引。')]
for i,(a,b,d) in enumerate(rows):
 y=304+i*55;box(38,y,884,50,WHITE);text(50,y+11,a,12.5,w=221);text(281,y+11,b,12.5,w=154,bold=True,color=TEAL);text(450,y+9,d,12.5,w=456,leading=18)
text(38,542,'关键关系',15,bold=True,color=TEAL)
text(159,542,'12 秒承诺与敌人移动、玩家取卡同时进行。弱呼叫不能立刻拉回它们；承诺结束也不强制回门。7 秒保持到期且门口清空至少 1.2 秒，才开始闭合。',13,w=752,leading=20)
finish('站立、转头不发脚步声；近身接触仍有危险。投瓶失误允许重试，关闭电台只停止后续发声，不清空搜索记忆。')
# 07
title('CLIMAX / 07','把撬门留作收尾，而非背身战斗考验','交互前允许自由应对，主动开始后转入受保护的交互式逃生演出。')
image('bench.jpg',38,144,279,157)
image('pry.jpg',338,144,279,157)
image('garage.jpg',638,144,284,160)
text(38,319,'开场：先认出工具的用途',17,bold=True,color=TEAL)
text(38,353,'出口卡住，附近维修台提供撬棍。允许先拿工具再检查门；成功撬开后，金属声引来教学感染者。',14,w=279,leading=22)
text(338,319,'结尾：工具成为逃生手段',17,bold=True,color=TEAL)
text(338,353,'电梯卡门声带来威胁。玩家决定开始时机；按住推进门开度，完成后自动入厢回身、夹手关门。',14,w=279,leading=22)
text(638,319,'闭门：短暂安全后重建目标',17,bold=True,color=TEAL)
text(638,353,'保留玩家、摄像机与轿厢，闭门下行阶段准备并生成车库。再开门时看到新的车辆与出口关系。',14,w=284,leading=22)
box(38,470,884,115,WHITE)
text(53,483,'从可中断交互到受控收尾',16,bold=True,color=GOLD)
text(53,516,'面朝门持续操作时，很难同时判断背后威胁。保留自由战斗能增加临场变化，却会与发力操作争夺注意力；当前选择在开始后隔离普通敌人伤害，牺牲这段战术自由，换取明确的关卡结束。',13,w=850,leading=21)
finish('回收的是撬棍用途与动作认知，前后输入方式不同。约 20 秒电梯连续附件为无声实录；夹手仍是几何占位。')
# 08
title('INTERACTION / 08','用状态边界管理输入、保护与交接','一处启动，一条连续流程；松开只暂停发力，不退出演出。')
for i,(h,b) in enumerate([('呼梯 / 运行','按钮仅负责呼梯'),('停稳卡门','中央门缝可交互'),('发力','按住推进，松开暂停'),('自动入厢','角色与镜头一起移动'),('关门 / 下行','全闭后下行交接')]):
 node(38+i*179,144,166,h,b,TEAL if i<2 else GOLD)
 if i<4:arrow([(205+i*179,185),(214+i*179,185)],GOLD)
text(38,268,'阶段',13,bold=True,color=MUTED);text(238,268,'玩家能做什么',13,bold=True,color=MUTED);text(598,268,'必须守住的边界',13,bold=True,color=MUTED)
rows=[('门缝启动前','自由移动、观察和应对威胁。','检查距离、朝向、遮挡与工具；按钮不触发 QTE。'),('对齐与有效发力','约 3.6 秒有效按住；松开保留进度。\n暂停仍可用，其余世界操作锁定。','不接受重复启动；普通敌人不伤害或推挤玩家。'),('入厢、关门与下行','自动跨过门槛并回身，继续受保护。','角色完整进入才关门；门全闭才下降；车库未准备好则闭门等待。'),('第二关开门 / 异常恢复','开门完成恢复自由操作；可显式重试。','统一清理控制锁、保护与临时对象，避免重试遗留状态。')]
for i,(a,b,d) in enumerate(rows):
 y=297+i*69;box(38,y,884,62,WHITE);text(51,y+10,a,14,w=173,bold=True);text(238,y+8,b,13,w=329,leading=20);text(598,y+8,d,13,w=308,leading=20)
finish('电梯专项 14 项检查通过，含暂停、重复交互、伤害保护与中途重试；首次玩家能否理解发力提示仍待验证。')
# 07 - Garage geometry updated, no obsolete 4.5 s reveal on diagram.
title('LEVEL 02 / 09','先允许挪车，再制造返程压力','可玩灰盒 / 迭代中：允许先试驾，也可直接开闸；提前挪车是策略，开闸不会复位车辆。')
ox,oy,sk=39,157,.081
def G(x,y):return ox+x*sk,oy+(y+1600)*sk
def gr(x,y,w,h,col,st=None):
 a,b=G(x-w/2,y-h/2);box(a,b,w*sk,h*sk,col,st)
gr(2400,0,4800,3200,'#E1E8E8',INK)
gr(2500,-1280,1000,640,'#EAD7CF',INK)
gr(320,1320,600,540,'#D5E6D6',INK)
gr(5050,120,500,560,'#ECE0C6',INK)
for xx in (1850,3050):
 gr(xx,100,500,600,'#C8D5D0',GOLD);gr(xx-110,110,185,430,'#8FA1A6');gr(xx+155,80,90,90,INK)
for xx in (320,860,1400):gr(xx,-1290,185,430,'#A4B2B7')
for xx in (1300,2010,2720,3430):gr(xx,1430,430,185,'#A4B2B7')
gr(5500,850,1400,800,'#E6DBC0');gr(4790,850,30,800,GOLD)
gr(5050,400,500,20,'#55B9CE');gr(4800,290,20,220,'#55B9CE')
angle=math.radians(-55)
def carlocal(x,y):return G(960+x*math.cos(angle)-y*math.sin(angle),510+x*math.sin(angle)+y*math.cos(angle))
poly=[carlocal(x,y) for x,y in [(-235,-102),(235,-102),(235,102),(-235,102),(-235,-102)]]
line(poly,TEAL,8);line([carlocal(86,-105),carlocal(-27,-218)],GOLD,3)
for x,y,s in [(2050,-1510,'Boss 隔间'),(10,1270,'电梯'),(430,850,'维修皮卡'),(4000,-400,'玻璃岗亭'),(4970,740,'坡道'),(2100,30,'7 m'),(3430,-100,'回转区')]:
 xx,yy=G(x,y);text(xx,yy,s,10,bold=True)
arrow([G(960,210),G(1400,-680),G(3500,-680),G(4250,100),G(4270,970),G(3350,1080),G(1180,1080)],TEAL,1.8,True)
arrow([G(2500,-950),G(3580,-620),G(4590,700)],RED,2,True)
small(39,430,'主体 48 × 32 m；中间通道 7 m；坡道宽 8 m、实长 24 m。',520)
small(39,453,'按生成坐标示意；坡道长度在图中压缩。虚线为可行路线示意。',525)
image('truck.jpg',594,147,328,184.5)
small(594,338,'实机：斜停皮卡、半开驾驶门与车内灯。',328)
text(594,373,'被打断的撤离，仍可继续',18,bold=True,color=TEAL)
text(594,407,'出口门缝透光却无法通行，旁边玻璃岗亭可见控制台。撬门、按一次按钮恢复开闸；噪声引 Boss 去出口，玩家回到车辆实际位置，撞倒或绕过后撤离。',14,w=328,leading=22)
box(38,527,884,58,WHITE)
text(52,539,'双岛同时容纳两种移动方式：步行时绕柱避冲撞；驾驶时保留中央通道与外圈，允许调整再出库。',13,w=855)
text(52,561,'不强制回到原车位，也不把击杀 Boss 作为开门条件；代价是返程压力随玩家停车策略而变化。',12,color=MUTED)
finish('刚体自由驾驶原型，非轨道。双岛和坡道已有脚本驾驶记录；手感、首次引导与 Boss 压力仍待真人试玩。')


# 10
title('AUTHOR / 10','职责与作品边界','以空间组织、引导、风险交换和机制边界展示关卡策划能力；不将原型描述为正式成品。')
text(38,145,'设计贡献',18,bold=True,color=TEAL)
text(38,179,'关卡布局与南北动线；B 门 / 通风路线、工具教学、可选回访、C 门回路；电梯节奏、车库遭遇与环境引导的取舍和迭代。',14,w=418,leading=22)
text(38,280,'实现与资源',18,bold=True,color=TEAL)
text(38,313,'代码部分使用 Codex / Gemini 辅助实现。美术与动画复用 UE / GASP、Kenney CC0 及第三方资源；不作为个人独立编程、建模或动画制作成果。',14,w=418,leading=22)
text(493,145,'目前的证据',18,bold=True,color=TEAL)
text(493,179,'实机截图、电梯连续片段，以及开场、门禁、通风、驾驶和电梯的分阶段运行检查。规则与画面已有证据，声音核心玩法仍缺少可交付的有声演示。',14,w=429,leading=22)
text(493,280,'接下来优先验证',18,bold=True,color=GOLD)
text(493,313,'记录首次玩家是否发现门卡、是否选择风道、投瓶后何时行动；再检验手电拾取门槛、车库返程压力和驾驶手感。跨关卡卡顿与性能尚未测量。',14,w=429,leading=22)
box(38,427,884,62,PALE)
text(53,438,'林顺  |  悉尼大学 计算机科学硕士在读  |  浙大城市学院 环境设计学士',14,bold=True)
text(53,464,'电话 / 微信：19858188672',12.5);link(330,464,'邮箱：737401696@qq.com','mailto:737401696@qq.com',12.5)
small(38,504,'资源：Zombie Number 7 - Animated / Tony Flanagan，CC BY 4.0；项目有组合、尺度与动画接入调整。')
link(38,527,'原作品（Fab）','https://www.fab.com/listings/a5710c50-a98e-4b55-98f8-c0a0f8318a84');link(158,527,'许可 CC BY 4.0','https://creativecommons.org/licenses/by/4.0/')
small(38,550,'临时表现：程序驱动工具与电梯演出、占位手臂、组合车辆 / Boss 和 POI；场内部分旧标识仍待统一。')
small(38,575,'补充影像：ElevatorFinale.mp4（连续片段，无声）；核心玩法有声录像待补。')
finish()
c.save()

# Editable browser source: each SVG contains real text and vector diagrams.
sections='\n'.join(f'<section id="page-{i+1}"><svg viewBox="0 0 960 675" xmlns="http://www.w3.org/2000/svg">{body}</svg></section>' for i,(body,_) in enumerate(pages))
style='''body{margin:0;background:#d3d8d8;font-family:"Microsoft YaHei",sans-serif}section{width:960px;height:675px;margin:24px auto;box-shadow:0 2px 18px #0002;background:#f4f3ee}svg{width:100%;height:100%} @page{size:960px 675px;margin:0}@media print{body{background:white}section{margin:0;box-shadow:none;break-after:page}}'''
(ROOT/'作品集_可编辑.html').write_text(f'<!doctype html><html lang="zh-CN"><meta charset="utf-8"><title>林顺 · 封锁区作品集</title><style>{style}</style><body>{sections}</body></html>',encoding='utf-8')
(ROOT/'作品集正文.md').write_text('\n\n'.join(f'## 第 {i+1} 页\n\n'+'\n\n'.join(t) for i,(_,t) in enumerate(pages)),encoding='utf-8')
print(PDF)
