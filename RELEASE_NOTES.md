## 0.4.0-test18

- Steam 工具区新增“修复首次安装”，针对初始化时提示不支持当前 Windows 的情况。
- 请求 Steam 正常退出，备份并移走根目录 tier0_s.dll、tier0_s64.dll，再带 -no-cef-sandbox 重试引导下载；不清理游戏或账户文件。
- 生成 Start-Steam-Legacy.cmd 供再次启动，并记录备份位置和 SHA256；检查重复执行、未知脚本、进程占用、部分失败回滚与 x64-on-x86 架构不匹配。
- 新增独立修复说明，主界面、Zstd 说明和 README 均提供入口。

方案步骤已由用户在 Win8.1 32 位手动验证；Fixer 自动流程、UAC、Win7 和其他位数仍需实机测试。-no-cef-sandbox 会关闭 CEF 沙箱。Steam 负责实际下载及选择客户端，Fixer 不提前报告安装成功。此功能不要求启用 VxKex，也不能让 32 位系统运行 64 位程序。

可从 test17 测试在线升级。升级后在下方“Steam 工具”点击“修复首次安装”。
