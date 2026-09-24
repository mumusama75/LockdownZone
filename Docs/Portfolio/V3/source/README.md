# V3 可编辑源

build_portfolio.py 是排版主源（ReportLab、Pillow，Windows Microsoft YaHei 字体）。修改后运行即可生成 PDF、逐页 SVG、HTML 和正文 Markdown。图片只做等比例排版；茶水间奖励图作了记录在 image_sources.json 中的裁切，无生成式修饰。

作品集_可编辑.html 在浏览器中查看；page-*.svg 包含可编辑的中文文字、矢量路线和图片引用。编辑 SVG 后若要用 Python 重新生成，应同步回主源，避免覆盖手动修改。

所有 assets 均为本地相对引用。字体嵌入在 PDF 中，源文件需要本机安装微软雅黑才能维持相同排版；字体文件不随包分发。

主 PDF 为 10 页；配套唯一视频附件是根目录 ElevatorFinale.mp4（连续、无声）。有声核心演示暂缺，详见修改说明。没有改动 UE 玩法代码。
