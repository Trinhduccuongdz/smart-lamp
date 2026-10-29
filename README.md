# Đèn thông minh tự động theo ánh sáng trên nền FreeRTOS, có giám sát tải CPU

Bài tập lớn môn Hệ thống nhúng. Vi điều khiển STM32F103C8 (Blue Pill), FreeRTOS V11.1.0,
**Keil C (MDK-ARM)** + thư viện **STM32F10x Standard Peripheral (SPL) V3.5.0**,
kiến trúc phân tầng **Driver → Middleware → Application**.

Khi phòng chuyển sang tối, ma trận LED **sáng dần**; khi sáng trở lại, đèn **tắt dần**.
Người dùng chỉnh chế độ, độ sáng, hẹn giờ tắt qua **menu trên OLED bằng 2 nút**, hoặc gõ lệnh
từ máy tính qua **USB-TTL**. Hệ thống tự **đo tải CPU** (tổng và từng task) bằng hai phương
pháp để đối chiếu, và cho phép **cho CPU ngủ khi rảnh** để đánh giá tiết kiệm năng lượng.

## 1. Tài liệu thiết kế (thư mục `docs/`)

| File | Nội dung |
|---|---|
| `1_block_diagram.png` | Sơ đồ khối: đầu vào → khối xử lý → đầu ra, kèm chân và ngoại vi |
| `2_task_ipc.png` | Các task, ISR và đối tượng FreeRTOS (queue, mailbox, semaphore, mutex, event group, timer) |
| `3_layers.png` | Kiến trúc phân tầng, màu theo người phụ trách |
| `4_lamp_fsm.png` | Máy trạng thái của đèn (mục 6: State machine / event-driven firmware) |

Nguồn sơ đồ ở `docs/src/*.dot` (Graphviz). Sửa xong chạy: `dot -Tpng docs/src/1_block_diagram.dot -o docs/1_block_diagram.png`.

## 2. Phần cứng và nối dây

| Thiết bị | Chân thiết bị | Blue Pill | Ngoại vi |
|---|---|---|---|
| Cảm biến quang (LM393) | VCC / GND / DO | 3.3V / GND / **PA0** | GPIO + EXTI0 |
| Nút NEXT (nối GND) | 1 chân / chân kia | **PB12** / GND | GPIO pull-up + EXTI12 |
| Nút OK (nối GND) | 1 chân / chân kia | **PB13** / GND | GPIO pull-up + EXTI13 |
| Ma trận LED MAX7219 | VCC / GND / DIN / CS / CLK | **5V** / GND / **PA7** / **PA4** / **PA5** | SPI1 |
| OLED SSD1306 (I2C) | VCC / GND / SCL / SDA | 3.3V / GND / **PB6** / **PB7** | I2C1 400 kHz |
| Mạch USB-TTL | RX / TX / GND | **PA9** / **PA10** / GND | USART1 115200 8N1, TX qua DMA1 kênh 4 |
| LED trên bo | | PC13 | nháy mỗi giây = hệ thống còn chạy |

Bảng chân được định nghĩa **duy nhất** trong `bsp/inc/board.h`.

Kiểm tra trước khi chạy:
- **Cảm biến:** che tay, xem chân DO lên mức nào khi tối. Mặc định coi `DO = 1` là tối; nếu ngược thì đổi `LIGHT_SENSOR_DARK_LEVEL` trong `light_sensor.h`.
- **OLED** địa chỉ 0x3C; nếu là 0x3D thì sửa `SSD1306_I2C_ADDR` trong `ssd1306.h`.
- **Ma trận** loại 4 module nối tiếp: đặt `MAX7219_NUM_DEVICES` = 4 trong `max7219.h`.

## 3. Cấu trúc thư mục và phân tầng

```
smart_lamp/
├── bsp/              # Board support & pin mapping: board.h (bảng chân), nút, cảm biến quang,
│   ├── inc/          #   MAX7219, SSD1306, font5x7
│   └── src/
├── hal/              # Drivers ngoại vi (dùng SPL): hal_clock, hal_nvic, hal_gpio, hal_exti,
│   ├── inc/          #   hal_spi, hal_i2c, hal_uart (+DMA), hal_timer
│   └── src/
├── middlewares/      # FreeRTOS V11.1.0 (kernel, port ARM_CM3, heap_4) + dịch vụ dùng chung:
│   ├── FreeRTOS/     #   console (bộ đệm dòng lệnh UART), cpu_monitor (đo tải CPU), xprintf
│   ├── inc/
│   └── src/
├── app/              # FreeRTOS tasks & IPC (không truy cập thanh ghi, không gọi SPL)
│   ├── inc/          #   app_ipc.h (kênh giao tiếp), app_cfg.h (ưu tiên, stack), task_xxx.h,
│   │                 #   FreeRTOSConfig.h, lamp_fsm.h, menu.h, effects.h, lamp_types.h
│   └── src/          #   app_ipc.c, task_xxx.c (6 task), lamp_fsm.c, menu.c, effects.c, hooks.c
├── main.c            # Init hardware, tạo kênh IPC, tạo task, vTaskStartScheduler()
├── platform/         # Mã của hãng, không sửa: SPL V3.5.0, CMSIS, startup (Keil và GCC)
├── keil/             # SmartLamp.uvprojx - project Keil, nhóm file theo tầng
├── test/             # unit test và kiểm thử tích hợp mô phỏng trên máy tính
├── docs/             # sơ đồ thiết kế
├── .gitignore
└── README.md
```

Quy tắc phân tầng:
- `main.c` chỉ khởi tạo bo mạch, tạo kênh giao tiếp, tạo task và chạy scheduler. Không có logic chức năng.
- `app/` không include header của `hal/`, không gọi SPL, không truy cập thanh ghi.
- `bsp/` và `middlewares/` không gọi SPL, không truy cập thanh ghi, chỉ gọi hàm của `hal/`.
- Chỉ `hal/` gọi SPL. Muốn đổi sang thư viện khác chỉ cần viết lại `hal/`, các tầng trên giữ nguyên.
- `hal/` không phụ thuộc FreeRTOS: báo sự kiện lên tầng trên bằng **callback** (chạy trong ISR).
- Mỗi task nằm gọn trong `app/src/task_xxx.c` và chỉ đưa ra ngoài hàm `task_xxx_create()` qua `app/inc/task_xxx.h`.

## 4. Phân công: mỗi người sở hữu task của mình

| Người | Task (app) | BSP / Middlewares | HAL |
|---|---|---|---|
| **1. Đầu vào + điều khiển đèn** | `task_input.c`, `task_lamp.c`, `lamp_fsm.c`, `menu.c`, unit test | `button.c`, `light_sensor.c` | `hal_gpio.c`, `hal_exti.c` |
| **2. Hiển thị + xuất dữ liệu** | `task_display.c`, `effects.c`, `task_log.c` | `max7219.c`, `ssd1306.c`, `font5x7.c`, `console.c` | `hal_spi.c`, `hal_i2c.c`, `hal_uart.c` (+DMA) |
| **3. Giao tiếp lệnh + giám sát hệ thống** | `task_console.c`, `task_monitor.c`, `hooks.c`, `main.c` | `cpu_monitor.c`, `board.c`, `xprintf.c` | `hal_clock.c`, `hal_nvic.c`, `hal_timer.c` |
| Cả nhóm | `app_ipc.h/.c`, `app_cfg.h`, `lamp_types.h` | | |

Các task **chỉ trao đổi qua kênh** khai báo trong `app_ipc.h` (queue, mailbox, event group).
Không task nào gọi hàm hay sửa biến của task người khác. Có lỗi thì xem dữ liệu trên kênh:
sai quy ước là lỗi bên gửi, đúng quy ước mà phản ứng sai là lỗi bên nhận.

## 5. Các task

| Task | Ưu tiên | Kích hoạt | Việc làm |
|---|---|---|---|
| MonitorTask | 4 | tuần hoàn 1 s | hiệu chuẩn idle, đo tải CPU, cập nhật `qCpuStat`, nháy LED |
| InputTask | 3 | notification từ ISR EXTI và timer chống nhiễu | sinh `DARK/LIGHT`, `BTN_xxx` vào `qEvents` |
| Timer daemon | 3 | software timer | chống nhiễu (3 timer), `tmrFade` 100 ms, `tmrOff` hẹn tắt |
| LampTask | 2 | `qEvents` | máy trạng thái đèn + menu, ghi `qLampView` |
| ConsoleTask | 2 | notification từ ISR USART1 (đủ 1 dòng) | phân tích lệnh |
| DisplayTask | 1 | tuần hoàn 40 ms | hiệu ứng ma trận (SPI), OLED mỗi 200 ms (I2C) |
| LogTask | 1 | `qLog` | gatekeeper UART TX, phát bằng DMA + binary semaphore |
| Idle | 0 | khi rảnh | idle hook: đếm + `__WFI()` (nếu bật ngủ) |

Khởi động: mọi task (trừ Monitor) chờ bit `READY` của event group `egSystem`. MonitorTask dùng
1 giây đầu để đo `idle_max` khi CPU rảnh hoàn toàn, sau đó mới đặt `READY`.

## 6. Mở project trên Keil C, biên dịch và nạp

Cần: **Keil MDK-ARM 5** (µVision) với **ARM Compiler 6** (mặc định từ MDK 5.37) và pack **Keil::STM32F1xx_DFP**
(cài trong Pack Installer). Thư viện SPL, CMSIS và FreeRTOS đã nằm sẵn trong project, không cần tải thêm.

1. Mở `keil/SmartLamp.uvprojx`. Cây project chia nhóm theo tầng: Startup, CMSIS, StdPeriph_Driver,
   FreeRTOS, HAL, BSP, Middlewares, App.
2. **Build** (F7). Kết quả `keil/Objects/smart_lamp.hex`.
3. Nối ST-Link (SWDIO, SWCLK, GND, 3.3V) và **Download** (F8).
   Nếu Keil báo không thấy ST-Link: Options for Target → Debug → chọn **ST-Link Debugger** → Settings → Port **SW**.

Thiết lập đã có sẵn trong project (để đối chiếu nếu phải tạo project mới):

| Mục (Options for Target) | Giá trị |
|---|---|
| Device | STM32F103C8 |
| Target → ARM Compiler | Use default compiler version 6, tích **Use MicroLIB** |
| Target → bộ nhớ | IROM1 0x08000000, 0x10000; IRAM1 0x20000000, 0x5000 |
| C/C++ → Define | `USE_STDPERIPH_DRIVER,STM32F10X_MD` |
| C/C++ → Language C | c99 |
| C/C++ → Include Paths | `..\app\inc; ..\bsp\inc; ..\middlewares\inc; ..\hal\inc; ..\middlewares\FreeRTOS\include; ..\middlewares\FreeRTOS\portable\RVDS\ARM_CM3; ..\platform\cmsis\core; ..\platform\cmsis\device; ..\platform\StdPeriph_Driver\inc` |
| Asm → Assembler Option | armasm (Arm Syntax), cho file startup |
| Linker | Use Memory Layout from Target Dialog |

**Lưu ý:**
- **Port FreeRTOS:** với ARM Compiler 6 dùng port `portable/GCC/ARM_CM3` (FreeRTOS khuyến nghị cách này, vì AC6 dựa trên clang và hiểu cú pháp GCC).
  Nếu máy chỉ có **ARM Compiler 5**, thay file `port.c` trong nhóm FreeRTOS bằng `portable/RVDS/ARM_CM3/port.c`
  và đổi include path tương ứng sang `..\middlewares\FreeRTOS\portable\RVDS\ARM_CM3`.
- **Không bật RTE:** không tích Device → StdPeriph Drivers hay Device → Startup trong *Manage Run-Time Environment*,
  vì các file đó đã có trong project. Thêm lần nữa sẽ bị trùng hàm.

**Không có Keil** (Linux hoặc chỉ cần kiểm tra biên dịch): `make` dùng `arm-none-eabi-gcc` với cùng mã nguồn;
`make flash` nạp bằng `st-flash`.

## 7. Lệnh qua cổng nối tiếp (115200 8N1, gõ xong nhấn Enter)

| Lệnh | Tác dụng |
|---|---|
| `help` | danh sách lệnh |
| `status` | trạng thái đèn, cảm biến, cài đặt |
| `on` / `off` / `toggle` | bật / tắt / đảo đèn |
| `mode 0..2` | 0 FULL, 1 BREATH (thở), 2 WAVE (sóng) |
| `bright 1..16` | độ sáng tối đa |
| `timer N` | hẹn tắt sau N phút kể từ khi đèn sáng hẳn (0 = không) |
| `auto 0/1` | tắt/bật tự động theo cảm biến |
| `cpu` | tải CPU (2 phương pháp), heap, bảng % CPU và stack trống của từng task |
| `sleep 0/1` | tắt/bật cho CPU ngủ (`__WFI`) trong idle hook |
| `log 0/1` | in mỗi giây một dòng CSV: `CPU,thời_gian,tải_idlehook,tải_runtime,ngủ` |

Nút nhấn: menu đóng thì **OK** = bật/tắt đèn, **NEXT** = mở menu. Trong menu: NEXT chuyển mục,
OK vào sửa / lưu, nhấn giữ ≥ 0.8 s để đóng menu hoặc hủy sửa.

## 8. Đo tải CPU và thí nghiệm năng lượng

**Phương pháp 1: idle hook** (gợi ý của thầy). `vApplicationIdleHook()` tăng biến đếm mỗi vòng Idle.
Hiệu chuẩn lúc khởi động cho `idle_max` lần/giây (CPU rảnh 100%). Khi chạy, mỗi giây đọc `idle_now`:
**Tải CPU = 100% × (1 − idle_now / idle_max)**.

**Phương pháp 2: run-time stats.** Bật `configGENERATE_RUN_TIME_STATS`, TIM2 đếm 20 kHz làm thang thời
gian, `uxTaskGetSystemState()` cho thời gian chạy của từng task. Lấy hiệu số mỗi giây để có
**% CPU từng task**, tải tổng = 100% − % của Idle.

Gợi ý thí nghiệm cho báo cáo:
1. `log 1`, để đèn ở từng trạng thái (OFF, FULL, BREATH, WAVE, mở menu), ghi tải CPU hai phương pháp, so sánh.
2. Dùng `cpu` xem task nào tốn CPU nhất (dự kiến là DisplayTask vì phải chờ bus I2C khi vẽ OLED).
3. `sleep 1`: phương pháp 1 không còn đúng (CPU ngủ nên Idle chạy ít vòng), phương pháp 2 vẫn đúng.
   Giải thích sự khác biệt này trong báo cáo.
4. Mắc đồng hồ đo dòng nối tiếp nguồn Blue Pill (tháo ma trận để chỉ đo MCU), so sánh dòng khi `sleep 0` và `sleep 1`.

## 9. Kiểm thử

- `make test`: unit test cho `lamp_fsm`, `menu`, `effects`, `xprintf` (module thuần logic, chạy trên máy tính).
- `make sim` (Linux): kiểm thử tích hợp, chạy toàn bộ tầng Application với FreeRTOS POSIX port và phần cứng giả trong `test/sim/`; task kịch bản đổi cảm biến, nhấn nút, gõ lệnh rồi kiểm tra 17 điều kiện.
- Mỗi người tự test task của mình bằng **task giả (stub)**: ví dụ người 2 viết task tạm gửi `EV_SENSOR_DARK`
  vào `qEvents` để thử đèn khi chưa có InputTask; người 3 test console và đo tải CPU khi chỉ có task giả tạo tải.
- `configASSERT`, kiểm tra tràn stack (`configCHECK_FOR_STACK_OVERFLOW = 2`) và hook hết heap in lỗi
  qua UART rồi nháy nhanh LED PC13.

## 10. Đối chiếu tiêu chí bài tập lớn

| Tiêu chí | Đáp ứng |
|---|---|
| Sử dụng phối hợp ngoại vi | GPIO, EXTI, Timer (TIM2, SysTick), SPI, I2C, UART, DMA, NVIC (qua thư viện SPL) |
| Quản lý luồng thông tin | queue, mailbox, task notification, binary semaphore, mutex, event group, software timer |
| Đánh giá hoạt động | tải CPU 2 phương pháp, stack high water mark, heap còn trống, chế độ ngủ |
| Xây dựng kiến trúc | sơ đồ khối và sơ đồ task làm trước, code theo sơ đồ |
| Module hóa phần mềm, thư viện | 3 tầng, mỗi thiết bị một thư viện, mỗi người một nhóm task |
| Đóng gói và thử nghiệm | Makefile, unit test, test từng task bằng stub |
| Mục 6 (mở rộng) | **State machine, event-driven firmware** (`lamp_fsm.c`, `menu.c`); thêm **Unit Test** |
