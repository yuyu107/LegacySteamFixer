using System;using System.Windows.Forms;using System.Runtime.InteropServices;
// Keep selection and viewport intact while inspecting earlier log lines.
class DiagnosticsLogBox:TextBox {
 [DllImport("user32.dll")]static extern IntPtr SendMessage(IntPtr window,int message,IntPtr wParam,IntPtr lParam);
 public void AppendLog(string text,bool follow){
  if(follow){AppendText(text);SelectionStart=TextLength;SelectionLength=0;ScrollToCaret();return;}
  int start=SelectionStart,length=SelectionLength;
  int first=IsHandleCreated?SendMessage(Handle,0x00CE,IntPtr.Zero,IntPtr.Zero).ToInt32():0;
  AppendText(text);Select(start,length);
  if(IsHandleCreated){int current=SendMessage(Handle,0x00CE,IntPtr.Zero,IntPtr.Zero).ToInt32();SendMessage(Handle,0x00B6,IntPtr.Zero,new IntPtr(first-current));}
 }
}
