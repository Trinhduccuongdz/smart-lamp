"""Sơ đồ task, ngắt và các đối tượng FreeRTOS (vẽ lại cho dễ đọc).
Chạy: python3 docs/src/task_ipc.py  ->  docs/2_task_ipc.png

Bố cục theo 4 luồng (mỗi luồng một hàng), đọc từ trái sang phải:
  1. Luồng điều khiển đèn : ngắt EXTI -> InputTask -> qEvents -> LampTask -> qLampView -> DisplayTask
  2. Luồng lệnh máy tính   : ngắt USART1 -> ConsoleTask -> qEvents
  3. Luồng giám sát CPU    : Idle hook, ngắt TIM2 -> MonitorTask -> qCpuStat / mutex / egSystem
  4. Luồng log             : mọi task -> qLog -> LogTask -> DMA -> ngắt DMA -> semaphore
"""
import os
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.patches import FancyBboxPatch, FancyArrowPatch

plt.rcParams['font.family'] = 'DejaVu Sans'
OUT = os.path.join(os.path.dirname(__file__), '..', '2_task_ipc.png')

C = {
    'p1': '#c6f6d5', 'p2': '#bee3f8', 'p3': '#fed7e2', 'isr': '#fefcbf', 'kern': '#e2e8f0',
    'queue': '#ffffff', 'sync': '#efe5fb',
}
EDGE = {'p1': '#2f855a', 'p2': '#2b6cb0', 'p3': '#b83280', 'isr': '#975a16', 'kern': '#4a5568',
        'queue': '#2d3748', 'sync': '#6b46c1'}

fig, ax = plt.subplots(figsize=(19, 12.5))
ax.set_xlim(-0.6, 19.4); ax.set_ylim(-1.5, 12.6); ax.axis('off')

W, H = 2.7, 1.0
nodes = {}


def node(key, x, y, title, sub, kind, w=W, h=H):
    """Vẽ một khối; (x, y) là tâm. kind quyết định màu và hình dạng."""
    style = 'round,pad=0,rounding_size=0.18'
    if kind == 'isr':
        style = 'square,pad=0'
    if kind == 'sync':
        style = 'round,pad=0,rounding_size=0.45'
    lw = 2.4 if kind == 'queue' else 1.6
    ax.add_patch(FancyBboxPatch((x - w / 2, y - h / 2), w, h, boxstyle=style,
                                fc=C[kind], ec=EDGE[kind], lw=lw, zorder=3))
    if kind == 'queue':   # vạch đôi bên trái: ký hiệu hàng đợi
        ax.plot([x - w / 2 + 0.12] * 2, [y - h / 2 + 0.08, y + h / 2 - 0.08], color=EDGE[kind], lw=1.4, zorder=4)
    ax.text(x, y + (0.17 if sub else 0), title, ha='center', va='center', fontsize=13.5, fontweight='bold', zorder=5)
    if sub:
        ax.text(x, y - 0.24, sub, ha='center', va='center', fontsize=10.5, zorder=5, color='#2d3748')
    nodes[key] = (x, y, w, h)


def port(key, side, off=0.0):
    x, y, w, h = nodes[key]
    return {'l': (x - w / 2, y + off), 'r': (x + w / 2, y + off),
            't': (x + off, y + h / 2), 'b': (x + off, y - h / 2)}[side]


def arrow(a, b, label='', dashed=False, color='#1a202c', rad=0.0, lpos=0.5, loff=(0, 0.16), fs=10.5):
    p = FancyArrowPatch(a, b, arrowstyle='-|>', mutation_scale=18, lw=1.8, color=color,
                        linestyle=(0, (5, 3)) if dashed else '-', connectionstyle=f'arc3,rad={rad}', zorder=2,
                        shrinkA=2, shrinkB=2)
    ax.add_patch(p)
    if label:
        x = a[0] + (b[0] - a[0]) * lpos + loff[0]
        y = a[1] + (b[1] - a[1]) * lpos + loff[1]
        ax.text(x, y, label, ha='center', va='bottom', fontsize=fs, color=color, zorder=6,
                bbox=dict(boxstyle='round,pad=0.12', fc='white', ec='none', alpha=0.9))


def lane(y, text):
    ax.plot([-0.5, 19.3], [y, y], color='#cbd5e0', lw=1, ls=(0, (2, 3)), zorder=0)
    ax.text(-0.5, y + 0.1, text, fontsize=11, color='#718096', style='italic', va='bottom')


# ---------------- Hàng 1: điều khiển đèn ----------------
Y1 = 10.2
lane(11.25, '① Luồng điều khiển đèn')
node('exti', 0.9, Y1, 'Ngắt EXTI', 'cảm biến PA0, nút PB12/13', 'isr', w=2.6)
node('in', 4.4, Y1, 'InputTask', 'Cường · prio 3', 'p1')
node('qev', 8.0, Y1, 'qEvents', 'queue 10 sự kiện', 'queue')
node('lamp', 11.6, Y1, 'LampTask', 'Cường · prio 2', 'p1')
node('qview', 15.0, Y1, 'qLampView', 'mailbox', 'queue', w=2.4)
node('disp', 18.0, Y1, 'DisplayTask', 'Việt · prio 1', 'p2', w=2.5)
node('tmr', 5.9, 8.05, 'Timer daemon', 'chống dội, tmrFade, tmrOff', 'kern', w=3.0)

arrow(port('exti', 'r'), port('in', 'l'), 'notification')
arrow(port('in', 'r'), port('qev', 'l'), 'DARK / LIGHT / BTN')
arrow(port('qev', 'r'), port('lamp', 'l'), 'xQueueReceive')
arrow(port('lamp', 'r'), port('qview', 'l'), 'overwrite')
arrow(port('qview', 'r'), port('disp', 'l'), 'peek')
arrow(port('in', 'b', -0.7), port('tmr', 'l', 0.15), 'xTimerReset', dashed=True, color='#4a5568', rad=0.3, lpos=0.5, loff=(-1.0, -0.1))
arrow(port('tmr', 't', -0.6), port('in', 'b', 0.3), 'ổn định', dashed=True, color='#4a5568', rad=0.0, lpos=0.5, loff=(0.45, -0.1))
arrow(port('lamp', 'b', -0.6), port('tmr', 'r', -0.1), 'start / stop', dashed=True, color='#4a5568', rad=-0.15, lpos=0.3, loff=(0.6, -0.15))
arrow(port('tmr', 't', 0.7), port('qev', 'b', -0.6), 'FADE_TICK\nOFF_TIMEOUT', color='#4a5568', lpos=0.5, loff=(-0.05, -0.25), fs=9.5)
ax.text(18.0, Y1 + 0.6, 'ma trận LED (SPI), OLED (I2C)', ha='center', va='bottom', fontsize=10, color='#2b6cb0')

# ---------------- Hàng 2: lệnh từ máy tính ----------------
Y2 = 6.0
lane(7.05, '② Luồng lệnh từ máy tính')
node('rx', 0.9, Y2, 'Ngắt USART1', 'RXNE, gom 1 dòng', 'isr', w=2.6)
node('con', 11.6, Y2, 'ConsoleTask', 'Tiến · prio 2', 'p3')
arrow(port('rx', 'r'), port('con', 'l'), 'vTaskNotifyGiveFromISR (đủ 1 dòng lệnh)', lpos=0.5)
arrow(port('con', 't', -0.8), port('qev', 'b', 0.7), 'CMD_LAMP / CMD_SET', rad=0.0, lpos=0.4, loff=(0.05, 0))
arrow(port('qview', 'b'), port('con', 'r', 0.2), 'peek (lệnh status)', rad=-0.2, lpos=0.5, loff=(0.9, -0.1))

# ---------------- Hàng 3: giám sát CPU ----------------
Y3 = 3.3
lane(4.45, '③ Luồng giám sát tải CPU')
node('tim2', 0.9, Y3 + 0.6, 'Ngắt TIM2', 'thang đo 20 kHz', 'isr', w=2.6, h=0.95)
node('idle', 0.9, Y3 - 0.65, 'Idle task', 'idle hook: đếm + WFI', 'kern', w=2.6, h=0.95)
node('mon', 4.4, Y3, 'MonitorTask', 'Tiến · prio 4 · 1 s', 'p3')
node('mtx', 11.6, Y3 + 0.05, 'mutex', 'bảng thống kê CPU', 'sync', w=2.6)
node('qcpu', 15.0, 1.95, 'qCpuStat', 'mailbox', 'queue', w=2.4)
node('eg', 4.4, 1.55, 'egSystem', 'event group: bit READY', 'sync', w=3.0, h=0.9)
arrow(port('tim2', 'r'), port('mon', 'l', 0.3), 'bộ đếm 32 bit', fs=10)
arrow(port('idle', 'r'), port('mon', 'l', -0.3), 'số lần idle', fs=10)
arrow(port('mon', 'r'), port('mtx', 'l'), 'ghi bảng thống kê', lpos=0.5)
arrow(port('mtx', 't'), port('con', 'b'), 'đọc (lệnh cpu)', lpos=0.5, loff=(0.85, -0.15))
arrow(port('mon', 'r', -0.35), port('qcpu', 'l'), 'overwrite', rad=0.0, lpos=0.5, loff=(0, 0.02))
arrow(port('qcpu', 'r'), port('disp', 'b'), 'peek', rad=0.25, lpos=0.5, loff=(0.55, 0))
arrow(port('mon', 'b'), port('eg', 't'), 'đặt READY sau hiệu chuẩn', dashed=True, color='#6b46c1', lpos=0.5, loff=(1.25, -0.2), fs=10)
ax.text(6.05, 1.35, 'các task khác chờ bit READY rồi mới chạy\n(xEventGroupWaitBits)', fontsize=10,
        va='center', color='#6b46c1', style='italic')

# ---------------- Hàng 4: log ----------------
Y4 = -0.15
lane(0.85, '④ Luồng log ra UART')
node('all', 0.9, Y4, 'Mọi task', 'log_printf()', 'kern', w=2.6)
node('qlog', 4.4, Y4, 'qLog', 'queue 16 dòng', 'queue')
node('log', 8.0, Y4, 'LogTask', 'Việt · prio 1', 'p2')
node('dma', 11.6, Y4, 'Ngắt DMA1_CH4', 'phát UART xong', 'isr')
node('sem', 15.0, Y4, 'semaphore', 'binary: TX xong', 'sync', w=2.4)
arrow(port('all', 'r'), port('qlog', 'l'), 'xQueueSend')
arrow(port('qlog', 'r'), port('log', 'l'), 'xQueueReceive')
arrow(port('log', 'r'), port('dma', 'l'), 'khởi động DMA')
arrow(port('dma', 'r'), port('sem', 'l'), 'GiveFromISR', dashed=True, color='#6b46c1')
arrow(port('sem', 'b', -0.3), port('log', 'b', 0.3), 'xSemaphoreTake: chờ phát xong', dashed=True, color='#6b46c1',
      rad=-0.12, lpos=0.5, loff=(0, -0.72))

# ---------------- Chú thích ----------------
ax.text(9.4, 12.35, 'SƠ ĐỒ TASK, NGẮT VÀ CÁC ĐỐI TƯỢNG FreeRTOS', ha='center', fontsize=17, fontweight='bold')
leg = [('p1', 'Cường'), ('p2', 'Việt'), ('p3', 'Tiến'), ('kern', 'task của kernel'),
       ('isr', 'ngắt (ISR)'), ('queue', 'queue / mailbox'), ('sync', 'mutex / semaphore / event group')]
lx = 1.3
for k, t in leg:
    ax.add_patch(FancyBboxPatch((lx, 11.72), 0.42, 0.3, boxstyle='round,pad=0,rounding_size=0.05',
                                fc=C[k], ec=EDGE[k], lw=1.4))
    ax.text(lx + 0.52, 11.87, t, va='center', fontsize=10.5)
    lx += 0.85 + len(t) * 0.095
ax.plot([lx + 0.1, lx + 0.7], [11.87, 11.87], color='#1a202c', lw=1.8)
ax.text(lx + 0.8, 11.87, 'dữ liệu', va='center', fontsize=10.5)
ax.plot([lx + 1.75, lx + 2.35], [11.87, 11.87], color='#4a5568', lw=1.8, ls=(0, (5, 3)))
ax.text(lx + 2.45, 11.87, 'báo hiệu / đồng bộ', va='center', fontsize=10.5)

fig.tight_layout()
fig.savefig(OUT, dpi=170)
print('saved', os.path.abspath(OUT))
