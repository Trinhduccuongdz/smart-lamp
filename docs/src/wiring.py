"""Sơ đồ cắm chân phần cứng: Blue Pill (nhìn từ trên, cổng USB ở trên) và các module.
Chạy: python3 docs/src/wiring.py  ->  docs/wiring.png
"""
import os
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.patches import FancyBboxPatch, Rectangle

plt.rcParams['font.family'] = 'DejaVu Sans'
OUT = os.path.join(os.path.dirname(__file__), '..', 'wiring.png')

# Thứ tự chân Blue Pill (USB ở trên), từ trên xuống
LEFT = ['B12', 'B13', 'B14', 'B15', 'A8', 'A9', 'A10', 'A11', 'A12', 'A15', 'B3', 'B4', 'B5', 'B6', 'B7', 'B8', 'B9', '5V', 'G', '3V3']
RIGHT = ['G', 'G', '3V3', 'R', 'B11', 'B10', 'B1', 'B0', 'A7', 'A6', 'A5', 'A4', 'A3', 'A2', 'A1', 'A0', 'C15', 'C14', 'C13', 'VB']
USED = {'B12', 'B13', 'A9', 'A10', 'B6', 'B7', 'A7', 'A5', 'A4', 'A0', 'C13', '5V', 'G', '3V3'}

BX0, BX1, TOP, STEP = 6.9, 9.3, 10.0, 0.42
NET = {'5V': '#d62728', '3V3': '#ff7f0e', 'GND': '#222222'}

fig, ax = plt.subplots(figsize=(17, 11.5))
ax.set_xlim(-0.2, 17.2); ax.set_ylim(0.6, 11.7); ax.axis('off')


def pin_y(i):
    return TOP - STEP * i


def board_pin(name, side):
    lst = LEFT if side == 'L' else RIGHT
    i = lst.index(name)
    return ((BX0 if side == 'L' else BX1), pin_y(i))


# ---------------- Blue Pill ----------------
ax.add_patch(FancyBboxPatch((BX0, pin_y(19) - 0.35), BX1 - BX0, pin_y(0) - pin_y(19) + 0.9,
                            boxstyle='round,pad=0,rounding_size=0.12', fc='#1f5fa8', ec='black', lw=1.5))
ax.add_patch(Rectangle((7.7, TOP + 0.3), 0.8, 0.35, fc='#c0c0c0', ec='black'))
ax.text(8.1, TOP + 0.42, 'USB', ha='center', va='center', fontsize=8)
ax.text(8.1, 6.2, 'BLUE\nPILL', ha='center', va='center', color='white', fontsize=12, fontweight='bold')
ax.text(8.1, 5.3, 'STM32\nF103C8', ha='center', color='white', fontsize=8.5)
for side, lst in (('L', LEFT), ('R', RIGHT)):
    for i, n in enumerate(lst):
        x = BX0 + 0.18 if side == 'L' else BX1 - 0.18
        used = n in USED
        ax.add_patch(plt.Circle((x, pin_y(i)), 0.1, fc='#ffd400' if used else '#dddddd', ec='black', lw=0.6))
        ax.text(x + (0.2 if side == 'L' else -0.2), pin_y(i), n, ha='left' if side == 'L' else 'right',
                va='center', fontsize=8, color='white', fontweight='bold' if used else 'normal')


# ---------------- Module ----------------
def module(x, y, w, title, pins, side, fc, note=None):
    """pins: list of (label, net or None). side: chân ở cạnh 'L' hoặc 'R' của module. Trả về dict label->(x,y)."""
    h = 0.42 * len(pins) + 0.7
    ax.add_patch(FancyBboxPatch((x, y), w, h, boxstyle='round,pad=0,rounding_size=0.1', fc=fc, ec='black', lw=1.2))
    ax.text(x + w / 2, y + h - 0.3, title, ha='center', va='center', fontsize=10.5, fontweight='bold')
    if note:
        ax.text(x + w / 2, y - 0.25, note, ha='center', va='top', fontsize=8.3, style='italic')
    pos = {}
    for k, (lab, net) in enumerate(pins):
        py = y + h - 0.75 - 0.42 * k
        px = x if side == 'L' else x + w
        ax.add_patch(plt.Circle((px, py), 0.07, fc='white', ec='black', lw=0.8))
        ax.text(px + (0.15 if side == 'L' else -0.15), py, lab, ha='left' if side == 'L' else 'right', va='center', fontsize=8.5)
        pos[lab] = (px, py)
        if net:
            net_flag(px, py, net, -1 if side == 'L' else 1)
    return pos


def net_flag(x, y, net, d):
    """Nhãn nguồn: các chân cùng nhãn được nối chung."""
    x2 = x + d * 0.45
    ax.plot([x, x2], [y, y], color=NET[net], lw=2)
    ax.text(x2 + d * 0.06, y, net, ha='left' if d > 0 else 'right', va='center', fontsize=8.5,
            color='white', fontweight='bold', bbox=dict(boxstyle='round,pad=0.15', fc=NET[net], ec='none'))


def wire(p, q, color, xmid, label=None):
    (x1, y1), (x2, y2) = p, q
    ax.plot([x1, xmid, xmid, x2], [y1, y1, y2, y2], color=color, lw=2.2, solid_capstyle='round')
    for (x, y) in (p, q):
        ax.add_patch(plt.Circle((x, y), 0.06, fc=color, ec=color))
    if label:   # nhãn đặt sát chân Blue Pill, phía trên dây
        d = 1 if x2 > x1 else -1
        ax.text(x2 - d * 0.15, y2 + 0.1, label, fontsize=8, color=color, va='bottom',
                ha='right' if d > 0 else 'left', fontweight='bold')


# Nhãn nguồn trên Blue Pill
net_flag(*board_pin('5V', 'L'), '5V', -1)
net_flag(*board_pin('G', 'L'), 'GND', -1)
net_flag(*board_pin('3V3', 'L'), '3V3', -1)

# Nút nhấn (trái trên)
btn = module(1.0, 8.53, 2.6, '2 nút nhấn', [('NEXT', None), ('OK', None), ('chân còn lại', 'GND')], 'R', '#ead1dc',
             note='mỗi nút: 1 chân vào PBx, chân kia vào GND')
wire(btn['NEXT'], board_pin('B12', 'L'), '#9467bd', 5.2, 'B12 = NEXT (EXTI12)')
wire(btn['OK'], board_pin('B13', 'L'), '#8c564b', 5.7, 'B13 = OK (EXTI13)')

# USB-TTL (trái giữa)
usb = module(1.0, 5.6, 2.6, 'Mạch USB-TTL', [('RX', None), ('TX', None), ('GND', 'GND'), ('5V/3V3', None)], 'R', '#e8e8e8',
             note='chân 5V/3V3 để trống (Blue Pill cấp nguồn riêng)')
wire(usb['RX'], board_pin('A9', 'L'), '#2ca02c', 5.2, 'A9 = USART1 TX')
wire(usb['TX'], board_pin('A10', 'L'), '#17becf', 5.7, 'A10 = USART1 RX')

# OLED (trái dưới)
oled = module(1.0, 1.6, 2.6, 'OLED SSD1306 (I2C)', [('VCC', '3V3'), ('GND', 'GND'), ('SCL', None), ('SDA', None)], 'R', '#cfe2f3',
              note='địa chỉ 0x3C, module có sẵn điện trở kéo lên')
wire(oled['SCL'], board_pin('B6', 'L'), '#e377c2', 5.2, 'B6 = I2C1 SCL')
wire(oled['SDA'], board_pin('B7', 'L'), '#1f77b4', 5.7, 'B7 = I2C1 SDA')

# MAX7219 (phải trên)
mx = module(13.2, 6.0, 3.0, 'Ma trận LED MAX7219', [('VCC', '5V'), ('GND', 'GND'), ('DIN', None), ('CS', None), ('CLK', None)], 'L', '#f4cccc',
            note='nguồn 5 V, tín hiệu 3,3 V vẫn nhận được')
wire(mx['DIN'], board_pin('A7', 'R'), '#d62728', 11.2, 'A7 = SPI1 MOSI')
wire(mx['CS'], board_pin('A4', 'R'), '#bcbd22', 11.7, 'A4 = CS')
wire(mx['CLK'], board_pin('A5', 'R'), '#ff7f0e', 12.2, 'A5 = SPI1 SCK')

# Quang trở (phải dưới)
ldr = module(13.2, 1.6, 3.0, 'Quang trở LM393', [('VCC', '3V3'), ('GND', 'GND'), ('DO', None)], 'L', '#fff2cc',
             note='DO = 1 khi tối (EXTI0)')
wire(ldr['DO'], board_pin('A0', 'R'), '#7f7f7f', 11.4, 'A0 = cảm biến (EXTI0)')

ax.text((BX0 + BX1) / 2, pin_y(19) - 0.5, 'C13: LED có sẵn trên bo, nháy 1 Hz (không cần nối)', fontsize=8.5, style='italic', va='top', ha='center')

ax.text(8.6, 11.4, 'SƠ ĐỒ CẮM CHÂN – ĐÈN THÔNG MINH TRÊN NỀN FreeRTOS (STM32F103C8)', ha='center',
        fontsize=14, fontweight='bold')
ax.text(8.6, 10.95, 'Các chân mang cùng nhãn nguồn (5V, 3V3, GND) nối chung với chân tương ứng của Blue Pill. '
        'Tất cả GND phải nối chung. Chân vàng = chân được dùng.', fontsize=9, style='italic', ha='center')

fig.tight_layout()
fig.savefig(OUT, dpi=170)
print('saved', os.path.abspath(OUT))
