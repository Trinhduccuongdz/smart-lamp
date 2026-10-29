"""Sơ đồ khối phần mềm tổng thể: task, ngắt và kênh FreeRTOS kèm dữ liệu đưa vào từng kênh."""
import os, sys
sys.path.insert(0, os.path.dirname(__file__))
from kit import Canvas, KER, HW, SYN
from matplotlib.patches import FancyArrowPatch
OUT = os.path.join(os.path.dirname(__file__), '..', '..', 'sw_block_diagram.png')
c = Canvas(20, 14.0)
ax = c.ax
def elbow(a, b, label='', dashed=False, first='h', lab_at=None, fs=9):
    cs = 'angle,angleA=0,angleB=90,rad=0' if first == 'h' else 'angle,angleA=90,angleB=0,rad=0'
    ax.add_patch(FancyArrowPatch(a, b, arrowstyle='-|>', mutation_scale=15, lw=1.5, color='#1a202c',
                                 linestyle=(0, (4, 3)) if dashed else '-', connectionstyle=cs, shrinkA=2, shrinkB=2, zorder=2))
    if label and lab_at:
        ax.text(*lab_at, label, ha='center', va='center', fontsize=fs, zorder=6,
                bbox=dict(boxstyle='round,pad=0.1', fc='white', ec='none'))

c.title(10, 13.65, 'SƠ ĐỒ KHỐI PHẦN MỀM: TASK, NGẮT VÀ KÊNH GIAO TIẾP', fs=16)
# chú giải
lx = 1.0
for kind, t in (('task', 'task'), ('isr', 'ngắt (ISR)'), ('q', 'queue (dãy ô)'), ('m', 'mailbox (1 ô)'), ('s', 'notification / semaphore /\nmutex / event group'), ('d', 'dữ liệu đưa vào kênh')):
    if kind == 'task': c.block('lg1', lx, 13.05, 0.5, 0.32, '', '', ('#c6f6d5', '#2f855a'))
    if kind == 'isr': c.isr('lg2', lx, 13.05, '', '', w=0.5, h=0.32)
    if kind == 'q': c.queue('lg3', lx, 13.05, '', '', cells=4, cw=0.12, ch=0.26)
    if kind == 'm': c.queue('lg4', lx, 13.05, '', '', mailbox=True, ch=0.26)
    if kind == 's': c.sync('lg5', lx, 13.05, '', '', w=0.55, h=0.3)
    if kind == 'd': c.data(lx, 13.05, ' {...} ', fs=8)
    ax.text(lx + 0.4, 13.05, t, va='center', fontsize=10)
    lx += 3.2

Y1, Y2, Y3, Y4 = 10.4, 7.2, 4.4, 1.7
# ---- hàng 1: điều khiển đèn ----
c.isr('exti', 1.2, Y1, 'Ngắt EXTI', 'PA0, PB12, PB13', w=1.9)
c.sync('n1', 3.2, Y1, 'notification', '', w=1.4, h=0.48, fs=9.5)
c.task('in', 5.3, Y1, 'InputTask', 'Cường', 3, w=1.9)
c.queue('qev', 7.9, Y1, 'qEvents', '', cells=6)
c.task('lamp', 10.5, Y1, 'LampTask', 'Cường', 2, w=1.9)
c.queue('qlv', 13.1, Y1, 'qLampView', '', mailbox=True)
c.task('disp', 15.7, Y1, 'DisplayTask', 'Việt', 1, w=1.9)
c.block('mx', 18.6, Y1 + 0.55, 2.0, 0.62, 'Ma trận LED', 'MAX7219 · SPI1', HW, style='square', fs=10.5)
c.block('ol', 18.6, Y1 - 0.55, 2.0, 0.62, 'OLED', 'SSD1306 · I2C1', HW, style='square', fs=10.5)
c.arrow(c.p('exti', 'r'), c.p('n1', 'l'), dashed=True)
c.arrow(c.p('n1', 'r'), c.p('in', 'l'), dashed=True)
c.arrow(c.p('in', 'r'), c.p('qev', 'l'), 'app_post_event', fs=8.5)
c.arrow(c.p('qev', 'r'), c.p('lamp', 'l'), 'xQueueReceive', fs=8.5)
c.arrow(c.p('lamp', 'r'), c.p('qlv', 'l'), 'Overwrite', fs=8.5)
c.arrow(c.p('qlv', 'r'), c.p('disp', 'l'), 'Peek', fs=8.5)
c.arrow(c.p('disp', 'r', 0.2), c.p('mx', 'l'))
c.arrow(c.p('disp', 'r', -0.2), c.p('ol', 'l'))
c.data(3.2, Y1 + 0.75, 'bit cạnh 0/1/2', fs=8.5)
c.data(7.9, Y1 + 1.3, '10 × app_event_t, ví dụ:\n{EV_SENSOR_DARK, 0, 0}\n{EV_BTN_OK, 0, 0}')
c.data(13.1, Y1 + 1.3, '1 × lamp_view_t, ví dụ:\n{state=ON, level=16,\n mode=WAVE, menu, ...}')
# timer daemon dưới LampTask, đổ sự kiện vào qEvents
c.block('tmr', 10.5, Y1 - 1.65, 2.0, 0.62, 'Software timer', 'tmrFade, tmrOff', KER, fs=10.5)
c.arrow(c.p('lamp', 'b'), c.p('tmr', 't'), 'start/stop', dashed=True, fs=8.5, loff=(0.45, -0.05), va='center')
elbow(c.p('tmr', 'l'), (8.12, Y1 - 0.19), first='h')
c.data(9.05, Y1 - 2.25, '{EV_FADE_TICK, 0, 0}', fs=8.3)
# ---- hàng 2: lệnh máy tính ----
c.isr('uart', 1.2, Y2, 'Ngắt USART1', 'RX: đủ 1 dòng', w=1.9)
c.sync('n2', 3.2, Y2, 'notification', '', w=1.4, h=0.48, fs=9.5)
c.task('con', 5.3, Y2, 'ConsoleTask', 'Tiến', 2, w=1.9)
c.arrow(c.p('uart', 'r'), c.p('n2', 'l'), dashed=True)
c.arrow(c.p('n2', 'r'), c.p('con', 'l'), dashed=True)
c.data(3.2, Y2 + 0.72, 'dòng "mode 2"', fs=8.5)
elbow((6.25, Y2 + 0.12), (7.68, Y1 - 0.19), first='h')
c.data(7.0, Y1 - 1.65, '{EV_CMD_SET,\n SET_MODE, 2}', fs=8.3)
elbow(c.p('qlv', 'b'), (6.25, Y2 - 0.18), first='v')
ax.text(13.1, Y2 - 0.18 + 0.22, 'Peek (lệnh status)', ha='center', fontsize=8.5, bbox=dict(boxstyle='round,pad=0.1', fc='white', ec='none'))
# ---- hàng 3: giám sát CPU ----
c.sync('mtx', 5.3, (Y2 + Y3) / 2, 'mutex', '', w=1.2, h=0.46, fs=9.5)
c.task('mon', 5.3, Y3, 'MonitorTask', 'Tiến', 4, w=1.9)
c.arrow(c.p('mon', 't'), c.p('mtx', 'b'), dashed=True)
c.arrow(c.p('mtx', 't'), c.p('con', 'b'), dashed=True)
c.data(6.85, (Y2 + Y3) / 2, 'bảng % CPU, stack', fs=8.3)
c.block('idle', 1.2, Y3 + 0.5, 1.9, 0.6, 'Idle hook', 'đếm vòng rảnh', KER, fs=10)
c.isr('tim2', 1.2, Y3 - 0.5, 'Ngắt TIM2', '20 kHz', w=1.9, h=0.6, fs=10)
c.arrow(c.p('idle', 'r'), c.p('mon', 'l', 0.15))
c.arrow(c.p('tim2', 'r'), c.p('mon', 'l', -0.15))
c.queue('qcpu', 13.1, Y3, 'qCpuStat', 'mailbox cpu_summary_t', mailbox=True)
c.arrow(c.p('mon', 'r'), c.p('qcpu', 'l'), 'xQueueOverwrite (mỗi 1 s)', fs=8.5, lpos=0.5)
elbow(c.p('qcpu', 'r'), c.p('disp', 'b'), first='h')
ax.text(15.7, (Y1 + Y3) / 2, 'Peek', ha='center', fontsize=8.5, bbox=dict(boxstyle='round,pad=0.1', fc='white', ec='none'))
c.data(13.1, Y3 - 0.72, '{idle=30, rt=29,\n sleep=0, uptime=560}', fs=8.3)
c.sync('eg', 9.0, Y3 - 1.25, 'egSystem', '', w=1.3, h=0.46, fs=9.5)
elbow(c.p('mon', 'b'), c.p('eg', 'l'), dashed=True, first='v')
ax.text(10.05, Y3 - 1.25, '→ mọi task chờ bit READY rồi mới chạy', ha='left', va='center', fontsize=9, style='italic', color='#553c9a')
# ---- hàng 4: log ----
c.block('all', 1.2, Y4, 1.9, 0.66, 'Mọi task', 'log_printf()', KER, fs=10.5)
c.queue('qlog', 3.9, Y4, 'qLog', '16 × log_msg_t (64 B)', cells=6)
c.task('log', 7.0, Y4, 'LogTask', 'Việt', 1, w=1.9)
c.block('dmahw', 10.5, Y4, 2.2, 0.66, 'DMA1 kênh 4', '→ USART1 TX (PA9)', HW, style='square', fs=10.5)
c.block('pc', 18.6, Y4, 2.0, 0.66, 'Máy tính', 'terminal', HW, style='square', fs=10.5)
c.isr('dma', 10.5, Y4 - 1.3, 'Ngắt DMA1_CH4', 'phát xong', w=2.0, h=0.62, fs=10)
c.sync('sem', 7.0, Y4 - 1.3, 'semaphore', '', w=1.4, h=0.46, fs=9.5)
c.arrow(c.p('all', 'r'), c.p('qlog', 'l'), 'xQueueSend', fs=8.5)
c.arrow(c.p('qlog', 'r'), c.p('log', 'l'), 'Receive', fs=8.5)
c.arrow(c.p('log', 'r'), c.p('dmahw', 'l'), 'khởi động DMA', fs=8.5)
c.arrow(c.p('dmahw', 'r'), c.p('pc', 'l'), 'chuỗi ký tự', fs=8.5)
c.arrow(c.p('dmahw', 'b'), c.p('dma', 't'))
c.arrow(c.p('dma', 'l'), c.p('sem', 'r'), 'GiveFromISR', dashed=True, fs=8.5)
c.arrow(c.p('sem', 't'), c.p('log', 'b'), dashed=True)
ax.text(7.15, Y4 - 0.63, 'Take', ha='left', va='center', fontsize=8.5, color='#1a202c')
c.data(3.9, Y4 - 0.68, '"[LAMP] OFF -> FADE_IN"', fs=8.3)
c.save(OUT)
print('ok')
