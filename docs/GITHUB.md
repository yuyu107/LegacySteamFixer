# 发布与更新机制

仓库：https://github.com/yuyu107/LegacySteamFixer

程序从已发布的 GitHub Releases 检查更新，不使用未发布源码提交。版本源为 src/AppInfo.cs；正式通道排除 prerelease 和 test 标签，测试通道可接收 testN 与正式版。

当前版本为 0.4.0-test15。发布标签必须为 v0.4.0-test15，并勾选 Pre-release。上传完整运行包 LegacySteamFixer-v0.4.0-test15.zip，以及 LegacySteamFixer-v0.4.0-test15.sha256.txt，正文使用 RELEASE_NOTES.md。

附件 SHA256 文本每行格式：64 位十六进制哈希、两个空格、完整运行包文件名。程序优先使用 GitHub asset digest，缺失时读取同版本 sha256 附件。仅源码包不支持本体更新。

运行 ZIP 内放一个 LegacySteamFixer 目录，包含主 EXE、EXE.config、完整 modules/inject/templates/probes/zstd 与许可证。不要包含 profiles/settings/logs、源码目录、旧 Zstd 启动器。编译后运行 `python tools/package.py 输出目录` 会生成统一命名的 ZIP 和 SHA256。必须保留 zstd/LegacySteam-ZstdModule.dll 与 src/ZstdSupport.cs 校验值一致。新增原生模块由 zstd/source/build_module.py 构建，依赖 pip ziglang（本次 0.17.0）。Build.cmd 编译 x86 主工具和两个探测器。

下载校验并暂存后，用户选择重启。临时 Fixer 副本等待旧进程退出，备份替换文件，失败回滚；配置不在可覆盖范围。该流程没有另行发布的更新器程序。

发布使用 `.github/workflows/release.yml`：修改版本号并推送 main、推送标签，或在 Actions 手动运行。标签必须与 AppInfo.cs 一致；自动生成运行包及 SHA256 并创建 Release，已有版本不覆盖。可用 `tools/test.ps1` 在 Windows 运行相同回归检查。正式发布前验证 Win7/8.1 上 UAC、真实 Zstd、自动检查开关、测试通道、真实资产下载与重启更新。先用 test14 检查 test15，并验证 profiles/settings 保留。也要验证模块文件被占用时是否安全失败并还原。
