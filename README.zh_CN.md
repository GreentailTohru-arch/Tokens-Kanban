[English](README.md)

# Tokens Kanban

[打开明信片编辑器](https://greentailtohru-arch.github.io/Tokens-Kanban/)

在 Chrome／Edge 中通过网页蓝牙或 USB 连接 AI Passport，在小窗中调整头像和二维码构图，
编辑标题、副标题与联系方式，再保存到设备。需配合兼容的 Passport 固件。
连接前先退出电脑用量同步程序，释放设备连接。

图片在浏览器本地处理，经用户点击授权后直接写入设备。GitHub 仅托管静态编辑器和公开默认素材。
通用头像和指向本网页的二维码为默认内容；设备上的个人卡片仅在保存成功后替换。
网页不会把导入的图片提交到 GitHub 或上传到图片服务器。

硬件应用复用 [FoloToy AI Passport](https://gitee.com/FoloToy/ai-passport) 的 BSP。
代码沿用 MIT 授权。默认头像为原创、严格左右对称的矢量用户图，MIT 授权，源文件为 `default-avatar.svg`。

## 固件与电脑同步程序

当前 v6 的完整源码位于 [firmware/](firmware/)，包含 BSP、应用、测试与电脑同步程序。
[下载 Windows 配套程序](https://github.com/GreentailTohru-arch/Tokens-Kanban/releases/tag/v6)。
设备可查看 Codex 用量与金额估算、趋势，并展示个人明信片。上下键切换周期，单击确定查看明细或联系方式，双击确定循环三页。
设备无需配网；获取当前账户额度需要电脑已登录 Codex 并可联网。
