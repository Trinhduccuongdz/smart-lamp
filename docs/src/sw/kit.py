"""Bộ vẽ dùng chung: khối task, ngắt, queue (ô xếp hàng), mailbox, đối tượng đồng bộ, hộp dữ liệu."""
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.patches import FancyBboxPatch, FancyArrowPatch, Rectangle
plt.rcParams['font.family'] = 'DejaVu Sans'

OWN = {'Cường': ('#c6f6d5', '#2f855a'), 'Việt': ('#bee3f8', '#2b6cb0'), 'Tiến': ('#fed7e2', '#b83280')}
ISR = ('#fefcbf', '#975a16'); KER = ('#e2e8f0', '#4a5568'); SYN = ('#efe5fb', '#6b46c1'); HW = ('#ffffff', '#2d3748')
INK = '#1a202c'


class Canvas:
    def __init__(self, w, h, dpi=200):
        self.fig, self.ax = plt.subplots(figsize=(w, h), dpi=dpi)
        self.ax.set_xlim(0, w); self.ax.set_ylim(0, h); self.ax.axis('off')
        self.box = {}

    # ---------- khối ----------
    def block(self, key, x, y, w, h, title, sub='', colors=KER, style='round', fs=12, bold=True):
        bs = {'round': 'round,pad=0,rounding_size=0.12', 'square': 'square,pad=0',
              'pill': f'round,pad=0,rounding_size={h/2:.2f}'}[style]
        fc, ec = colors
        self.ax.add_patch(FancyBboxPatch((x - w/2, y - h/2), w, h, boxstyle=bs, fc=fc, ec=ec, lw=1.6, zorder=3))
        dy = 0.13 if sub else 0
        self.ax.text(x, y + dy, title, ha='center', va='center', fontsize=fs, fontweight='bold' if bold else 'normal', zorder=5, color=INK)
        if sub:
            self.ax.text(x, y - 0.17, sub, ha='center', va='center', fontsize=fs - 3, zorder=5, color='#2d3748')
        self.box[key] = (x, y, w, h)

    def task(self, key, x, y, name, owner, prio, w=1.9, h=0.75, fs=12):
        self.block(key, x, y, w, h, name, f'{owner} · ưu tiên {prio}', OWN[owner], fs=fs)

    def isr(self, key, x, y, title, sub='', w=1.9, h=0.75, fs=11):
        self.block(key, x, y, w, h, title, sub, ISR, style='square', fs=fs)

    def sync(self, key, x, y, title, sub='', w=1.7, h=0.6, fs=10.5):
        self.block(key, x, y, w, h, title, sub, SYN, style='pill', fs=fs)

    def queue(self, key, x, y, name, cap, cells=5, cw=0.17, ch=0.34, fs=10.5, mailbox=False, cap_top=False):
        """Queue: dãy ô (đầu vào bên trái, lấy ra bên phải). Mailbox: một ô duy nhất."""
        n = 1 if mailbox else cells
        w = n * cw if not mailbox else 0.42
        x0 = x - w/2
        for i in range(n):
            cwid = w / n
            fill = '#fff' if mailbox else ('#cbd5e0' if i >= n - 2 else '#fff')
            self.ax.add_patch(Rectangle((x0 + i*cwid, y - ch/2), cwid, ch, fc=fill, ec=INK, lw=1.3, zorder=4))
        if cap_top:
            self.ax.text(x, y + ch/2 + 0.3, name, ha='center', va='bottom', fontsize=fs, fontweight='bold', zorder=5, color=INK)
            self.ax.text(x, y + ch/2 + 0.06, cap, ha='center', va='bottom', fontsize=fs - 2.5, zorder=5, color='#4a5568')
        else:
            self.ax.text(x, y + ch/2 + 0.1, name, ha='center', va='bottom', fontsize=fs, fontweight='bold', zorder=5, color=INK)
            self.ax.text(x, y - ch/2 - 0.06, cap, ha='center', va='top', fontsize=fs - 2.5, zorder=5, color='#4a5568')
        self.box[key] = (x, y, w, ch)

    def data(self, x, y, text, fs=8.6, ha='center'):
        """Hộp dữ liệu đưa vào kênh (chữ đơn cách, viền đứt)."""
        self.ax.text(x, y, text, ha=ha, va='center', fontsize=fs, family='DejaVu Sans Mono', zorder=6, color='#22543d',
                     linespacing=1.25, bbox=dict(boxstyle='round,pad=0.25', fc='#f0fff4', ec='#68d391', ls=(0, (3, 2)), lw=1))

    def note(self, x, y, text, fs=9, color='#4a5568', ha='center', style='italic'):
        self.ax.text(x, y, text, ha=ha, va='center', fontsize=fs, color=color, style=style, zorder=6)

    # ---------- nối ----------
    def p(self, key, side, off=0.0):
        x, y, w, h = self.box[key]
        return {'l': (x - w/2, y + off), 'r': (x + w/2, y + off), 't': (x + off, y + h/2), 'b': (x + off, y - h/2)}[side]

    def arrow(self, a, b, label='', dashed=False, color=INK, rad=0.0, lpos=0.5, loff=(0, 0.11), fs=9, va='bottom'):
        self.ax.add_patch(FancyArrowPatch(a, b, arrowstyle='-|>', mutation_scale=15, lw=1.5, color=color,
                                          linestyle=(0, (4, 3)) if dashed else '-', connectionstyle=f'arc3,rad={rad}',
                                          shrinkA=2, shrinkB=2, zorder=2))
        if label:
            lx = a[0] + (b[0] - a[0]) * lpos + loff[0]; ly = a[1] + (b[1] - a[1]) * lpos + loff[1]
            self.ax.text(lx, ly, label, ha='center', va=va, fontsize=fs, color=color, zorder=6,
                         bbox=dict(boxstyle='round,pad=0.1', fc='white', ec='none', alpha=0.9))

    def title(self, x, y, text, fs=15):
        self.ax.text(x, y, text, ha='center', va='center', fontsize=fs, fontweight='bold', color=INK)

    def save(self, path):
        self.fig.savefig(path, bbox_inches='tight', pad_inches=0.12, facecolor='white')
        plt.close(self.fig)
