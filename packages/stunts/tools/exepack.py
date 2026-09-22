"""Unpack a Microsoft EXEPACK-compressed DOS executable."""
import struct


def unpack(raw):
    hdr = struct.unpack('<H', raw[8:10])[0] * 16
    ip, cs = struct.unpack('<HH', raw[20:24])
    stub = hdr + cs * 16
    (real_ip, real_cs, mem_start, exepack_size, real_sp, real_ss,
     dest_len, skip_len) = struct.unpack('<8H', raw[stub:stub + 16])
    sig = raw[stub + 16:stub + 18]
    packed = bytearray(raw[hdr:stub])
    # The packer pads the tail with 0xFF up to a paragraph boundary.
    end = len(packed)
    while end > 0 and packed[end - 1] == 0xFF:
        end -= 1
    out = bytearray(dest_len * 16)
    dst = len(out)
    src = end
    while src > 0:
        cmd = packed[src - 1]; src -= 1
        count = packed[src - 1] << 8 | packed[src - 2]; src -= 2
        kind = cmd & 0xFE
        if kind == 0xB0:            # fill with one byte
            val = packed[src - 1]; src -= 1
            dst -= count
            out[dst:dst + count] = bytes([val]) * count
        elif kind == 0xB2:          # copy a block
            src -= count
            dst -= count
            out[dst:dst + count] = packed[src:src + count]
        else:
            raise ValueError('bad command %02x at %d' % (cmd, src))
        if cmd & 1:
            break
    return {'image': bytes(out), 'sig': sig, 'entry_cs': real_cs,
            'entry_ip': real_ip, 'dest_len': dest_len, 'skip_len': skip_len,
            'stub_at': stub, 'filled_from': dst}
