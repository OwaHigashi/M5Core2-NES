import ctypes, ctypes.wintypes, time, sys, struct

GENERIC_READ = 0x80000000
GENERIC_WRITE = 0x40000000
OPEN_EXISTING = 3

handle = ctypes.windll.kernel32.CreateFileW(
    r'\\.\COM11',
    GENERIC_READ | GENERIC_WRITE,
    0, None, OPEN_EXISTING, 0, None
)
err = ctypes.windll.kernel32.GetLastError()
print(f'Handle: {handle}, Error: {err}', flush=True)

if handle == -1 or handle == 0xFFFFFFFF:
    print('Failed to open COM11')
    sys.exit(1)

# Set baud rate via DCB
class DCB(ctypes.Structure):
    _fields_ = [
        ('DCBlength', ctypes.c_uint32),
        ('BaudRate', ctypes.c_uint32),
        ('flags', ctypes.c_uint32),
        ('wReserved', ctypes.c_uint16),
        ('XonLim', ctypes.c_uint16),
        ('XoffLim', ctypes.c_uint16),
        ('ByteSize', ctypes.c_uint8),
        ('Parity', ctypes.c_uint8),
        ('StopBits', ctypes.c_uint8),
        ('XonChar', ctypes.c_char),
        ('XoffChar', ctypes.c_char),
        ('ErrorChar', ctypes.c_char),
        ('EofChar', ctypes.c_char),
        ('EvtChar', ctypes.c_char),
        ('wReserved1', ctypes.c_uint16),
    ]

dcb = DCB()
dcb.DCBlength = ctypes.sizeof(DCB)
if ctypes.windll.kernel32.GetCommState(handle, ctypes.byref(dcb)):
    print(f'Current baud: {dcb.BaudRate}', flush=True)
    dcb.BaudRate = 115200
    dcb.ByteSize = 8
    dcb.Parity = 0
    dcb.StopBits = 0
    result = ctypes.windll.kernel32.SetCommState(handle, ctypes.byref(dcb))
    print(f'SetCommState: {result}, err: {ctypes.windll.kernel32.GetLastError()}', flush=True)
else:
    print(f'GetCommState failed: {ctypes.windll.kernel32.GetLastError()}', flush=True)
    print('Trying BuildCommDCBW...', flush=True)
    dcb2 = DCB()
    dcb2.DCBlength = ctypes.sizeof(DCB)
    ctypes.windll.kernel32.BuildCommDCBW('baud=115200 parity=N data=8 stop=1', ctypes.byref(dcb2))
    ctypes.windll.kernel32.SetCommState(handle, ctypes.byref(dcb2))

# Set timeouts
class COMMTIMEOUTS(ctypes.Structure):
    _fields_ = [
        ('ReadIntervalTimeout', ctypes.c_uint32),
        ('ReadTotalTimeoutMultiplier', ctypes.c_uint32),
        ('ReadTotalTimeoutConstant', ctypes.c_uint32),
        ('WriteTotalTimeoutMultiplier', ctypes.c_uint32),
        ('WriteTotalTimeoutConstant', ctypes.c_uint32),
    ]

timeouts = COMMTIMEOUTS()
timeouts.ReadIntervalTimeout = 50
timeouts.ReadTotalTimeoutMultiplier = 0
timeouts.ReadTotalTimeoutConstant = 500
ctypes.windll.kernel32.SetCommTimeouts(handle, ctypes.byref(timeouts))

# Toggle DTR to reset
ctypes.windll.kernel32.EscapeCommFunction(handle, 6)  # CLRDTR
time.sleep(0.1)
ctypes.windll.kernel32.EscapeCommFunction(handle, 5)  # SETDTR
time.sleep(0.1)

# Toggle RTS to reset ESP32
ctypes.windll.kernel32.EscapeCommFunction(handle, 4)  # CLRRTS
time.sleep(0.1)
ctypes.windll.kernel32.EscapeCommFunction(handle, 3)  # SETRTS
time.sleep(0.5)

print('--- Reading serial output ---', flush=True)

buf = ctypes.create_string_buffer(4096)
bytes_read = ctypes.c_uint32(0)

start = time.time()
while time.time() - start < 20:
    result = ctypes.windll.kernel32.ReadFile(
        handle, buf, 4096, ctypes.byref(bytes_read), None
    )
    if bytes_read.value > 0:
        text = buf.raw[:bytes_read.value].decode('utf-8', errors='replace')
        print(text, end='', flush=True)
    time.sleep(0.05)

ctypes.windll.kernel32.CloseHandle(handle)
print('\n--- Done ---', flush=True)
