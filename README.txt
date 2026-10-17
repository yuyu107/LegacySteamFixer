# LegacySteamFixer

面向 Windows 7 SP1 / 8.1 等旧系统的 Steam 游戏兼容工具。当前版本：**0.4.0-test14（测试版）**。

## 使用

从 Release 下载完整运行包并解压，运行 `LegacySteamFixer.exe`。需要 .NET Framework 4.0 或以上。

1. 在 **Steam 工具**中确认 Steam 目录，刷新游戏列表。
2. 在上方选中游戏，点击 **修补**，检查并保存支持的修补方式。
3. 点击 **启动游戏**测试；需要撤销时点击 **还原**。

| 区域 | 功能 |
| --- | --- |
| 游戏库 | 显示已安装游戏，按名称或 AppID 筛选 |
| 游戏操作 | 修补、启动所选游戏、还原 |
| Steam 工具 | 选择 Steam 目录、启用 Zstd 补丁、打开官网安装器下载页 |
| 诊断 / 日志 | 查看分析结果、导出诊断、打开所选游戏注入日志目录 |

注入方式保存启动配置，保留原版游戏 DLL，之后需从本工具启动。替换方式备份原文件再安装桥接；有文件检查的游戏可能退出。游戏或客户端更新后，需重新分析，不能直接复用不匹配的配置。

首次手动升级请解压到新目录，将旧 `profiles` 与 `settings` 文件夹复制到新版。本体在线更新会保留这些目录。不要用旧 `inject`、`templates` 或 `zstd` 覆盖新组件。不要删除游戏内的修补备份。

## Steam 下载与 Zstd

下载按钮打开 Steam 官网安装器页面，不提供未经确认的历史客户端镜像。安装后仍须核对客户端是否在相应修补的支持范围。

Zstd 功能来自 [SteamLegacyZstd](https://github.com/yuyu107/SteamLegacyZstd)，针对指定 32 位 `steamclient.dll`，必须匹配 SHA-256：

```
d0e83c515f17ca57090c8c73664e5d61e37eae718dfa3a5cbb1e4b909548fc34
```

暂停下载并关闭游戏后启用，成功后恢复下载。Steam 完全退出或重启后需再次启用。原有特殊启动参数需要由用户先正常启动 Steam，再启用补丁。

## 已知情况

- Dancing Line 注入版已由用户实测完成一关，覆盖层可用。
- Rizline 注入启动已由用户实测可玩，但 Shift+Tab 覆盖层仍不可用；替换启动会退出。
- Timeline 兼容返回不提供游戏录制功能。
- 不承诺支持全部游戏或任意 Steam 构建。
- 部分安全软件会报告本工具及注入程序；排查需要具体检测名称，不能仅据此认定误报。
- test13 的模块提权、真实 Steam 注入和 Windows 重启替换仍需实机验证。

反馈请附系统版本、游戏名称、Steam 版本、启动方式以及“诊断 / 日志”导出的文件；注入问题同时提供 Injector/InjectBridge/Unity 日志。

## 构建与发布

Windows 下运行 `Build.cmd` 编译主工具与探测器。预编译桥接、注入和 Zstd 组件保留在对应文件夹；其独立构建方法见原脚本和上游说明。运行包需要全部组件，不能只分发主 EXE。

统一版本常量位于 `src/AppInfo.cs`。GitHub 发布准备、检查更新约定见 [docs/GITHUB.md](docs/GITHUB.md)。

## 许可

主项目使用 MIT 许可证，见 `LICENSE.txt`。第三方项目的许可证与来源保留在 `LICENSE-GMod.txt`、`LICENSE-tModLoader.txt`、`zstd` 和 `UPSTREAM.txt` 中。

## test11：通用修补与更新

GMod/tModLoader 的新修补与其他游戏共用通用分析、注入或替换流程，不再新安装专用模块。已有专用补丁仍可启动或还原；切换前请先还原。通用引擎目前只覆盖已支持的原生 x64 SDK，GMod 32 位及 tModLoader 托管接口路径不保证可生成方案；暂不支持时明确报告失败。

## test13：模块调用与本体更新

Zstd 不再分发或启动 `OldSteam_VSZa_Launcher.exe`。x86 Fixer 直接调用 `zstd/LegacySteam-ZstdModule.dll`，保留原有目标客户端和组件哈希校验。需要管理员权限时提权运行同一个 Fixer，执行完返回结果；不会再显示独立启动器窗口。`steam_zstd.dll` 和 `vsza_hook.bin` 仍为运行组件。

主程序使用 x86 构建，可在 32/64 位 Windows 上打开。游戏注入仍通过原有 x64 `inject/LegacySteam-Injector.exe` 完成，本次未改变游戏注入架构，亦未新增 32 位游戏支持。

标题区“检查更新”可保存“自动检查”与测试通道选项。默认开启启动时后台检查，最多每 24 小时一次；正式版本默认只检查正式通道，测试版本默认包含测试版。后台失败只记录日志，手动检查会显示错误。发现新版本才提示，下载和安装均由用户选择。

下载完整运行包后校验 GitHub digest 或 SHA256 附件，再检查 ZIP 路径、CRC、文件范围、完整性和主程序版本。下载可取消。校验完成后选择“重启并更新”，Fixer 的临时副本等待当前窗口退出，再备份、替换文件并重新打开主程序。无需另行分发更新器 EXE。替换失败会尝试还原旧文件，备份与错误日志保存在本机 `%LOCALAPPDATA%\LegacySteamFixer\Updates`。进程被强制终止或断电时不保证自动恢复，可根据该目录中的备份与 recovery.json 手动还原。

更新不覆盖 `profiles`、`settings`、`logs` 或游戏备份。安装目录不可写、组件被其他进程占用时会报告失败；不自动提权更新整个工具。若选择“稍后”，已校验 ZIP 保存在提示的路径，仍可手动解压。在线更新成功会移除旧 Zstd 独立 EXE。

仓库没有发布 Release 时显示暂无更新，未发布的源码提交不会作为更新。旧系统连接 GitHub 需具备可用的 TLS 1.2 和证书。真实 GitHub 下载、UAC 与 Windows 进程重启替换尚未实测。

## test12：自动读取启动模式

注入启动自动读取本机 `appcache/appinfo.vdf`，支持 0x07564427 / 28 / 29 缓存格式。多个适用 Windows 程序模式时显示选择列表；单一模式直接使用，无需手填参数。自动应用模式程序、参数和工作目录，并合并最近登录账号的 Steam 自定义 LaunchOptions。不同程序会重新验证对应 SDK 与注入适用性。缓存失败时提供普通 Steam 启动入口，不静默选择默认模式。

当前只读取可用的 Windows EXE 启动模式，不执行安装脚本、外部路径、非 Windows 模式或有未解析 Beta/DLC 条件的模式。无法确定时拒绝注入，不承诺完全复制 Steam 的全部条件判断。

本版测试采用合成 v27/v28/v29 缓存与本地回归；尚未拿到 Rizline/跳舞的线真实 appinfo 缓存进行实测。叠加层本次未继续修改，Rizline Shift+Tab 仍待排查。

## 自动发布

更新 `src/AppInfo.cs` 的版本与 `RELEASE_NOTES.md` 后，推送到 main（版本号文件有变化），或推送对应 `v版本号` 标签，即可触发 Actions 的 Build and release：重建 Zstd 模块、编译 Fixer 和探测器、运行回归检查、打包 ZIP/SHA256 并发布 Release。测试版标签自动标为 Pre-release，标题只有版本号。也可在 Actions 手动运行；已有 Release 不会被自动覆盖。
