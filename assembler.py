# Assemble and flash the ATtiny85 using pySerial to communicate with Arduino ISP

import re
import serial
import time
import struct

assembly_code = '''
; Blink an LED on PB3 every second
; Device: ATtiny85

.include "tn85def.inc"

.org 0x0000
    rjmp init          ; Reset vector

init:
    ldi r16, (1 << PB3)   ; Load bit mask for PB3 into r16
    out DDRB, r16         ; Set PB3 as output

loop:
    sbi PORTB, PB3        ; Set PB3 high
    rcall timer           ; Call delay subroutine
    cbi PORTB, PB3        ; Set PB3 low
    rcall timer           ; Call delay subroutine
    rjmp loop             ; Repeat loop

timer:
    ldi r16, 0            ; Initialize counters
    ldi r17, 0
    ldi r18, 5

timer2:
    inc r16               ; Increment r16
    brne timer2           ; If r16 != 0, continue inner loop
    inc r17               ; Increment r17
    brne timer2           ; If r17 != 0, continue inner loop
    dec r18               ; Decrement r18
    brne timer2           ; If r18 != 0, continue loops
    ret                   ; Return from subroutine
'''

def assemble(assembly):
    """
    Assembles the provided assembly code into machine code.
    """
    import re

    # Opcode mapping for supported instructions
    opcodes = {
        'rjmp':  0xC000,
        'ldi':   0xE000,
        'out':   0xB800,
        'sbi':   0x9A00,
        'cbi':   0x9800,
        'rcall': 0xD000,
        'dec':   0x940A,
        'inc':   0x9403,
        'brne':  0xF401,
        'ret':   0x9508
    }

    # Register mapping
    registers = {
        'r16': 16, 'r17': 17, 'r18': 18, 'r19': 19, 'r20': 20, 'r21': 21, 'r22': 22, 'r23': 23,
        'r24': 24, 'r25': 25, 'r26': 26, 'r27': 27, 'r28': 28, 'r29': 29, 'r30': 30, 'r31': 31
    }

    # I/O registers mapping for instructions
    io_registers = {
        'DDRB': 0x04,
        'PORTB': 0x05,
        'PB3': 3
    }

    # Remove directives and preprocess assembly code
    lines = assembly.strip().split('\n')
    processed_lines = []
    for line in lines:
        line = line.strip()
        if line.startswith('.include') or line.startswith('.org'):
            continue
        processed_lines.append(line)

    machine_code = []
    labels = {}
    address = 0

    # First pass: Identify labels
    for line in processed_lines:
        line_clean = line.split(';')[0].strip()  # Remove comments
        if not line_clean:
            continue
        if ':' in line_clean:
            label = line_clean.replace(':', '').strip()
            labels[label] = address
        else:
            address += 1  # Assuming each instruction is 1 word

    # Second pass: Assemble instructions
    address = 0
    for line in processed_lines:
        original_line = line
        line_clean = line.split(';')[0].strip()
        if not line_clean or ':' in line_clean:
            continue

        # Split instruction and arguments, handling commas
        parts = re.split(r'\s+', line_clean, maxsplit=1)
        instr = parts[0]
        args = []
        if len(parts) > 1:
            args_str = parts[1]
            args = [arg.strip() for arg in re.split(r',\s*', args_str)]

        try:
            if instr == 'rjmp':
                offset = labels[args[0]] - (address + 1)
                opcode = opcodes['rjmp'] | (offset & 0x0FFF)
            elif instr == 'ldi':
                rd = registers[args[0]]
                K = eval(args[1], {}, io_registers)
                if K < 0 or K > 255:
                    print(f"Error: Immediate value out of range in 'ldi' at line: {original_line}")
                    opcode = 0xFFFF
                else:
                    opcode = opcodes['ldi'] | ((K & 0xF0) << 4) | ((rd - 16) << 4) | (K & 0x0F)
            elif instr == 'out':
                io_addr = io_registers[args[0]]
                rr = registers[args[1]]
                opcode = opcodes['out'] | ((rr & 0x1F) << 4) | (io_addr & 0x3F)
            elif instr == 'sbi':
                io_addr = io_registers[args[0]]
                bit = io_registers[args[1]]
                opcode = opcodes['sbi'] | ((io_addr & 0x1F) << 3) | (bit & 0x07)
            elif instr == 'cbi':
                io_addr = io_registers[args[0]]
                bit = io_registers[args[1]]
                opcode = opcodes['cbi'] | ((io_addr & 0x1F) << 3) | (bit & 0x07)
            elif instr == 'rcall':
                offset = labels[args[0]] - (address + 1)
                opcode = opcodes['rcall'] | (offset & 0x0FFF)
            elif instr == 'dec':
                rd = registers[args[0]]
                opcode = opcodes['dec'] | ((rd & 0x1F) << 4)
            elif instr == 'inc':
                rd = registers[args[0]]
                opcode = opcodes['inc'] | ((rd & 0x1F) << 4)
            elif instr == 'brne':
                offset = labels[args[0]] - (address + 1)
                if offset < -64 or offset > 63:
                    print(f"Error: Offset out of range in 'brne' at line: {original_line}")
                    opcode = 0xFFFF
                else:
                    offset &= 0x7F  # Ensure offset is 7 bits
                    opcode = opcodes['brne'] | (offset << 3)
            elif instr == 'ret':
                opcode = opcodes['ret']
            else:
                print(f"Error: Unknown instruction '{instr}' in line: {original_line}")
                opcode = 0xFFFF  # Undefined instruction
        except Exception as e:
            print(f"Error assembling line: '{original_line}' - {e}")
            opcode = 0xFFFF

        machine_code.append((address * 2, opcode))
        address += 1

    return machine_code

def write_hex_file(machine_code, filename):
    """
    Writes the machine code to a HEX file in Intel HEX format.
    """
    with open(filename, 'w') as f:
        for addr, code in machine_code:
            byte1 = code & 0xFF
            byte2 = (code >> 8) & 0xFF
            checksum = (-(2 + (addr >> 8) + (addr & 0xFF) + byte2 + byte1)) & 0xFF
            line = ':{count:02X}{addr:04X}00{data:02X}{data2:02X}{checksum:02X}'.format(
                count=2,
                addr=addr,
                data=byte2,
                data2=byte1,
                checksum=checksum
            )
            f.write(line + '\n')
        f.write(':00000001FF\n')  # End-of-file record

def parse_hex_file(filename):
    """
    Parses an Intel HEX file and returns a dictionary of address to data bytes.
    """
    flash_data = {}
    with open(filename, 'r') as f:
        for line in f:
            if line.strip() == ':00000001FF':
                break
            if line[0] != ':':
                continue
            count = int(line[1:3], 16)
            addr = int(line[3:7], 16)
            rectype = int(line[7:9], 16)
            if rectype != 0:
                continue
            data = line[9:9+count*2]
            for i in range(0, count*2, 2):
                byte = int(data[i:i+2], 16)
                flash_data[addr] = byte
                addr += 1
    return flash_data

def flash_to_device(hex_filename, com_port):
    """
    Flashes the HEX file to the device via the Arduino ISP using STK500 protocol.
    """
    import serial
    from time import sleep

    print(f"Flashing '{hex_filename}' to ATtiny85 using Arduino ISP on {com_port}...")

    flash_data = parse_hex_file(hex_filename)
    max_address = max(flash_data.keys())

    # Open serial port
    ser = serial.Serial(com_port, 19200, timeout=1)
    sleep(2)  # Wait for Arduino reset

    def get_sync():
        for _ in range(3):
            ser.write(b'\x30' + b'\x20')
            response = ser.read(2)
            if response == b'\x14\x10':
                return True
        return False

    def enter_prog_mode():
        ser.write(b'\x50\x20')
        response = ser.read(2)
        return response == b'\x14\x10'

    def leave_prog_mode():
        ser.write(b'\x51\x20')
        response = ser.read(2)
        return response == b'\x14\x10'

    def load_address(address):
        high = (address >> 8) & 0xFF
        low = address & 0xFF
        ser.write(b'\x55' + bytes([high, low]) + b'\x20')
        response = ser.read(2)
        return response == b'\x14\x10'

    def program_page(address, data):
        length = len(data)
        ser.write(b'\x64' + bytes([0x00, length]) + b'F' + data + b'\x20')
        response = ser.read(2)
        return response == b'\x14\x10'

    def read_signature():
        ser.write(b'\x75\x20')
        response = ser.read(5)
        if response.startswith(b'\x14') and response.endswith(b'\x10'):
            sig = response[1:4]
            return sig
        return None

    # Synchronize
    if not get_sync():
        print("Failed to synchronize with the programmer.")
        ser.close()
        return

    # Enter programming mode
    if not enter_prog_mode():
        print("Failed to enter programming mode.")
        ser.close()
        return

    # Read signature
    sig = read_signature()
    if sig is None:
        print("Failed to read device signature.")
        ser.close()
        return
    else:
        print(f"Device signature: {sig.hex()}")

    # Program flash memory
    page_size = 32  # ATtiny85 page size in bytes
    current_address = 0

    while current_address <= max_address:
        page_data = bytearray()
        for i in range(page_size):
            addr = current_address + i
            if addr in flash_data:
                page_data.append(flash_data[addr])
            else:
                page_data.append(0xFF)
        if not load_address(current_address // 2):
            print(f"Failed to load address {current_address}.")
            ser.close()
            return
        if not program_page(current_address, page_data):
            print(f"Failed to program page at address {current_address}.")
            ser.close()
            return
        current_address += page_size

    # Leave programming mode
    if not leave_prog_mode():
        print("Failed to leave programming mode.")
        ser.close()
        return

    ser.close()
    print("Flashing complete.")

# Main process
if __name__ == '__main__':
    machine_code = assemble(assembly_code)
    hex_filename = 'blink.hex'
    write_hex_file(machine_code, hex_filename)
    flash_to_device(hex_filename, 'COM7')
