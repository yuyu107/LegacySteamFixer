using System;using System.Drawing;using System.Drawing.Drawing2D;using System.Collections.Generic;using System.Windows.Forms;
// GDI+ outlines: independent of installed fonts, recolored for button states.
static class UiIcons {
 static Dictionary<string,Image> images=new Dictionary<string,Image>();
 static Icon app;
 public static Icon App {get{if(app==null){using(var stream=typeof(UiIcons).Assembly.GetManifestResourceStream("LegacySteamFixer.Icon")){if(stream!=null)using(var icon=new Icon(stream)){app=(Icon)icon.Clone();}}}return app;}}
 static bool Has(string text,params string[] terms){foreach(string term in terms)if(text.IndexOf(term,StringComparison.OrdinalIgnoreCase)>=0)return true;return false;}
 public static string Kind(string text){text=text??"";if(text=="—"||text=="□"||text=="×")return "none";
 if(Has(text,"automatic","自动检查","自動檢查"))return "clock";
 if(Has(text,"follow log","自动滚动","自動捲動"))return "download";
 if(Has(text,"force close","强制","強制"))return "force";
 if(Has(text,"uninstall","卸载","解除安裝"))return "trash";
 if(Has(text,"stop game","停止游戏","停止遊戲","stopping","停止中"))return "stop";
 if(Has(text,"starting","启动中","啟動中","checking","正在检查","正在檢查","detecting status","检测状态","偵測狀態"))return "clock";
 if(Has(text,"restart","重启","重新啟動"))return "refresh";
 if(Has(text,"download","下载","下載","export","导出","匯出"))return "download";
 if(Has(text,"update","更新"))return "update";
 if(Has(text,"folder","目录","目錄"))return "folder";
 if(Has(text,"restore","还原","還原"))return "restore";
 if(Has(text,"injection","注入"))return "inject";
 if(Has(text,"replacement","替换","替換","patch","修补","修補","enable","启用","啟用"))return "wrench";
 if(Has(text,"launch","启动游戏","啟動遊戲","Steam 启动","Steam 啟動"))return "play";
 if(Has(text,"refresh","刷新","重新整理","detect","重新检测","重新偵測"))return "refresh";
 if(Has(text,"diagnostic","诊断","診斷","log","日志","日誌"))return "log";
 if(Has(text,"more","更多"))return "more";
 if(Has(text,"cancel","取消","close","关闭","關閉"))return "close";
 if(Has(text,"later","稍后","稍後"))return "clock";
 if(Has(text,"test release","测试版","測試版"))return "flask";
 if(Has(text,"copy","复制","複製"))return "copy";
 if(Has(text,"save","保存","儲存"))return "save";
 return "check";}
 public static void Draw(Graphics g,string kind,RectangleF box,Color color){if(kind=="none")return;var state=g.Save();g.TranslateTransform(box.X,box.Y);g.ScaleTransform(box.Width/20,box.Height/20);g.SmoothingMode=SmoothingMode.AntiAlias;using(var p=new Pen(color,1.7f)){p.StartCap=p.EndCap=LineCap.Round;p.LineJoin=LineJoin.Round;using(var b=new SolidBrush(color)){
 switch(kind){
 case "play":g.FillPolygon(b,new[]{new PointF(6,3),new PointF(17,10),new PointF(6,17)});break;
 case "stop":g.FillRectangle(b,4,4,12,12);break;
 case "force":g.DrawPolygon(p,new[]{new PointF(7,2),new PointF(13,2),new PointF(18,7),new PointF(18,13),new PointF(13,18),new PointF(7,18),new PointF(2,13),new PointF(2,7)});g.DrawLine(p,7,7,13,13);g.DrawLine(p,13,7,7,13);break;
 case "close":g.DrawLine(p,5,5,15,15);g.DrawLine(p,15,5,5,15);break;
 case "clock":g.DrawEllipse(p,3,3,14,14);g.DrawLines(p,new[]{new PointF(10,6),new PointF(10,10),new PointF(13,12)});break;
 case "update":g.DrawLine(p,10,14,10,3);g.DrawLines(p,new[]{new PointF(6,7),new PointF(10,3),new PointF(14,7)});g.DrawLines(p,new[]{new PointF(3,13),new PointF(3,17),new PointF(17,17),new PointF(17,13)});break;
 case "download":g.DrawLine(p,10,3,10,13);g.DrawLines(p,new[]{new PointF(6,9),new PointF(10,13),new PointF(14,9)});g.DrawLines(p,new[]{new PointF(3,14),new PointF(3,17),new PointF(17,17),new PointF(17,14)});break;
 case "folder":g.DrawLines(p,new[]{new PointF(2,16),new PointF(2,4),new PointF(8,4),new PointF(10,7),new PointF(18,7),new PointF(16,16),new PointF(2,16),new PointF(5,9),new PointF(17,9)});break;
 case "refresh":case "restore":g.DrawArc(p,4,4,12,12,kind=="restore"?40:200,280);g.DrawLines(p,new[]{new PointF(2,4),new PointF(4,9),new PointF(8,6)});break;
 case "trash":g.DrawRectangle(p,5,6,10,11);g.DrawLine(p,3,4,17,4);g.DrawLine(p,8,2,12,2);g.DrawLine(p,8,9,8,14);g.DrawLine(p,12,9,12,14);break;
 case "log":g.DrawRectangle(p,4,2,12,16);for(int y=6;y<=14;y+=4)g.DrawLine(p,7,y,13,y);break;
 case "more":for(int x=4;x<=16;x+=6)g.FillEllipse(b,x-1,9,2,2);break;
 case "copy":g.DrawRectangle(p,7,6,10,12);g.DrawLines(p,new[]{new PointF(4,14),new PointF(3,14),new PointF(3,2),new PointF(13,2)});break;
 case "save":g.DrawRectangle(p,3,3,14,14);g.DrawRectangle(p,6,3,8,5);g.DrawRectangle(p,6,12,8,5);break;
 case "flask":g.DrawLines(p,new[]{new PointF(7,2),new PointF(13,2),new PointF(12,2),new PointF(12,8),new PointF(17,16),new PointF(16,18),new PointF(4,18),new PointF(3,16),new PointF(8,8),new PointF(8,2)});g.DrawLine(p,6,12,14,12);break;
 case "inject":g.DrawLine(p,4,16,14,6);g.DrawRectangle(p,11,3,6,6);g.DrawLine(p,3,17,6,17);break;
 case "wrench":g.DrawLines(p,new[]{new PointF(16,3),new PointF(12,7),new PointF(9,4),new PointF(13,1)});g.DrawArc(p,8,1,10,10,0,230);g.DrawLines(p,new[]{new PointF(9,10),new PointF(2,16),new PointF(4,18),new PointF(11,11)});break;
 default:g.DrawLines(p,new[]{new PointF(4,10),new PointF(8,14),new PointF(16,5)});break;
 }}}g.Restore(state);}
 public static Image ImageFor(string text,int size){string kind=Kind(text),key=kind+size;Image image;if(images.TryGetValue(key,out image))return image;var bitmap=new Bitmap(size,size);using(var g=Graphics.FromImage(bitmap))Draw(g,kind,new RectangleF(0,0,size,size),UiTheme.Blue);images[key]=bitmap;return bitmap;}
 public static void Options(Control root){foreach(Control control in root.Controls){var option=control as ButtonBase;if(option is CheckBox||option is RadioButton){option.Image=ImageFor(option.Text,Math.Max(16,option.Font.Height));option.TextImageRelation=TextImageRelation.ImageBeforeText;option.ImageAlign=ContentAlignment.MiddleLeft;option.TextAlign=ContentAlignment.MiddleLeft;}var choices=control as ComboBox;if(choices!=null){choices.DrawMode=DrawMode.OwnerDrawFixed;choices.ItemHeight=Math.Max(24,choices.Font.Height+8);choices.DrawItem+=(sender,e)=>{e.DrawBackground();if(e.Index>=0){int size=Math.Max(16,e.Font.Height);Draw(e.Graphics,"play",new RectangleF(e.Bounds.X+5,e.Bounds.Y+(e.Bounds.Height-size)/2,size,size),e.ForeColor);TextRenderer.DrawText(e.Graphics,choices.GetItemText(choices.Items[e.Index]),e.Font,new Rectangle(e.Bounds.X+size+12,e.Bounds.Y,Math.Max(1,e.Bounds.Width-size-14),e.Bounds.Height),e.ForeColor,TextFormatFlags.VerticalCenter|TextFormatFlags.EndEllipsis|TextFormatFlags.SingleLine);}e.DrawFocusRectangle();};}Options(control);}}
}
