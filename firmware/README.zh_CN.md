[English](README.md)

# Codex 用量看板 v6

本目录包含本次固件、电脑同步程序、测试和原项目 BSP 的完整源码。
v6 已验收合并固件 SHA-256：`75c08d1a7d2332eeec46ba86d4857445d055ba0bf722d5aecf8aa31ed21731e2`。

- [应用说明](docs/codex-token-display.zh_CN.md)
- [开发与环境准备](docs/README.zh_CN.md)
- 使用 ESP-IDF 5.5.3，运行 `./tools/validate.sh` 生成并校验可从 0x0 刷写的合并镜像。
- Python 同步程序：在安装并登录 Codex 的电脑上，先执行
  `python -m pip install -r companion/requirements.txt`，再执行
  `python companion/codex_tokens.py --transport auto`。
- Windows 免安装同步程序与启动脚本位于本仓库 v6 Release。

构建和主机测试通过；实机已完成的验收范围见应用说明。重新构建的固件身份可能改变，发布文件以上述哈希为准。
