# 发布与更新机制

仓库：https://github.com/yuyu107/LegacySteamFixer

程序从 GitHub Releases 检查更新；不会把未发布的源码提交当成新版本。测试通道包含 testN；正式通道排除 prerelease 和测试标签。版本源为 src/AppInfo.cs。

发布 test11 时使用标签 v0.4.0-test11，勾选 Pre-release。上传完整运行包 LegacySteamFixer-v0.4.0-test11.zip 和 LegacySteamFixer-v0.4.0-test11.sha256.txt，粘贴 RELEASE_NOTES.md。附件 SHA256 文本每行格式：64 位十六进制哈希、两个空格、完整运行包文件名。后续版本沿用同一命名规则。源码包不能代替运行包。

程序优先使用 GitHub asset digest，缺失时读取对应 sha256 附件；校验失败不会把临时文件保存为运行包。下载不会自动安装，用户解压到新目录并复制 profiles。

源码含必要预编译组件，GitHub 尚未自动重建全部原生组件。发布前验证 Windows 100%、125%、150% DPI，以及真实游戏和 GitHub 下载路径。
