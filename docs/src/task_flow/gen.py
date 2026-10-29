import subprocess, pathlib
common=open('common.dot.in').read()
S='shape=oval, fillcolor="#2b6cb0", fontcolor=white, color="#2b6cb0", group=t'
IN='shape=box, style=filled, fillcolor="#fefcbf", color="#975a16", margin="0.1,0.05", io=1'
OUT='shape=box, style=filled, fillcolor="#c6f6d5", color="#2f855a", margin="0.1,0.05", io=1'
D='shape=diamond, fillcolor="#fff5f5", color="#c53030", margin="0.03,0.02"'
W='fillcolor="#edf2f7", color="#4a5568"'
C='shape=circle, style=filled, fillcolor=white, color="#2d3748", fontsize=13, width=0.38, fixedsize=true, label="A"'
T='group=t'
charts={}
charts['1_input_task']=f'''
s [label="InputTask (ưu tiên 3)", {S}];
r [label="Chờ bit READY", {W}, {T}];
i [label="Khởi tạo cảm biến, 2 nút\\n(ngắt EXTI 2 cạnh)", {T}];
o0 [label="Đầu ra: trạng thái ban đầu\\nEV_SENSOR_DARK/LIGHT\\n→ qEvents", {OUT}, {T}];
ca [{C}];
w [label="Chờ task notification\\n(xTaskNotifyWait)", {W}, {T}];
in1 [label="Đầu vào: bit CẠNH từ ISR,\\nbit ỔN ĐỊNH từ timer\\nchống nhiễu", {IN}, {T}];
rt [label="Có bit CẠNH thì reset timer\\nchống nhiễu (1 s / 30 ms)", {T}];
d2 [label="Có bit\\nổn định?", {D}, {T}];
d3 [label="Nguồn là\\ncảm biến?", {D}, {T}];
rs [label="Đọc mức\\ncảm biến"];
d4 [label="Khác trạng\\nthái cũ?", {D}];
o1 [label="Đầu ra:\\nEV_SENSOR_DARK\\nhoặc _LIGHT → qEvents", {OUT}];
rb [label="Đọc mức nút"];
d5 [label="Nhấn\\nhay nhả?", {D}];
pt [label="Lưu thời\\nđiểm nhấn"];
cls [label="Phân loại theo thời\\ngian giữ: ≥ 0,8 s là\\nGIỮ, ngược lại NGẮN"];
o2 [label="Đầu ra: EV_BTN_xxx\\nhoặc _HOLD → qEvents", {OUT}];
ce [{C}, {T}];
s -> r -> i -> o0 -> w -> in1 -> rt -> d2;
ca -> w; {{rank=same; ca; w}}
d2 -> d3 [label=" Có"];
d2 -> ce [label=" Không"];
d3 -> rs [label=" Có"]; rs -> d4;
d4 -> o1 [label=" Có"];
d4 -> ce [label=" Không\\n (nhiễu)"];
d3 -> rb [label=" Nút"]; rb -> d5;
d5 -> pt [label=" Nhấn"];
d5 -> cls [label=" Nhả"]; cls -> o2;
o1 -> ce; pt -> ce; o2 -> ce;
{{rank=same; rs; rb}}
'''
charts['2_lamp_task']=f'''
s [label="LampTask (ưu tiên 2)", {S}];
r [label="Chờ bit READY", {W}, {T}];
p0 [label="Công bố trạng thái\\nban đầu → qLampView", {T}];
ca [{C}];
w [label="Chờ sự kiện trong qEvents\\n(xQueueReceive)", {W}, {T}];
in1 [label="Đầu vào: sự kiện từ InputTask,\\nConsoleTask, timer sáng dần\\n(100 ms), timer hẹn tắt", {IN}, {T}];
d1 [label="Loại sự\\nkiện?", {D}, {T}];
m [label="Máy trạng thái menu\\n(CLOSED / NAV / EDIT)"];
d2 [label="Lưu cài đặt\\nhoặc bật/tắt?", {D}];
set [label="Áp dụng cài đặt\\n(mode, độ sáng,\\nhẹn giờ, auto)"];
fsm [label="Máy trạng thái đèn\\nOFF / FADE_IN /\\nON / FADE_OUT", {T}];
act [label="Chạy / dừng timer sáng dần,\\nchạy / dừng timer hẹn tắt", {T}];
d3 [label="Trạng thái\\nđèn đổi?", {D}, {T}];
lg [label="Đầu ra: log\\n\\"[LAMP] A -> B\\" → qLog", {OUT}];
o1 [label="Đầu ra: trạng thái, mức sáng,\\ncài đặt, menu → qLampView", {OUT}, {T}];
ce [{C}, {T}];
s -> r -> p0 -> w -> in1 -> d1;
ca -> w; {{rank=same; ca; w}}
d1 -> fsm [label=" Cảm biến, nhịp sáng dần,\\n hết giờ, lệnh on/off"];
d1 -> m [label=" Nút nhấn"];
d1 -> set [label=" Lệnh cài đặt"];
m -> d2; d2 -> fsm [label=" Có"]; d2 -> o1 [label=" Không"];
set -> fsm;
fsm -> act -> d3;
d3 -> lg [label=" Có"]; lg -> o1;
d3 -> o1 [label=" Không"];
o1 -> ce;
'''
charts['3_display_task']=f'''
s [label="DisplayTask (ưu tiên 1)", {S}];
r [label="Chờ bit READY", {W}, {T}];
i [label="Khởi tạo MAX7219 (SPI1)\\nvà OLED (I2C1)", {T}];
o0 [label="Đầu ra: log OLED\\nOK / lỗi → qLog", {OUT}, {T}];
ca [{C}];
in1 [label="Đầu vào: đọc qLampView\\n(xQueuePeek)", {IN}, {T}];
d1 [label="Mức sáng\\n= 0?", {D}, {T}];
sd [label="Tắt ma trận\\n(shutdown)"];
fr [label="Tính khung hình theo\\nhiệu ứng và độ sáng", {T}];
d2 [label="Khung / độ\\nsáng đổi?", {D}, {T}];
spi [label="Đầu ra: khung hình\\n→ ma trận LED\\n(SPI + DMA)", {OUT}];
d3 [label="Đủ 5 khung\\n(200 ms)?", {D}, {T}];
in2 [label="Đầu vào: đọc qCpuStat\\n(xQueuePeek)", {IN}, {T}];
d4 [label="Menu\\nđang mở?", {D}, {T}];
pm [label="Vẽ trang\\nmenu cài đặt"];
ps [label="Vẽ trang trạng thái\\n(đèn, ánh sáng, CPU)"];
oled [label="Đầu ra: nội dung\\n→ OLED (I2C)", {OUT}, {T}];
wt [label="frame + 1, chờ hết\\nchu kỳ 40 ms\\n(xTaskDelayUntil)", {W}, {T}];
ce [{C}, {T}];
s -> r -> i -> o0 -> in1 -> d1;
ca -> in1; {{rank=same; ca; in1}}
d1 -> fr [label=" Không"]; d1 -> sd [label=" Có"];
fr -> d2; d2 -> spi [label=" Có"];
d2 -> d3 [label=" Không"]; spi -> d3; sd -> d3;
d3 -> in2 [label=" Có"]; d3 -> wt [label=" Không"];
in2 -> d4; d4 -> ps [label=" Không"]; d4 -> pm [label=" Có"];
pm -> oled; ps -> oled; oled -> wt -> ce;
{{rank=same; sd; fr}}
'''
charts['4_console_task']=f'''
s [label="ConsoleTask (ưu tiên 2)", {S}];
r [label="Chờ bit READY", {W}, {T}];
o0 [label="Đầu ra: lời chào → qLog", {OUT}, {T}];
ca [{C}];
w [label="Chờ notification từ ISR\\nUSART (ulTaskNotifyTake)", {W}, {T}];
in1 [label="Đầu vào: dòng lệnh từ\\nmáy tính (UART RX)", {IN}, {T}];
cb [{C}, label="B"];
d1 [label="Còn dòng\\ntrong bộ đệm?", {D}, {T}];
sp [label="Tách tên lệnh\\nvà tham số", {T}];
d2 [label="Loại lệnh?", {D}, {T}];
o1 [label="Đầu ra:\\nEV_CMD_xxx\\n→ qEvents", {OUT}];
rd [label="Đọc trạng thái\\nđèn / CPU"];
o2 [label="Đầu ra: kết quả\\n→ qLog", {OUT}];
cf [label="Bật / tắt\\nsleep, log 1"];
o3 [label="Đầu ra:\\nbáo lỗi\\n→ qLog", {OUT}];
ce [{C}, {T}];
cb2 [{C}, label="B"];
s -> r -> o0 -> w -> in1 -> d1;
ca -> w; {{rank=same; ca; w}}
cb -> d1; {{rank=same; cb; d1}}
d1 -> sp [label=" Có"]; d1 -> ce [label=" Không"];
sp -> d2;
d2 -> o1 [label=" điều\\n khiển"];
d2 -> rd [label=" xem"]; rd -> o2;
d2 -> cf [label=" cấu\\n hình"];
d2 -> o3 [label=" sai"];
o1 -> cb2; o2 -> cb2; cf -> cb2; o3 -> cb2;
{{rank=same; o1; rd; cf; o3}} {{rank=same; ce; cb2}}
'''
charts['5_log_task']=f'''
s [label="LogTask (ưu tiên 1)", {S}];
r [label="Chờ bit READY", {W}, {T}];
ca [{C}];
w [label="Chờ chuỗi trong qLog\\n(xQueueReceive)", {W}, {T}];
in1 [label="Đầu vào: chuỗi log\\ntừ các task (log_printf)", {IN}, {T}];
dma [label="Gửi chuỗi qua\\nUSART1 TX bằng DMA", {T}];
sem [label="Chờ binary semaphore\\n(ngắt DMA báo xong)", {W}, {T}];
o1 [label="Đầu ra: chuỗi hiện\\ntrên terminal máy tính", {OUT}, {T}];
ce [{C}, {T}];
s -> r -> w -> in1 -> dma -> sem -> o1 -> ce;
ca -> w; {{rank=same; ca; w}}
'''
charts['6_monitor_task']=f'''
s [label="MonitorTask (ưu tiên 4)", {S}];
c1 [label="Bắt đầu hiệu chuẩn:\\nxóa bộ đếm idle", {T}];
c2 [label="Ngủ 1 s (task khác chờ\\nREADY → chỉ Idle chạy)", {W}, {T}];
c3 [label="Kết thúc hiệu chuẩn:\\nidle_max (lần/s)", {T}];
o0 [label="Đầu ra: log idle_max\\n→ qLog, đặt bit READY", {OUT}, {T}];
ca [{C}];
w [label="Chờ hết chu kỳ 1 s\\n(xTaskDelayUntil)", {W}, {T}];
in1 [label="Đầu vào: bộ đếm idle hook,\\nthời gian chạy từng task\\n(TIM2)", {IN}, {T}];
calc [label="Tính tải CPU 2 phương pháp,\\n% CPU, stack trống từng task", {T}];
o1 [label="Đầu ra: thống kê CPU\\n→ qCpuStat", {OUT}, {T}];
d1 [label="Đang bật\\nlog 1?", {D}, {T}];
o2 [label="Đầu ra: dòng CSV\\n→ qLog", {OUT}];
led [label="Đầu ra: đảo LED PC13\\n(báo hệ thống còn chạy)", {OUT}, {T}];
ce [{C}, {T}];
s -> c1 -> c2 -> c3 -> o0 -> w -> in1 -> calc -> o1 -> d1;
ca -> w; {{rank=same; ca; w}}
d1 -> o2 [label=" Có"]; o2 -> led;
d1 -> led [label=" Không"];
led -> ce;
'''
import re
def finalize(src):
    plain=subprocess.run(['dot','-Tplain'],input=src.encode(),capture_output=True,check=True).stdout.decode()
    sizes={}
    for line in plain.splitlines():
        p=line.split(' ')
        if p[0]=='node': sizes[p[1]]=(float(p[4]),float(p[5]))
    ios=re.findall(r'^(\w+) \[[^\n]*io=1',src,flags=re.M)
    extra=''.join(f'{n} [shape=polygon, sides=4, skew=0.22, fixedsize=true, width={sizes[n][0]*1.28:.2f}, height={sizes[n][1]*1.1:.2f}];\n' for n in ios)
    return src.replace(', io=1','').rstrip().rstrip('}')+extra+'}\n'
for name,body in charts.items():
    src=finalize('digraph g {\n'+common+body+'\n}\n')
    pathlib.Path(name+'.dot').write_text(src)
    subprocess.run(['dot','-Tpng','-Gdpi=200','-o',f'../../flow_{name}.png',name+'.dot'],check=True)
print('ok')
