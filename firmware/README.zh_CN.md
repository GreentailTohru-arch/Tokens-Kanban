[English](README.md)

# Codex Tokens Kanban v9

完整固件、电脑同步程序、测试、原始录音及复用 BSP 源码。
已验证合并镜像 SHA256 为 `220dd600137cc9622754746a82492687c7006ebb2d4b0b619821147fb1bd25f3`。

- [应用与验收说明](docs/codex-token-display.zh_CN.md)
- [开发环境](docs/README.zh_CN.md)
- 使用 ESP-IDF 5.5.3，运行 `./tools/validate.sh` 构建并验证从 0x0 刷写的镜像。
- 安装 `companion/requirements.txt` 后运行 `python companion/codex_tokens.py --transport auto`。
- Windows 程序与启动脚本位于本仓库 v9 Release。

构建、主机测试及限定范围的实机验收通过，长期、距离和压力测试未验证。
重新构建可能与已验证发布文件的身份不同。
