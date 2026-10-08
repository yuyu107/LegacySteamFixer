# LegacySteamFixer

面向 Windows 7 SP1 / 8.1 等旧系统的 Steam 游戏兼容工具。当前版本：**0.4.0-test10（测试版）**。

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

升级前备份旧工具目录；如需保留配置，将旧 `profiles` 文件夹复制到新版。不要用旧 `inject`、`templates` 或 `zstd` 覆盖新组件。不要删除游戏内的修补备份。

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
- test10 已通过编译及本地回归检查，界面排版仍需 Windows 实机验证。

反馈请附系统版本、游戏名称、Steam 版本、启动方式以及“诊断 / 日志”导出的文件；注入问题同时提供 Injector/InjectBridge/Unity 日志。

## 构建与发布

Windows 下运行 `Build.cmd` 编译主工具与探测器。预编译桥接、注入和 Zstd 组件保留在对应文件夹；其独立构建方法见原脚本和上游说明。运行包需要全部组件，不能只分发主 EXE。

统一版本常量位于 `src/AppInfo.cs`。GitHub 发布准备、检查更新约定见 [docs/GITHUB.md](docs/GITHUB.md)。

## 许可

主项目使用 MIT 许可证，见 `LICENSE.txt`。第三方项目的许可证与来源保留在 `LICENSE-GMod.txt`、`LICENSE-tModLoader.txt`、`zstd` 和 `UPSTREAM.txt` 中。
