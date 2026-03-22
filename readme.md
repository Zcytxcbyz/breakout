# Breakout - 打砖块游戏

基于 **Box2D** 物理引擎和 **SFML** 图形库的打砖块游戏，使用 **ImGui** 实现 UI 界面。支持随机砖块血量、碰撞加速、独立 exe 运行。

---

## ✨ 游戏特性

- 经典打砖块玩法：移动挡板反弹小球，击碎所有砖块即可获胜
- **随机砖块系统**：砖块按概率生成，拥有 1~3 点血量，不同颜色代表不同血量
- **物理碰撞**：完全弹性碰撞，球与挡板、砖块、边界自然交互
- **动态难度**：每次碰撞后球速略微提升，最高速度有限制，游戏节奏逐步加快
- **完整 UI**：
  - 主菜单（开始游戏/帮助/退出）
  - 帮助窗口（操作说明和砖块颜色对照）
  - 实时分数显示
  - 游戏结束窗口（胜利/失败，可重开或返回主菜单）
- **单文件可执行**：静态编译所有依赖，无需额外 DLL，仅依赖 Windows 系统库

---

## 🎮 操作说明

| 按键            | 功能       |
|-----------------|-----------|
| `A` / `←`       | 挡板左移   |
| `D` / `→`       | 挡板右移   |

游戏目标：击碎所有砖块。若小球掉落底部则游戏失败。

---

## 📦 依赖库

项目使用以下库（已包含在源码中，无需手动安装）：

- **SFML 3.0.2**（图形、窗口、系统）
- **Box2D 2.4.2**（物理引擎）
- **Dear ImGui 1.91.9**（界面库）
- **imgui-sfml 3.0**（ImGui 与 SFML 的绑定）

所有依赖均以源码形式存放在 `libs/` 目录，通过 CMake 统一编译。

---

## 🔧 编译与构建

### 环境要求

- Windows 10/11
- Visual Studio 2022 或更高版本（或 MinGW-w64）
- CMake 3.15 或更高版本

### 步骤

1. **克隆仓库**（假设你已拥有源码）：
   ```bash
   git clone https://github.com/yourname/breakout.git
   cd breakout
   ```

2. **生成项目文件**  
   打开终端，执行以下命令（以 Visual Studio 2022 为例）：
   ```bash
   mkdir build
   cd build
   cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
   ```

3. **编译**  
   在 Visual Studio 中打开生成的 `breakout.sln`，选择 `Release` 配置，生成解决方案；或直接在命令行：
   ```bash
   cmake --build . --config Release
   ```

4. **运行**  
   编译完成后，可执行文件位于 `build/Release/breakout.exe`，直接运行即可。

> **注意**：若使用 MinGW-w64，可将生成器改为 `"MinGW Makefiles"`，并确保已安装对应工具链。

---

## 🚀 运行与分发

- **Release 版**已静态链接所有依赖，且禁用了控制台窗口，可直接双击运行。
- 若需分发，只需将 `breakout.exe` 复制到目标机器即可，无需附带任何 DLL。

---

## 🛠️ 自定义配置

游戏参数集中定义在 `src/breakout.cpp` 的 `Config` 命名空间中，你可以轻松调整：

- 窗口大小、帧率
- 球/挡板/砖块的尺寸、速度、密度、弹性
- 砖块生成概率、血量范围、分数
- 碰撞加速系数、最大速度等

修改后重新编译即可生效。

---

## 📄 许可证

本项目仅供学习交流使用，你可以自由修改和分发，但请保留原作者信息。

---

## 🙏 致谢

- [SFML](https://www.sfml-dev.org/) – 简单易用的多媒体库
- [Box2D](https://box2d.org/) – 强大的 2D 物理引擎
- [Dear ImGui](https://github.com/ocornut/imgui) – 轻量级即时模式 GUI
- [imgui-sfml](https://github.com/SFML/imgui-sfml) – ImGui 与 SFML 的桥接层

---

## 📧 联系

如有问题或建议，欢迎提交 Issue 或联系作者。

**Enjoy the game!**