"""Sơ đồ khối phần cứng: đầu vào - STM32F103C8 (ngoại vi dùng) - đầu ra, kèm chân nối và nguồn."""
import os, sys
sys.path.insert(0, os.path.dirname(__file__))
from kit import Canvas, HW
from matplotlib.patches import FancyBboxPatch, FancyArrowPatch
OUT = os.path.join(os.path.dirname(__file__), '..', '..', 'hw_block_diagram.png')
c = Canvas(17.5, 10.2); ax = c.ax
IN = ('#fefcbf', '#975a16'); OU = ('#c6f6d5', '#2f855a'); PE = ('#e8f1fb', '#2b6cb0'); PC = ('#edf2f7', '#4a5568'); PW = ('#fff5f5', '#c53030')
c.title(8.75, 9.85, 'SƠ ĐỒ KHỐI PHẦN CỨNG', fs=17)
# khung MCU
ax.add_patch(FancyBboxPatch((4.6, 0.5), 7.9, 7.0, boxstyle='round,pad=0,rounding_size=0.25', fc='#f7fbff', ec='#2b6cb0', lw=2, zorder=1))
ax.text(8.55, 7.2, 'STM32F103C8 (Blue Pill)', ha='center', fontsize=14, fontweight='bold', color='#2b6cb0')
c.block('core', 8.55, 4.6, 2.6, 1.5, 'Lõi Cortex-M3', '72 MHz · FreeRTOS', ('#bee3f8', '#2b6cb0'), fs=13)
ax.text(8.55, 4.05, 'Flash 64 KB · RAM 20 KB', ha='center', fontsize=9.5, color='#2d3748', zorder=6)
c.block('tim', 8.55, 2.2, 2.4, 0.75, 'TIM2 20 kHz', 'đo thời gian chạy task', PE, fs=11)
# ngoại vi trái
c.block('gpio', 5.75, 5.4, 1.9, 1.7, 'GPIO + EXTI', 'PA0, PB12, PB13', PE, fs=11)
c.block('uart', 5.75, 2.9, 1.9, 0.95, 'USART1', '+ DMA1 kênh 4', PE, fs=11)
c.block('swd', 5.75, 1.3, 1.9, 0.7, 'SWD', 'PA13, PA14', PE, fs=11)
# ngoại vi phải
c.block('spi', 11.35, 6.0, 1.9, 0.75, 'SPI1', '', PE, fs=11)
c.block('i2c', 11.35, 4.6, 1.9, 0.75, 'I2C1', '', PE, fs=11)
c.block('pc13', 11.35, 3.2, 1.9, 0.75, 'GPIO', '', PE, fs=11)
for k, side in (('gpio', 'r'), ('uart', 'r'), ('swd', 'r')):
    c.arrow(c.p(k, side), c.p('core', 'l', {'gpio': 0.35, 'uart': -0.35, 'swd': -0.55}[k]))
for k in ('spi', 'i2c', 'pc13'):
    c.arrow(c.p('core', 'r', {'spi': 0.45, 'i2c': 0.0, 'pc13': -0.45}[k]), c.p(k, 'l'))
c.arrow(c.p('tim', 't'), c.p('core', 'b'))
# đầu vào
ax.text(1.6, 7.6, 'ĐẦU VÀO', ha='center', fontsize=12, fontweight='bold', color='#975a16')
c.block('ldr', 1.6, 6.5, 2.5, 0.85, 'Cảm biến quang', 'quang trở + LM393 (DO)', IN, fs=11)
c.block('bn', 1.6, 5.4, 2.5, 0.6, 'Nút NEXT', '', IN, fs=11)
c.block('bo', 1.6, 4.4, 2.5, 0.6, 'Nút OK', '', IN, fs=11)
c.arrow(c.p('ldr', 'r'), c.p('gpio', 'l', 0.6), 'PA0 → EXTI0', fs=9)
c.arrow(c.p('bn', 'r'), c.p('gpio', 'l', 0.0), 'PB12 → EXTI12', fs=9)
c.arrow(c.p('bo', 'r'), c.p('gpio', 'l', -0.6), 'PB13 → EXTI13', fs=9, loff=(0, -0.28))
# máy tính
c.block('ttl', 1.6, 2.9, 2.5, 0.85, 'USB-TTL', 'nối máy tính (Hercules)', PC, fs=11)
c.block('stl', 1.6, 1.3, 2.5, 0.85, 'ST-Link V2', 'nối máy tính (Keil, nạp code)', PC, fs=11)
ax.add_patch(FancyArrowPatch(c.p('ttl', 'r'), c.p('uart', 'l'), arrowstyle='<|-|>', mutation_scale=15, lw=1.5, color='#1a202c', shrinkA=2, shrinkB=2))
ax.text(3.8, 3.03, 'PA9 TX, PA10 RX', ha='center', fontsize=9, bbox=dict(boxstyle='round,pad=0.1', fc='white', ec='none'))
c.arrow(c.p('stl', 'r'), c.p('swd', 'l'), 'SWDIO, SWCLK', fs=9)
# đầu ra
ax.text(15.5, 7.6, 'ĐẦU RA', ha='center', fontsize=12, fontweight='bold', color='#2f855a')
c.block('mx', 15.5, 6.0, 2.5, 0.85, 'Ma trận LED 8×8', 'MAX7219', OU, fs=11)
c.block('ol', 15.5, 4.6, 2.5, 0.85, 'OLED 128×64', 'SSD1306', OU, fs=11)
c.block('lp', 15.5, 3.2, 2.5, 0.85, 'LED PC13', 'báo hệ thống chạy', OU, fs=11)
c.arrow(c.p('spi', 'r'), c.p('mx', 'l'), 'PA7, PA5, PA4 (CS)', fs=8.5)
c.arrow(c.p('i2c', 'r'), c.p('ol', 'l'), 'PB6 SCL, PB7 SDA', fs=8.5)
c.arrow(c.p('pc13', 'r'), c.p('lp', 'l'), 'PC13', fs=8.5)
# nguồn
c.block('pw', 8.55, 8.75, 3.6, 0.7, 'Nguồn 5 V từ USB', 'USB-TTL hoặc micro USB', PW, fs=11.5)
c.arrow(c.p('pw', 'b'), (8.55, 7.5), color='#c53030')
ax.text(8.75, 8.05, '5 V → ổn áp 3,3 V trên bo', ha='left', fontsize=9, color='#c53030')
ax.add_patch(FancyArrowPatch(c.p('pw', 'r'), (15.5, 6.43), arrowstyle='-|>', mutation_scale=15, lw=1.5, color='#c53030',
                             connectionstyle='angle,angleA=0,angleB=90,rad=0', shrinkA=2, shrinkB=2))
ax.text(13.6, 8.9, '5 V cho MAX7219', ha='center', fontsize=9, color='#c53030')
ax.text(8.55, 0.05, 'Cảm biến, nút, OLED dùng nguồn 3,3 V lấy từ Blue Pill; mọi khối nối chung GND.', ha='center', fontsize=9.5,
        style='italic', color='#4a5568')
c.save(OUT)
print('ok')
