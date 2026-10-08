# GitHub 发布准备

建议仓库名：LegacySteamFixer
建议描述：Steam game compatibility tools for legacy Windows, with injection launch profiles and integrated Zstd support.

远程仓库：https://github.com/yuyu107/LegacySteamFixer 。首次同步包含源码、文档和必要组件；本次未发布 Release。保留所有第三方许可证。源码包包含必要预编译组件；它们尚未由 GitHub Actions 自动重建。

## 发布本测试版

1. 在 Windows 运行 Build.cmd，并检查主窗口、修补窗口、Zstd 窗口和诊断窗口；建议验证 100%、125%、150% DPI。
2. 标签使用 v0.4.0-test10，勾选 Pre-release。
3. 上传完整运行 ZIP 及对应 SHA256 文件，粘贴 RELEASE_NOTES.md。
4. README 记录实测支持范围，不能把本地编译或模拟测试写为真实游戏测试。

## 为检查更新保留的约定

- src/AppInfo.cs 是程序显示版本与程序集信息的版本源。
- release-manifest.example.json 是后续更新清单的字段示例，不是在线更新源。
- repository_url、release_url、asset_url、sha256 必须在实际仓库和文件发布后填写，不能把示例作为可用更新发布。
- 后续检查更新提供稳定版与测试版两种通道。稳定版跳过 prerelease；测试版可以显示 testN。
- 比较核心数字版本，再比较测试版序号；同核心版本正式版高于测试版，不能按字符串比较 test9 和 test10。
- GitHub 的“latest”不应作为测试版通道的唯一来源。
- 用户主动检查，提供发行说明和下载页面。网络失败与“已经最新”必须区分。
- 第一版先打开发布页下载，后续如实现自动下载，再验证清单 SHA256；不得自动覆盖正在运行的程序或 profiles、游戏备份。

本版只准备版本源和发布结构，尚未加入联网检查更新或自动升级。
