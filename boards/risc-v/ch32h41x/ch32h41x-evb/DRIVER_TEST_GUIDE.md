# CH32H417 EVB NuttX 外设驱动手动测试指导书

> 适用: `ch32h41x-evb:nsh` 配置,V3F 核,400MHz (SYSPLL),USART1 @ COM6 115200。
> 维护约定: **每新增一个驱动,必须在本文档追加对应章节**(含: 跳线、命令、预期输出、失败排查),并同步更新文末"驱动覆盖总表"。

---

## 0. 测试前准备与故障恢复

### 0.1 上电与连接

| 项 | 内容 |
|---|---|
| 串口 | WCH-LinkE CDC → COM6, 115200-8-N-1 |
| 调试 | WCH-LinkE SDI (OpenOCD `wch-dual-core.cfg`) |
| 基础跳线 | **PA8 → PA0**(PWM→捕获闭环) |

### 0.2 ⚠️ 遇到"串口无输出/CPU 卡死"先做这个

调试器(OpenOCD `wp` 命令)布下的**硬件观察点会残留在 QingKe 触发器模块里,跨软复位存活,并阻塞 SRAM 写**,导致每次上电都卡死在启动代码的 bss 清零循环(PC=0x128)。

**恢复步骤(按顺序):**
1. **给板子断电**(拔掉板子供电,不只是 USB 线)→ 重新上电。断电会清空调试触发器。
2. 若仍无输出: 断电重插 WCH-LinkE USB(其 CDC 桥偶发卡死,表现为收到全 0 字节)。
3. 若怀疑固件问题,用调试器读"自测日志"(见 0.4)而不是串口。

### 0.3 无串口时的替代验证(headless)

固件内置两条 RAM 诊断通道,断电重启后由 OpenOCD 读取:

| 通道 | 地址 | 内容 |
|---|---|---|
| 自测日志 `g_testlog` | `nm nuttx | grep g_testlog` | selftest 应用的 PASS/FAIL 逐行日志 |
| 启动阶段堆审计 | `0x2017f000` 起,每阶段 16 字节 `{stage, gits, end, firstsize}` | bringup 各阶段后堆链健康快照 |
| 控制台镜像环 | `0x2017f800`(2KB) | 所有 up_putc 输出(含 panic 消息) |
| 未注册 IRQ 记录 `g_unexp_irq` / `g_unexp_count` | nm 查地址 | 意外中断号列表 |

OpenOCD 读法(在 `wch-dual-core.cfg` 目录):
```
openocd -f wch-dual-core.cfg -c "init; targets wch_riscv.cpu.0; halt; \
  dump_image out.bin 0x2017f800 2048; exit"
```

### 0.4 烧写

```
tools/build-ch32h41x-evb.sh                # 增量构建
cd <OpenOCD/bin>
openocd -f wch-dual-core.cfg -c "adapter speed 2000; init; \
  targets wch_riscv.cpu.0; halt; \
  program <nuttx 路径>/nuttx.bin verify 0x00000000; \
  wlink_reset_resume; exit"
```
注意: flash 地址是 **0x00000000**(不是 0x08000000);必须用 **双核配置** `wch-dual-core.cfg`。

---

## 1. 系统与时钟

**跳线**: 无  **命令**:
```
uname -a
```
**预期**: `NuttX 12.12.0 ... risc-v ch32h41x-evb`,串口无乱码。
**原理**: USART1 波特率由 100MHz HCLK 分频;串口干净 = 时钟树(SYSPLL 400MHz, V5F/400M, HCLK=100M)正确。
**示波器加强**: PA9 (USART1 TX) 逐字节 115200 波特。

**失败排查**: 无输出 → 0.2 恢复步骤;乱码 → 检查 SYSCLK 配置与 BRR。

---

## 2. FPU (RV32IMAFC 单精度)

**跳线**: 无  **命令**:
```
ostest
```
**预期**: 全部子测试通过,无 `FAIL` / `PANIC`;含多线程 FPU 上下文切换压力测试。
**配置**: `CONFIG_ARCH_FPU=y`, `CONFIG_ARCH_LAZYFPU=y`(惰性保存:仅浮点任务切换时存/取 FPU 寄存器)。

**注意**: 本平台 FPU 使能 = head.S 写 QingKe CSR 0xbc0 + mstatus.FS=Initial;若 FPU 未使能时执行 `flw/fsw` 会触发异常(mcause=4)并死循环,表现为启动卡死——出现此症状先查 0xbc0 写入与 FPU 配置开关。

---

## 3. GPIO 字符设备 (PA7/PA6/PA4)

**跳线**: PA7 → PA6(输出回环);可选 PA7 → PA4(EXTI 测试,见 §4)
**命令**:
```
gpio -o 1 /dev/gpio0      # PA7 输出高
gpio /dev/gpio1           # 读 PA6 → value=1
gpio -o 0 /dev/gpio0
gpio /dev/gpio1           # → value=0
```
**预期**: 读回与写入一致。
**设备表**: /dev/gpio0=PA7(输出), /dev/gpio1=PA6(输入), /dev/gpio2=PA4(双边沿中断)。

**失败排查**: 全 0 → 跳线未接;某一半区失败 → 检查 CFGLR(PA7 应为输出 50MHz, PA6 输入)。

---

## 4. EXTI 外部中断 (PA4, 双边沿)

**跳线**: PA7 → PA4
**命令**:
```
gpio -w 2 /dev/gpio2 &    # 注册并等待信号 2(后台)
gpio -o 1 /dev/gpio0      # 上升沿 → 后台任务被唤醒退出
gpio -o 0 /dev/gpio0      # 可重复触发验证
```
**预期**: 每次电平变化,等待任务被唤醒一次。
**配置**: `CONFIG_CH32H41X_EXTI=y`,EXTI7_0 线(IRQ 36),触发器 FTENR/RTENR bit4。

**注意**: PA4 在 bringup 中被配置为**下拉输入**(防浮空中断风暴,见 bringup.c 注释);若去掉该配置,浮空噪声会以数千次/秒触发意外中断。

---

## 5. 独立看门狗 IWDG

**跳线**: 无  **命令**:
```
wdog                       # 喂狗若干次后故意停喂 → 板子复位 → NSH 重启
wdog -h                    # 查看参数
```
**预期**: 停喂约 2s 后板复位(串口重新出现启动信息)。
**原理**: IWDG 一旦使能**不可停止**,只能靠喂狗维持;验证的是"停喂→复位"路径与复位后系统完整性。
**加强**: `selftest`(§9)以 RAM 交接块自动验证此路径,无需人工观察。

---

## 6. PWM 输出 (TIM1_CH1 @ PA8)

**跳线**: 可选 PA8 → PA0(配合 §7 闭环)
**命令**:
```
pwm -h                     # 用法
pwm -f 1000 -d 50 -t 30    # 1kHz/50%,30 秒
pwm -f 10 -t 5             # 频率扫描: 10/100/1k/10k/100k 各测
pwm -f 100000 -d 1 -t 5    # 占空比边界 1%/99%
```
**预期(示波器 CH1 探 PA8)**: 频率与设定值完全一致,Vpp≈3.3V,占空比吻合。
**机制**: TIM1_CH1 经 AF1 输出;PSC/ARR 由 `pwm_start()` 按 HCLK(100MHz) 计算。

**失败排查**:
- 频率整体偏差同一比例 → HCLK 与假设不符(查时钟树)。
- 无输出 → PA8 的 AF1 路由(bringup 中 `gpio_setaf`)、TIM1 时钟(HB2PCENR bit11)。

---

## 7. 输入捕获 (TIM2_CH1 @ PA0) — PWM↔捕获闭环

**跳线**: **PA8 → PA0**(必需)
**命令**:
```
timloop                    # 一体化闭环自测(60 秒窗口)
```
**预期**: 每秒打印 `edges=... (+1000/s 左右)`,结束 PASS。捕获计数与 PWM 设定互相印证,**不依赖示波器**。
**机制**: TIM2_CH1 经 AF1 输入捕获,TIM2 计数器 1MHz(TIM2 时钟来自 HCLK 经 PSC),捕获差值=周期。
**失败排查**: edges=0 → 跳线;edges=15 左右 → 未配置线的误捕获(老问题,已修)。

---

## 8. 看门狗+闭环自动化 (selftest 应用)

**跳线**: PA8 → PA0(必需);PA7→PA6、PA7→PA4 可选
**命令**:
```
selftest
```
**预期**: 依次输出 PWM/CAPTURE/GPIO/EXTI/IWDG 各项 PASS/FAIL;IWDG 项会让板子复位一次(看门狗验证),复位后自动汇报 `IWDG: PASS`。
**用途**: 开机自检(与 INIT 入口解耦,NSH 下手动运行)与无串口 headless 验证(日志在 RAM,见 0.3)。

---

## 9. 已规划/实现中的定时器矩阵 (TIM1-TIM12)

飞控基石需要完整定时器覆盖。当前状态与计划:

| 定时器 | 类型 | 通道 | 计划用途 | 状态 |
|---|---|---|---|---|
| TIM1 | 高级 | 4CH+互补 | ESC/PWM 主输出 | ✅ 已注册 /dev/pwm0 |
| TIM2 | 通用 | 4CH | 输入捕获(当前) | ✅ /dev/capture0 |
| TIM3-5 | 通用 | 4CH | PWM/捕获扩展 | ✅ 已在驱动表 |
| TIM6/7 | 基本 | 无(时基) | 时间基准/触发 | ⏳ 计划: oneshot 定时器下半层 |
| TIM8 | 高级 | 4CH+互补 | 第二组 ESC 输出 | 🔧 已注册待上板 |
| TIM9/12 | 通用 | 2CH | 额外 PWM | 🔧 已注册待上板 |
| TIM10/11 | 通用 | 1CH | 额外 PWM | 🔧 已注册待上板 |

**新增定时器验证模板**(每个新定时器照此执行):
1. 在 `ch32h41x_tim.c` 的 `g_timisr[]/uirq[]/clock_enable` 补齐本定时器的索引、时钟位、中断号。
2. 在 `ch32h41x_pwm.c` 的 `g_pwmtimers[]` 增加条目(类型/通道数),高级定时器需补 BDTR MOE。
3. board.h 增加 output pin(AF 表核对 DS0)。
4. 测试: `pwm -p /dev/pwmN -f 1000 -d 50 -t 10` + 示波器;或接入 selftest 自动化(读 CNT 运行 + 捕获闭环)。
5. 本文档追加章节并更新总表。

---

## 10. 驱动覆盖总表(随每个新驱动更新)

| 驱动 | 设备 | 验证手段 | 状态 |
|---|---|---|---|
| USART1 串口 | COM6 | 本文档全程 | ✅ |
| 时钟/复位的时钟树 | — | §1 | ✅ |
| FPU | — | §2 ostest | ✅ |
| GPIO 字符设备 | /dev/gpio0-2 | §3 | ✅ |
| EXTI | /dev/gpio2 | §4 | ✅ |
| IWDG | /dev/watchdog0 | §5 / §8 | ✅ |
| TIM1 PWM | /dev/pwm0 (PA8) | §6 / §7 | ✅ |
| TIM2 捕获 | /dev/capture0 (PA0) | §7 | ✅ |
| selftest 闭环 | — | §8 | ✅ |
| TIM3-5 PWM | /dev/pwm1-3 | 待接线验证 | 🔧 已注册未实测 |
| TIM8 PWM | /dev/pwm1 (4CH) | §9 模板 + selftest | 🔧 已注册待上板 |
| TIM9-12 PWM | /dev/pwm2-5 (2/1/1/2CH) | §9 模板 + selftest | 🔧 已注册待上板 |
| TIM6/7 时基 | oneshot | ⏳ 待实现 | 计划 |

| DMA | ⏳ | WP4 | 计划 |
| ADC | ⏳ | WP4 | 计划 |
| SPI | ⏳ | WP5 (MOSI↔MISO 回环) | 计划 |
| I2C | ⏳ | WP6 (I2C1↔I2C2 双控制器) | 计划 |
| CAN (bxCAN) | ⏳ | WP7 (回环模式) | 计划 |

## 11. 新驱动章节模板

```markdown
## N. <驱动名> (<硬件单元> @ <引脚>)

**跳线**: ...
**配置**: CONFIG_XXX=y
**命令**:
```
<命令序列>
```
**预期**: <可观测判据>
**机制**: <一句话原理>
**失败排查**: <症状 → 检查项>
```
