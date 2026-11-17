# CH32H417 EVB NuttX 外设驱动手动测试指导书

> 适用: `ch32h41x-evb:nsh` 配置,V3F 核,SYSPLL 400MHz / HCLK 100MHz,USART1 @ **COM48**(WCH USB-SERIAL Ch A)115200。
> 维护约定: **每新增一个驱动,必须在本文档追加对应章节**(含: 跳线、命令、预期输出、失败排查),并同步更新文末"驱动覆盖总表"。

---

## 0. 测试前准备与故障恢复

### 0.1 上电与连接

| 项 | 内容 |
|---|---|
| 串口 | WCH USB-SERIAL Ch A → **COM48**, 115200-8-N-1(接 USART1 PA9/PA10) |
| 调试 | WCH-LinkE SDI (OpenOCD `wch-dual-core.cfg`,CPU0=GDB 3333 / CPU1=GDB 3334) |
| 心跳 LED | **PA2**(500ms 翻转,上电即闪 = 调度存活) |
| 基础跳线 | **PA8 → PA0**(PWM→捕获闭环) |

### 0.1.1 ⚠️ NSH 输出行为须知(避免误判"系统没跑")

- NSH 的提示符 **只在 boot 后约 1 秒内打印一次**,之后阻塞在控制台读取,**不会主动再输出**。
- 终端如果在复位/上电瞬间没连上,就会看不到任何字——**这不代表系统没跑**。
- **任何时候发一个回车**,都会立刻得到 `nsh> ` 提示符。
- **PA7 LED 每 500ms 闪烁 = 调度器活着**;LED 不闪或闪得极慢 = 时钟异常(见 0.5)。

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

### 0.5 时钟异常诊断(LED 变慢 / 串口乱码或全 0x00)

**症状对照:**

| 现象 | 根因 |
|---|---|
| LED 闪烁明显变慢,串口输出乱码 | HCLK 实际值远低于 100MHz。固件按 100MHz 编了 BRR,实际波特率 = HCLK/868。例如收到全 0x00 或只能用 ~6k 波特读出文字 = HCLK≈5MHz |
| 串口正常但 LED 闪 5 秒一次 | SysTick 时钟源异常(tick 变慢) |

**读时钟寄存器定位**(OpenOCD,无 `wp` 残留时):
```
openocd -f wch-dual-core.cfg -c "init; targets wch_riscv.cpu.0; halt; \
  mdw 0x40021000; mdw 0x40021004; mdw 0x40021008; \
  mdw 0x40013808; mdw 0xe000f010; exit"
```
- `CTLR(0x40021000)`: 期望含 bit16 HSEON + bit17 HSERDY + bit24 PLLON + bit25 PLLRDY = `0x0332xxxx` 级别
- `CFGR0(0x40021004)`: 期望 `0x0002000A`(SWS=PLL, HPRE=/1, FPRE=/4)
- `PLLCFGR(0x40021008)`: 期望 `0x80000030`(GATE=1, SEL=0, 源=HSE bit5, 倍频=0x10×16)
- `BRR(0x40013808)`: 期望 868(=0x364, 100MHz/115200)
- `STK CMP(0xe000f010)`: 期望 999999(=0xF423F, HCLK/100)

**机制**: 冷启动时 V5F 的 bootROM 会为自身启动检查编程 SYSPLL 并与本核竞争 RCC 寄存器;固件 now 按 EVT 官方时序配置(含 mux 同步等待)并在结束后回读验证、最多重试 4 次。若仍失败,读上述寄存器对照期望值定位是哪一步被打断。

**⚠️ 注意**: WCH-LinkE/OpenOCD 接入(`init`/`wlink_reset_resume`)会**自行改写 RCC 时钟寄存器**(典型残留: PLLCFGR=0x80000010、HSEON=0)。由于 HSI 与 HSE 同为 25MHz,频率不受影响——**调试器接入后读到的 RCC 状态不代表固件配置**,要在"干净"状态下读时钟寄存器,请冷启动后不接调试器直读(或以下述期望值差异判断)。

### 0.6 编译流程与版本回退

**编译链**(Windows/MSYS 环境,**必须**用脚本,勿直接调 `tools/configure.sh`,否则会选错 Linux host 配置):

```
sh tools/build-ch32h41x-evb.sh              # 增量构建(日常用,~1分钟)
sh tools/build-ch32h41x-evb.sh reconfig     # 全净重建: distclean+重新配置+编译(改了 Kconfig/defconfig/Make.defs 后用)
```

- 脚本自举: PATH 注入 MSYS make + kconfiglib + zig host 编译器 + WCH GCC12;MSYS 目录副本(`arch/risc-v/src/chip` 等)由 `tools/sync-dirlinks-msys.sh` 维护,**任何 make(含 configure 内部的 clean_context)之前副本必须存在**。
- 产物: `nuttx` (ELF) / `nuttx.bin` / `nuttx.hex`;版本串由 git 提交哈希自动嵌入(`uname -a` 可见,如 `aaf0afd889`)。

**已知良好版本(可回退)**:

| 内容 | 位置 |
|---|---|
| 固件提交 | git `evb-known-good-1` 标签(releases/ 内含对应 nuttx.bin) |
| 已知良好固件镜像 | `boards/risc-v/ch32h41x/ch32h41x-evb/releases/nuttx-known-good.bin` |

**回退到已知良好版本**:

1. **只回退固件**(最常用): 直接烧写 `releases/nuttx-known-good.bin`(见 0.4 烧写命令,换文件名即可)。
2. **回退源码**: `git checkout evb-known-good-1 -- arch/risc-v/src/ch32h41x boards/risc-v/ch32h41x` 然后 `reconfig` 全净重建。
3. **确认当前固件版本**: NSH 里 `uname -a`,对照 git log。

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

**跳线**: PB0 → PB1(输出回环);可选 PB0 → PB2(EXTI 测试,见 §4)
**命令**:
```
gpio -o 1 /dev/gpio0      # PA7 输出高
gpio /dev/gpio1           # 读 PA6 → value=1
gpio -o 0 /dev/gpio0
gpio /dev/gpio1           # → value=0
```
**预期**: 读回与写入一致。
**设备表**: /dev/gpio0=PB0(输出), /dev/gpio1=PB1(输入), /dev/gpio2=PB2(双边沿中断)。
**变更记录**: 原测试脚位 PA6/PA7 已让给 SPI1(PA5/6/7 = SCK/MISO/MOSI, AF5);LED 从 PA7 迁至 PA2。

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

## 7. 输入捕获 (TIM2_CH1 @ PA0) — PWM↔捕获闭环 ✅ 已验证

**跳线**: **PA8 → PA0**(必需)
**命令**:
```
timloop                    # 1kHz/50% 闭环自测(~12 秒)
timloop 25                 # 变占空比: 请求 25% 并测量
timloop 50 /dev/pwm1       # 指定 PWM 设备(如 TIM5/TIM12)
```
**预期**: 每秒打印 `freq=1000 Hz duty=50 %`,结束 `PASS (9/9 samples)`。
捕获以 PWM 输入模式双沿测量(TI1FP1 上升沿测周期 + TI1FP2 下降沿测高电平),频率与占空比同时互相印证,**不依赖示波器**。
已验证(2026-11-16): 50% 与 25% 占空比设定均精确测得。
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
| TIM1 | 高级 | 4CH+互补 | ESC/PWM 主输出 | ✅ 已验证 /dev/pwm0(闭环) |
| TIM2 | 通用 | 4CH | 输入捕获(当前) | ✅ /dev/capture0 |
| TIM3-5 | 通用 | 4CH | PWM/捕获扩展 | TIM5 ✅ /dev/pwm1(PC1 AF2)待跳线验证;TIM3(PA6)/TIM4(PB6)与 SPI1/I2C1 冲突 |
| TIM6/7 | 基本 | 无(时基) | 时间基准/触发 | ✅ oneshot 下半层 /dev/oneshot/0(见 §17) |
| TIM8 | 高级 | 4CH+互补 | 第二组 ESC 输出 | 🔧 驱动就绪;CH1=PA5 AF3 与 SPI1 SCK 冲突 |
| TIM9/12 | 通用 | 2CH | 额外 PWM | TIM12 ✅ /dev/pwm2(PB1 AF5)待跳线验证;TIM9 CH1=PC9 AF6 |
| TIM10/11 | 通用 | 1CH | 额外 PWM | 🔧 驱动就绪;TIM10 CH1=PB6 AF0 冲突 I2C1;TIM11 CH1=PD3 AF2 |

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
| TIM1 PWM | /dev/pwm0 (PA8) | §6/§7/§18 闭环 | ✅ |
| TIM2 捕获 | /dev/capture0 (PA0) | §7/§18 PWM 输入模式(freq+duty) | ✅ |
| selftest 闭环 | — | §8 | ✅ |
| TIM3/TIM5 PWM | pwm1=TIM3(PA6)/pwm2=TIM5(PC1) | §18 闭环 | ✅ 硬件验证 |
| TIM8 PWM | pwm3=TIM8(PC6 AF3,高级) | §18 闭环 | ✅ 硬件验证 |
| TIM9/10/11/12 PWM | 32位 4通道;TIM9=PC9,TIM10=PB6,TIM11=PD3,TIM12=PB1 | §18 闭环(TIM10 基线关) | ✅ 硬件验证 |

| TIM6/7 时基 | /dev/oneshot/0 | §17(200us~6.5s 全档) | ✅ 硬件验证 |
| TIM4 PWM | PB6(AF2) | §18 Round A 闭环(基线关,PB6 归 I2C1) | ✅ 硬件验证 |

| DMA 管理器 | (驱动内部 API) | ch32h41x_dma.c,供后续驱动复用 | ✅ 代码就绪(中断分发未接) |
| ADC1 | /dev/adc0 (ch16 温度/ch17 Vrefint) | §12 内部通道 | ✅ 硬件验证 |
| CAN1 | /dev/can0 (500k 回环) | §13 回环自发自收 | ✅ 硬件验证 |
| SPI1 主模式 | /dev/spi1 (PA5/6/7, AF5) | §15 回环模式 0-3+5MHz | ✅ 硬件验证 |
| I2C1 主机 | /dev/i2c0 (PB6/7) | §16 SPL06 ID=0x10 | ✅ 硬件验证 |
| I2C2 从机 | 实验性 | TXE 风暴待 FAE | 🧪 默认关 |

**版本快照**: `releases/nuttx-known-good-3.bin`(2026-11-17)= 12/12 定时器验证版;
基线含 TIM1/5/8/9/11/12(PWM,pwm0-5)+TIM2(捕获)+TIM6/7(oneshot),TIM3/4/10 因
引脚互斥(PA6/SPI1、PB6/I2C1)默认关闭。旧镜像 `releases/nuttx-known-good.bin` 仍保留,回退方法见 §0.6。
~~旧注: 上述验证状态对应固件镜像 `releases/nuttx-known-good-3.bin`~~
(定时器全矩阵 + oneshot + capture 占空比 + TIM9-12 32位修复);旧镜像
`releases/nuttx-known-good.bin`(evb-known-good-2)仍保留,回退方法见 §0.6。

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

---

## 12. ADC (/dev/adc0, 内部温度 + Vrefint)

**跳线**: 无(通道组 = 内部通道,不需要任何接线)
**配置**: CONFIG_CH32H41X_HAVE_ADC1=y(自动选中 ADC 上半层与 examples/adc)
**命令**:
```
adc -n 4              # 连续触发 4 组采样
```
**预期**: 每组输出 2 条消息:
- `channel: 17`(Vrefint): 12 位原始值,理论 ≈ VDDA/1.2V 比例(3.3V 时约 1500 上下,随电压浮动)
- `channel: 16`(温度传感器): 原始值;工厂校准参数位于 0x1FFFF76C(低 16 位=参考电压,高 16 位=参考温度),换算公式 `T = Refer_Temper - (raw - Refer_Volt)*10/43`(EVT TempSensor_Volt_To_Temper)
**原理**: ADC1 时钟来自 USBHS PLL(480MHz):校准期 /36,工作期 /6(EVT ADC_SPEED_High);校准序列 = RSTCAL 清零 → SAMPTR1[29:27]=0x7 → CAL 自清零(EVT ADC_StartCalibration)。内部通道必须开 CTLR1.BUFEN(EVT Internal_Temperature demo)。轮询式驱动:ANIOC_TRIGGER 一次转换整组(软件启动 + 有界 EOC 等待),经 au_receive 推入上半层 FIFO。
**失败排查**:
- adc 命令不存在 → defconfig 未启用 EXAMPLES_ADC(reconfig 重新生成)
- 读数恒 0 / ioctl 报错 → dmesg 看 "adc0 register failed";RAM 日志 hw_audit stage8
- 原始值≈0xFFF 或≈0 → BUFEN 未开(内部通道开路读数)
- **无样本输出、read 永久阻塞** → CONFIG_EXAMPLES_ADC_SWTRIG 未开(应用只在 SWTRIG 下发触发;本驱动为触发式轮询设计)

---

## 13. CAN1 (/dev/can0, 500kbit/s 回环) ✅ 已验证

**跳线**: 无(默认回环模式,发送帧自动进入自身 RX FIFO)
**配置**: CONFIG_CH32H41X_HAVE_CAN1=y(选中 examples/can)
**命令**:
```
can                   # 周期发送并接收,核对回显
```
**预期**: 发送的标准帧(默认 ID)在回环下立即出现在接收端,数据一致;bit timing = prescaler 20 × (1+6+3)tq @100MHz = 500kbit/s,采样点 70%。
**机制**: bxCAN 控制器;BTIMR[31:30]=模式(回环=1),BRP 为 **10 位**(0x3FF,比 STM32 bxCAN 的 6 位宽,EVT CAN_BTIMR_BRP);过滤器集中在 CAN1 块,**本版硅片硬件过滤器无效(EVT 注明 version-A),配置单个"32 位掩码全接受"bank 0**,编码规则:掩码模式=FMCFGR 位清零、32 位=FSCFGR 位置一、FIFO0=FAFIFOR 位清零、激活=FWR 位置一(EVT CAN_FilterInit)。RX 用 CAN1_RX0 中断排空 FIFO0 → can_receive();TX 在 co_send 内有界轮询 RQCP/TXOK。
**切正常模式**(需收发器): can 应用无模式参数,需在代码中调用
`CANIOC_SET_CONNMODES`(bm_loopback=false)——真总线调试时再加。
**已验证**: 回环自发自收,数据逐字节一致(2026-11-13,COM6)。
**失败排查**:
- **INAK 不来 / open 报 -EIO** → **CTLR.SLEEP(bit1) 没清**!复位默认睡眠,睡眠中的核心忽略 INRQ(EVT CAN_Init 在置 INRQ 前先 `CTLR &= ~SLEEP`)
- 发送超时(-ETIMEDOUT) → 回环位未生效:读 BTIMR bit30
- 收不到 → RFIFO0 FMP 计数、RX0 中断挂接
- 帧内容错位 → 检查 TXMDLR/TXMDHR 字节序(每 32 位字低字节在前)
- "Bit timing not available" 提示 → can 应用查询了我们未实现的 ioctl,无害

---

## 15. SPI1 (/dev/spi1, 主模式回环) ✅ 已验证

**跳线**: **PA7(MOSI) → PA6(MISO)**(一根;SCK/CS 无需接线)
**配置**: CONFIG_CH32H41X_HAVE_SPI1=y(选中 SPI_EXCHANGE/SPI_DRIVER/SYSTEM_SPITOOL)
**命令**:
```
spi exch -b 1 deadbeef              # 发送 hex 串(不带 0x),读回=发送;默认 8 字
spi exch -b 1 -x 16 deadbeef        # -x N: 传输 N 字(不足补 0xFF)
spi exch -b 1 -m 1 a5c3691f         # -m: 模式 0-3(1-3 已实测)
spi exch -b 1 -f 5000000 beeff00d   # -f: 频率(5MHz 已实测)
spi exch -b 1 -w 16 deadbeef        # -w: 帧宽 8/16/32(8=默认)
```
**预期**: 读回字节与发送完全相同。
**原理**: SPI1 @ HB2 总线(时钟位 12, RM 已核);引脚 PA5=SCK/PA6=MISO/PA7=MOSI(AF5, CH32H417DS0 引脚表);PA4=NSS 软件管理(SSM|SSI)。轮询 TXE/RXNE 有界等待;bit rate = HCLK/2^(BR+1), BR=0..7。
**已验证**: 模式 0-3 + 5MHz 全部数据精确回显(2026-11-13)。
**失败排查**:
- **读回全 0x00(已接跳线)** → **MISO 输入脚没设 AF!** 本芯片 F4 式复用:输入脚同样需要 AF 编号路由进外设(EVT 主机配置对 MISO 也调 GPIO_PinAFConfig)——`ch32h41x_gpio_setaf(0, 6, 5)` 不可省
- 读回全 0xFF → 跳线接在别处
- MODF/模式错误 → SSM|SSI 必须置位(软件 NSS 主模式)
- spitool 语法: **命令在前、公共参数在后**(`spi exch -b 1 ...`);`-x N` 传输字数、`-w 8|16|32` 帧宽、`-f` 频率、`-m` 模式;默认字数 = CONFIG_SPITOOL_DEFWORDS(本配置 = 8)

---

**DMA 交换(2026-11-17)**: ≥16 字节的 exchange 走 DMA1(TX=通道3/请求63,
RX=通道2/请求64,RM 表10-2),RX 完成为结束信号;短传输保持轮询。回环实测:
轮询 4/8B 与 DMA 16/32B 均精确回显模式字节。Kconfig 
CH32H41X_SPI1_DMA(默认关)。

## 16. I2C1/I2C2 (/dev/i2c0, /dev/i2c1) - 主机已验证,从机实验性

**跳线(2根)**: PB6(I2C1 SCL) → PC0(I2C2 SCL);PB7(I2C1 SDA) → PC1(I2C2 SDA)。
**上拉**: I2C 开漏总线需要外部上拉(4.7kΩ 到 3V3)。**未接上拉时总线无法回到高电平**。
**配置**: CONFIG_CH32H41X_HAVE_I2C=y(I2C2 从机响应器 0x50 自动启动)
**命令**:
```
i2c dev -b 0 0x03 0x77            # I2C1 主机扫描 0x03-0x77 → 50 应出现(I2C2 从机)
i2c set -b 0 -a 50 11 22          # I2C1 写 2 字节到从机镜像缓冲
i2c get -b 0 -a 50                # I2C1 读回 → 应为 11 22(set/get 往返)
```
**原理**: I2C1 = PB6/PB7(AF4), I2C2 = PC0/PC1(AF9), 开漏复用输出(RM 表9-7);F4 式复用下**输入脚也要设 AF**。I2C2 从机 ISR: ADDR 匹配后按方向收/发,STOPF 复位,读回=镜像最后收到的缓冲。
**已验证(2026-11-16, 上拉 + 从机响应器关闭)**: 扫描全表 NACK 快速完成 = 主机电气行为正确(总线上拉生效、START/STOP/ACK 检测/AF 均工作)。LED 与系统全程稳定。
**失败排查**:
- **全表 `--` 且快速完成(有上拉时)**: 正常——总线上无真实从机设备;I2C2 从机响应器默认关闭(见下)
- 扫描挂死/LED 停闪 → I2C2 从机响应器开启后,主机读从机时 TXE 电平中断风暴(已定位: 从机 TX 模式下 DATAR 写入无法清 TXE);修复从机发送时序前保持 I2C2_SLAVE 关闭
- detect 空(已接从机) → 检查跳线 PB6-PC0 / PB7-PC1 是否插实

---

## 17. Oneshot (/dev/oneshot/0, TIM7) ✅ 已验证

**跳线**: 无  **配置**: CONFIG_CH32H41X_HAVE_ONESHOT=y + CONFIG_EXAMPLES_ONESHOT=y
**命令**:
```
oneshot                    # 默认 2s 延时,打印实际经过时间
oneshot -d 6500000         # 6.5s,接近 16bit 满量程(最大 6.55s)
oneshot -d 200             # 200us,分辨率下限附近
```
**预期**: `Elapsed time=...`(与请求值一致,100us 分辨率内)。
**说明**: TIM7 基本定时器,100us 分辨率(10kHz 计数时钟),最大延时 6.55s;
写 PSC 后必须发 UG 更新事件装载预分频影子寄存器(EVT 基本定时器时序)。
**已修复的坑**:
- `.timer` 索引错误(0/1)导致使能了 TIM1/TIM2 的总线时钟而非 TIM6/TIM7,寄存器写入全部无效;
  正确索引: TIM 层 5=TIM6, 6=TIM7(与 clock_enable/uirq[] 一致)。
- PSC 影子未装载(缺 UG)→ 计数时钟实为 100MHz 而非 10kHz。
- 本驱动实现 COUNT 接口(clkcnt 计数,oneshot_count_init 发布 10kHz 时钟);
  ARCH_CHIP_CH32H41X 强制 select ONESHOT_COUNT,旧式 timespec 驱动与该接口 ABI 不兼容
  (ops 签名不同,GCC 指针不兼容仅警告不报错),混用会导致 ioctl 乱值/定时器不触发。
**失败排查**: /dev/oneshot 不存在 → bringup HW_STAGE(12) 是否执行(看 syslog)。

## 18. 定时器全矩阵验证(TIM1-TIM12)✅ 全部完成

**最终基线设备映射**(pwm0-6):
| /dev | 定时器 | CH1 引脚(AF) | 闭环验证 |
|---|---|---|---|
| pwm0 | TIM1(高级) | PA8(AF1) | ✅ 50%/25% |
| pwm1 | TIM5 | PC1(AF2) | ✅ 50% |
| pwm2 | TIM8(高级) | PC6(AF3) | ✅ 50% |
| pwm3 | TIM9(32位) | PC9(AF6) | ✅ 50% |
| pwm4 | TIM11(32位) | PD3(AF2) | ✅ 50% |
| pwm5 | TIM12(32位) | PB1(AF5) | ✅ 50%/25% |

| TIM2 | 捕获 /dev/capture0 | PA0(AF1) | ✅ PWM 输入模式(freq+duty) |
| TIM6/7 | /dev/oneshot/0 | — | ✅ 200us~6.5s |
| TIM3 | (基线关闭) | PA6(AF2)/PB4/PC6/PD3 | ✅ Round A 闭环 PASS;PA6 归 SPI1 MISO |
| TIM4 | (基线关闭) | PB6(AF2)/PD12 | ✅ Round A 闭环 PASS;PB6 归 I2C1(SPL06) |
| TIM10 | (基线关闭) | PB6(AF0)/PA6(AF9)/PF8 | ✅ Round B 闭环 PASS;与 TIM4/I2C1 争 PB6 |
| TIM3 | (基线关闭) | PA6(AF2)/PB4/PC6/PD3 | ✅ Round A 闭环 PASS;PA6 归 SPI1 MISO(实测劫持致 SPI 全零) |

**验证方法**: 每个定时器 CH1 跳线到 PA0,TIM2 PWM 输入捕获闭环,
`timloop 50 /dev/pwmN`(timloop 首参为占空比百分比)。

**关键发现: TIM9-12 是 32 位定时器**(CNT/ATRLR/CH1-4CVR 均 32 位,4 通道;
BF 侧 SPL 以 `CNT_32/ATRLR_32/CHxCVR_32` union 呈现)。16 位写 ARR/CCR 会残留高半字
脏值 → 周期错误/输出静止。pwm 驱动已按宽度分流(`ch32h41x_pwm_is32bit()`)。
TIM8 CH1=PC6 AF3;PA5 AF3 是 TIM8_CH1N(互补 N 通道),不要用。

**PB6 互斥**: TIM4(AF2)/TIM10(AF0)与 I2C1 SCL(SPL06 气压计)共用 PB6。
使能任一即劫持该脚(bringup stage 13 后写覆盖),SPL06 即失联——实测确认。
基线默认关闭二者;若需启用,须放弃 PB6 的 I2C 功能或将 TIM4 改用 PD12。
**MSYS 构建注意**: 改动 .config(如 kconfig-tweak)后必须 `make clean` 全量重编——
`.dep` 不追踪 config.h,增量 make 会留下陈旧对象(症状: 设备数与配置不符)。

