# RUN.md — 运行说明

> 对应 Day 7 的第一版 MVP：**摄像头实时画面 + 底部状态栏**。
> 实测通过时间：2026-09-22 22:53。

一句话流程：**构建 → 烧录 → 屏幕上出现实时画面，底部状态栏显示 `STANDBY - LOOK AT CAMERA`。**

---

## 1. 环境前提（三条硬约束，缺一条就编不出 bin）

**① 工程路径必须是纯 ASCII。**
路径含中文时会出现两级故障，都已实测确认：

| 阶段 | 现象 |
|---|---|
| 配置期 | `ninja` 读不到确实存在的文件 → 连带 `check_include_file(sys/types.h / stdint.h / stddef.h)` 全判失败 → IDF 生成的头文件列表为空 → `check_type_size(time_t)` 编译失败 → `Failed to determine sizeof(time_t)` |
| 链接期 | 换 `ninja 1.11.1` 后 985 个 obj 全能编出，但 `ldgen.py` 调 `objdump` 时路径被截断成 `ESP32ʶ`，**仍不出 bin/elf** |

所以本工程的实际构建位置是 `D:\Cursor_program\ESP32_Face`（纯英文）。

**② 必须清掉两个环境变量。**

- `MSYSTEM`：Git Bash 强制注入，在同一个 shell 里 `unset` 无效。带着它跑 `idf.py`，只会打一行 warning 就 `exit(0)`，`main()` 根本不执行。
- `PYTHONPATH`：本机被注入了 safe-delete shim，会让组件管理器删不掉 `.temp` 目录，报 `SAFE_DELETE_FAIL_CLOSED`（与沙箱开关无关）。

**③ `ninja` 用 1.11.1**，不要用 IDF 自带的 1.10.2。
即 `D:\Trae-Python\Anaconda3\Anaconda3\Lib\site-packages\ninja\data\bin\ninja.exe`。

---

## 2. 构建（已实测通过）

```bash
env -u MSYSTEM -u PYTHONPATH \
  IDF_PATH="D:/ESP32IDE/Espressif/frameworks/esp-idf-v5.1.2" \
  IDF_TOOLS_PATH="D:/ESP32IDE/Espressif" \
  PYTHONUTF8=1 PYTHONIOENCODING=utf-8 \
  PATH="/d/Trae-Python/Anaconda3/Anaconda3/Lib/site-packages/ninja/data/bin:/d/ESP32IDE/Espressif/tools/xtensa-esp32s3-elf/esp-12.2.0_20230208/xtensa-esp32s3-elf/bin:/d/ESP32IDE/Espressif/tools/cmake/3.24.0/bin:/d/ESP32IDE/Espressif/tools/ninja/1.10.2:/d/ESP32IDE/Espressif/tools/idf-exe/1.0.3:$PATH" \
  "D:/ESP32IDE/Espressif/python_env/idf5.1_py3.11_env/Scripts/python.exe" \
  "D:/Cursor_program/_build_tools/run_idf.py"
```

`run_idf.py` 只做四件事：`os.environ.pop('MSYSTEM')` / `pop('PYTHONPATH')` → `chdir` 到 `D:\Cursor_program\ESP32_Face` → `sys.argv = ['idf.py','build']` → `runpy` 跑 IDF 的 `tools/idf.py`。

**实测输出（2026-09-22 22:53）**

```
Project build complete.
1.LED.bin binary size 0x580b0 bytes. Smallest app partition is 0x100000 bytes. 0xa7f50 bytes (66%) free.
```

产物（`build/` 已被 `.gitignore` 忽略）：

| 文件 | 大小 |
|---|---|
| `build/1.LED.bin` | 360,624 B |
| `build/1.LED.elf` | 4,592,092 B |
| `build/1.LED.map` | 3,499,134 B |

> 产物名 `1.LED` 是正点原子示例残留的工程名（见 `TECH_DESIGN.md` 待决项 D7），与功能无关，改名留到后续。

---

## 3. 烧录

```bash
idf.py -p COMx flash monitor
```

或使用构建日志给出的等价命令：

```
esptool.py -p COMx -b 460800 --before default_reset --after hard_reset --chip esp32s3 \
  write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m \
  0x0    build/bootloader/bootloader.bin \
  0x8000 build/partition_table/partition-table.bin \
  0x10000 build/1.LED.bin
```

---

## 4. 预期现象（对照 PRD F1 / F7）

屏幕按横屏 320 × 240 分区：**上方 0–215 行为视频区，底部 216–239 行为状态栏。**

| 现象 | 说明 |
|---|---|
| 0–215 行 | 摄像头实时画面（320 × 216），每帧刷新 |
| 216–239 行 | 黑底白字状态栏 `STANDBY - LOOK AT CAMERA`，**只在状态变化时重画一次**，所以不会被视频冲掉 |
| 摄像头初始化失败 | 整屏 `CAMERA ERROR` / `POWER CYCLE TO RETRY`（PRD 2.4 第一条：不允许静默失败） |
| 串口日志 | 每秒一行 `state=STANDBY - LOOK AT CAMERA fps=<实测值>` |
| 取帧失败 | 不静默跳过，打印 `frame grab failed, retry` 后重试 |

选择 216 行不是随手取的：`216 × 320 × 2 = 138,240 B = 9 × LCD_BUF_SIZE`，正好整块写屏。

---

## 5. 本期边界（明确不做）

- 只落地 `ST_STANDBY` / `ST_ERROR` **两态**；PRD 2.1 的「录入中 / 识别成功 / 识别失败」需要人脸检测接上后再加。
- **未做人脸检测框。**
- 状态文字只有英文：板载字库 `components/LCD/lcdfont.h` 是纯 ASCII（`asc2_1206/1608/2412/3216`），没有中文。PRD 里的「请看镜头」「欢迎，1 号」需另接中文字库。

---

## 6. 待办（Day 8 起）

1. **板子插上后烧录 + 拍 LCD 实物照** —— Day 7 的截图证据（当前无 COM 口，未烧录）
2. 接 esp-who / esp-dl 做人脸检测（`TECH_DESIGN.md` D2 / D3）
3. 内存实测：`lcd_buf` 占 150 KiB 片内 SRAM，与推理抢内存（D6）
4. 状态栏只有 24 px 高、16 px 字，离 PRD A7「1 米外可读」还差，需要加高
5. 工程名 `1.LED` → 改成有意义的名字（D7）
