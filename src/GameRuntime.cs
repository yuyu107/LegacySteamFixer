using System;using System.Collections.Generic;
public enum GameRunState {Checking,Stopped,Starting,Running,Stopping,NotDetected}
// UI-thread state; process enumeration happens separately on a worker.
public class GameRuntime {
 List<GameProcess> snapshot=new List<GameProcess>();bool ready;
 Dictionary<string,DateTime> starts=new Dictionary<string,DateTime>(StringComparer.OrdinalIgnoreCase),stops=new Dictionary<string,DateTime>(StringComparer.OrdinalIgnoreCase);
 public void Observe(List<GameProcess> processes){snapshot=processes;ready=true;}
 public void Launch(string root,DateTime now){starts[root]=now;stops.Remove(root);}
 public void CancelLaunch(string root){starts.Remove(root);}
 public void Stopping(string root,DateTime now){stops[root]=now;}
 public List<GameProcess> Processes(string root){return snapshot.FindAll(p=>GameTools.Within(root,p.Path)&&!Auxiliary(p.Path));}
 static bool Auxiliary(string path){string name=System.IO.Path.GetFileNameWithoutExtension(path);return name.Equals("UnityCrashHandler32",StringComparison.OrdinalIgnoreCase)||name.Equals("UnityCrashHandler64",StringComparison.OrdinalIgnoreCase)||name.Equals("crashpad_handler",StringComparison.OrdinalIgnoreCase);}
 public GameRunState State(string root,DateTime now){DateTime time;bool running=Processes(root).Count>0;if(running){starts.Remove(root);if(stops.TryGetValue(root,out time)){if(now-time<TimeSpan.FromSeconds(10))return GameRunState.Stopping;stops.Remove(root);}return GameRunState.Running;}stops.Remove(root);if(starts.TryGetValue(root,out time))return now-time<TimeSpan.FromSeconds(45)?GameRunState.Starting:GameRunState.NotDetected;return ready?GameRunState.Stopped:GameRunState.Checking;}
}
