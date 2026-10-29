"""Sơ đồ ngữ cảnh của từng task: bên trái là thứ đi VÀO task, bên phải là thứ task ĐƯA RA.
Mỗi liên kết vẽ đúng kênh FreeRTOS (queue = dãy ô, mailbox = 1 ô, notification/semaphore/... = viên thuốc tím)
và hộp xanh nét đứt là dữ liệu cụ thể được đưa vào kênh."""
import os, sys
sys.path.insert(0, os.path.dirname(__file__))
from kit import Canvas, OWN, ISR, KER, SYN, HW

OUT = os.path.join(os.path.dirname(__file__), '..', '..')
TASKS = {'InputTask': ('Cường', 3), 'LampTask': ('Cường', 2), 'DisplayTask': ('Việt', 1),
         'LogTask': ('Việt', 1), 'ConsoleTask': ('Tiến', 2), 'MonitorTask': ('Tiến', 4)}


def endpoint(c, key, x, y, ep):
    kind, title, sub = ep
    if kind == 'task':
        own, pr = TASKS[title]; c.task(key, x, y, title, own, pr, w=2.3, h=0.72, fs=11)
    elif kind == 'isr':
        c.isr(key, x, y, title, sub, w=2.3, h=0.72, fs=10.5)
    elif kind == 'ker':
        c.block(key, x, y, 2.3, 0.72, title, sub, KER, fs=10.5)
    else:
        c.block(key, x, y, 2.3, 0.72, title, sub, HW, style='square', fs=10.5)


def channel(c, key, x, y, ch):
    kind, name, cap = ch
    if kind == 'queue':
        c.queue(key, x, y, name, cap, cells=6)
    elif kind == 'mailbox':
        c.queue(key, x, y, name, cap, mailbox=True)
    elif kind == 'sync':
        c.sync(key, x, y, name, cap, w=1.45, h=0.5, fs=10)
    else:   # 'none': nối thẳng
        c.box[key] = (x, y, 0.01, 0.01)


def draw(name, left, right, trigger, fname, STEP=1.6):
    n = max(len(left), len(right))
    H = n * STEP + 1.5
    c = Canvas(13.6, H)
    own, pr = TASKS[name]
    cy = (H - 0.9) / 2
    th = n * STEP - 0.4
    fc, ec = OWN[own]
    from matplotlib.patches import FancyBboxPatch
    c.ax.add_patch(FancyBboxPatch((5.75, cy - th / 2), 2.1, th, boxstyle='round,pad=0,rounding_size=0.15',
                                  fc=fc, ec=ec, lw=2, zorder=3))
    c.ax.text(6.8, cy + 0.2, name, ha='center', va='center', fontsize=14, fontweight='bold', rotation=0, zorder=5)
    c.ax.text(6.8, cy - 0.2, f'{own} · ưu tiên {pr}', ha='center', va='center', fontsize=10, zorder=5)
    c.box['T'] = (6.8, cy, 2.1, th)
    c.ax.text(6.8, H - 0.35, trigger, ha='center', va='center', fontsize=12, fontweight='bold', color='#2d3748',
              bbox=dict(boxstyle='round,pad=0.35', fc='#fffaf0', ec='#dd6b20', lw=1.3))
    c.note(1.2, H - 0.95, 'ĐẦU VÀO', fs=11, color='#975a16', style='normal')
    c.note(12.4, H - 0.95, 'ĐẦU RA', fs=11, color='#2f855a', style='normal')

    def ys(k):
        return [cy + (k - 1) * STEP / 2 - i * STEP for i in range(k)]

    for i, (ep, ch, data, a1, a2) in enumerate(left):
        y = ys(len(left))[i]
        endpoint(c, f'L{i}', 1.2, y, ep)
        dashed = ch[0] == 'sync'
        if ch[0] == 'none':
            c.arrow((2.37, y), (5.75, y), a1, dashed=dashed)
        else:
            channel(c, f'LC{i}', 4.0, y, ch)
            x, _, w, _ = c.box[f'LC{i}']
            c.arrow((2.37, y), (x - w / 2 - 0.05, y), a1, dashed=dashed, fs=8.5)
            c.arrow((x + w / 2 + 0.05, y), (5.75, y), a2, dashed=dashed, fs=8.5)
            if data:
                c.data(4.0, y - 0.62, data)
    for i, (ep, ch, data, a1, a2) in enumerate(right):
        y = ys(len(right))[i]
        endpoint(c, f'R{i}', 12.4, y, ep)
        dashed = ch[0] == 'sync'
        if ch[0] == 'none':
            c.arrow((7.85, y), (11.23, y), a1, dashed=dashed)
        else:
            channel(c, f'RC{i}', 9.6, y, ch)
            x, _, w, _ = c.box[f'RC{i}']
            c.arrow((7.85, y), (x - w / 2 - 0.05, y), a1, dashed=dashed, fs=8.5)
            c.arrow((x + w / 2 + 0.05, y), (11.23, y), a2, dashed=dashed, fs=8.5)
            if data:
                c.data(9.6, y - 0.62, data)
    c.save(os.path.join(OUT, fname))


Q_EV = ('queue', 'qEvents', '10 × app_event_t (4 B)')
Q_LOG = ('queue', 'qLog', '16 × log_msg_t (64 B)')
M_LV = ('mailbox', 'qLampView', 'mailbox 1 × lamp_view_t')
M_CPU = ('mailbox', 'qCpuStat', 'mailbox 1 × cpu_summary_t')
NONE = ('none', '', '')
LOGT = ('task', 'LogTask', '')
LAMPT = ('task', 'LampTask', '')

draw('InputTask',
     left=[(('isr', 'Ngắt EXTI', 'PA0, PB12, PB13'), ('sync', 'notification', ''),
            'bit 0 / 1 / 2', 'NotifyFromISR', 'NotifyWait'),
           (('ker', 'Timer chống nhiễu', '1 s / 30 ms'), ('sync', 'notification', ''),
            'bit 8 / 9 / 10', 'xTaskNotify', 'NotifyWait'),
           (('task', 'MonitorTask', ''), ('sync', 'egSystem', ''), 'bit READY', 'SetBits', 'WaitBits (1 lần)')],
     right=[(LAMPT, Q_EV, '{EV_SENSOR_DARK, 0, 0}\n{EV_BTN_OK, 0, 0}', 'app_post_event', 'xQueueReceive'),
            (('ker', 'Timer chống nhiễu', 'khởi động lại'), NONE, '', 'xTimerReset', ''),
            (LOGT, Q_LOG, '"[IN] Troi TOI (lan 3)"', 'log_printf', 'xQueueReceive')],
     trigger='Chạy khi: ngắt EXTI hoặc timer chống nhiễu đánh thức', fname='ctx_1_input_task.png')

draw('LampTask',
     left=[(('task', 'InputTask', ''), Q_EV, '{EV_SENSOR_DARK, 0, 0}\n{EV_BTN_OK, 0, 0}', 'app_post_event', 'xQueueReceive'),
           (('task', 'ConsoleTask', ''), Q_EV, '{EV_CMD_LAMP, 0, 1}\n{EV_CMD_SET, SET_MODE, 2}', 'app_post_event', 'xQueueReceive'),
           (('ker', 'tmrFade / tmrOff', 'software timer'), Q_EV, '{EV_FADE_TICK, 0, 0}\n{EV_OFF_TIMEOUT, 0, 0}', 'callback', 'xQueueReceive')],
     right=[(('task', 'DisplayTask', ''), M_LV, '{state=FADE_IN, level=5,\n dark=1, mode=WAVE, ...}', 'xQueueOverwrite', 'xQueuePeek'),
            (('ker', 'tmrFade / tmrOff', 'chạy / dừng'), NONE, '', 'xTimerReset / Stop', ''),
            (LOGT, Q_LOG, '"[LAMP] OFF -> FADE_IN"', 'log_printf', 'xQueueReceive')],
     trigger='Chạy khi: có sự kiện mới trong qEvents', fname='ctx_2_lamp_task.png')

draw('DisplayTask',
     left=[(('task', 'LampTask', ''), M_LV, '{state, level, dark,\n settings, menu}', 'xQueueOverwrite', 'xQueuePeek'),
           (('task', 'MonitorTask', ''), M_CPU, '{idle=30, rt=29,\n sleep=0, uptime=560}', 'xQueueOverwrite', 'xQueuePeek')],
     right=[(('hw', 'Ma trận LED 8×8', 'MAX7219'), NONE, '', 'SPI1 + DMA: 8 byte hàng + độ sáng', ''),
            (('hw', 'OLED 128×64', 'SSD1306'), NONE, '', 'I2C1: khung 1 KB (mỗi 200 ms)', ''),
            (LOGT, Q_LOG, '"[DISP] OLED OK"', 'log_printf', 'xQueueReceive')],
     trigger='Chạy khi: mỗi 40 ms (xTaskDelayUntil)', fname='ctx_3_display_task.png')

draw('ConsoleTask',
     left=[(('isr', 'Ngắt USART1', 'RX, gom đến Enter'), ('sync', 'notification', ''),
            'dòng "mode 2"', 'GiveFromISR', 'NotifyTake'),
           (('task', 'LampTask', ''), M_LV, 'trạng thái đèn\n(cho lệnh status)', 'xQueueOverwrite', 'xQueuePeek'),
           (('task', 'MonitorTask', ''), ('sync', 'mutex', ''), '% CPU, stack\n(cho lệnh cpu)', 'ghi', 'đọc')],
     right=[(LAMPT, Q_EV, '{EV_CMD_LAMP, 0, 1}\n{EV_CMD_SET, SET_MODE, 2}', 'app_post_event', 'xQueueReceive'),
            (('task', 'MonitorTask', ''), NONE, '', 'sleep 0/1, log 0/1', ''),
            (LOGT, Q_LOG, '"Tai CPU: idle-hook 3.0%"', 'log_printf', 'xQueueReceive')],
     trigger='Chạy khi: ngắt UART báo nhận đủ một dòng lệnh', fname='ctx_4_console_task.png')

draw('LogTask',
     left=[(('ker', 'Mọi task', 'log_printf()'), Q_LOG, '"[LAMP] OFF -> FADE_IN"\n"CPU,1240,3.0,2.9,0"', 'log_printf', 'xQueueReceive'),
           (('isr', 'Ngắt DMA1', 'kênh 4: phát xong'), ('sync', 'semaphore', ''), '', 'GiveFromISR', 'Take')],
     right=[(('hw', 'Máy tính', 'terminal (Hercules)'), NONE, '', 'USART1 TX (PA9) + DMA', '')],
     trigger='Chạy khi: có chuỗi mới trong qLog', fname='ctx_5_log_task.png')

draw('MonitorTask',
     left=[(('ker', 'Idle hook', 'đếm vòng rảnh (+ WFI)'), NONE, '', 'số lần idle', ''),
           (('isr', 'TIM2 20 kHz', 'thang thời gian'), NONE, '', 'run-time stats', '')],
     right=[(('ker', 'Các task khác', 'chờ bit READY'), ('sync', 'egSystem', ''), 'bit READY (1 lần)', 'SetBits', 'WaitBits'),
            (('task', 'DisplayTask', ''), M_CPU, '{idle=30, rt=29,\n sleep=0, uptime=560}', 'xQueueOverwrite', 'xQueuePeek'),
            (('task', 'ConsoleTask', ''), ('sync', 'mutex', ''), '% CPU, stack từng task', 'ghi', 'đọc'),
            (LOGT, Q_LOG, '"CPU,560,3.0,2.9,0"', 'log_printf', 'xQueueReceive'),
            (('hw', 'LED PC13', ''), NONE, '', 'đảo mỗi 1 s', '')],
     trigger='Chạy khi: khởi động (hiệu chuẩn 1 s), sau đó mỗi 1 s', fname='ctx_6_monitor_task.png', STEP=1.32)
print('ok')
