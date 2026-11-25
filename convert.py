#!/usr/bin/env python3

#
# Converts the can_data.txt files into C++ headers for this project!
#

"""
can_data.txt -> github.com/rnd-ash/mb-w211-pc/blob/d442caf/can_spec_canb.txt

Usage: convert.py <can_data.txt>

Layout rules:
  * Byte order Little Endian
  * MSB first
  * gaps are filled with padding
  * signals spanning several whole bytes stay one field (needs byteswap)
  * signals crossing byte boundaries unaligned are split per byte
    (NAME_p0 = least significant part) and re-assembled in an "extra" field

Files created:
  * can_data.h
  * main.cpp
"""
import os
import re
import struct
import sys
from dataclasses import dataclass, field


@dataclass
class Sig:
    name: str
    desc: str
    offset: int
    length: int
    dtype: str = "RAW"
    mult: float = 1.0
    off: float = 0.0
    enums: list = field(default_factory=list)  # (name, raw, desc)


@dataclass
class Frame:
    name: str
    can_id: int
    sigs: list = field(default_factory=list)
    group: str = "CAN Frames"


@dataclass
class Item:
    start: int
    bits: int
    name: object      # None = padding
    sig: object
    comment: str


SIG_RE = re.compile(r"SIGNAL (\S+?),\s*OFFSET: (\d+), LEN: (\d+), DESC: (.*?)(?:, DATA TYPE (.*))?$")
ENUM_RE = re.compile(r"ENUM (.+?),\s*RAW: (-?\d+), DESC: (.*)$")
FRAME_RE = re.compile(r"FRAME (\S+)\s*\((\w+)\)")


def f32_short(x):
    """The txt holds float32 values widened to double (0.10000000149011612);
    return the shortest decimal that still maps to the same float32 (0.1)."""
    f = struct.unpack("f", struct.pack("f", x))[0]
    for n in range(1, 10):
        y = float(f"{f:.{n}g}")
        if struct.unpack("f", struct.pack("f", y))[0] == f:
            return y
    return x


def parse(path):
    ecus = {}
    frames = frame = sig = None
    group = "CAN Frames"
    for line in open(path, encoding="utf-8"):
        l = line.strip()
        g = re.match(r"#\s*GROUP\s+(.+)", l)
        if g:  # optional marker: "# GROUP Motor CAN-C" starts a new group
            group = g[1].strip()
        if not l or l.startswith("#"):
            continue
        if l.startswith("ECU "):
            frames = ecus.setdefault(l[4:].strip(), [])
        elif l.startswith("FRAME "):
            m = FRAME_RE.match(l)
            frame = Frame(m[1], int(m[2], 0), group=group)
            frames.append(frame)
        elif l.startswith("SIGNAL "):
            m = SIG_RE.match(l)
            sig = Sig(m[1], m[4].strip(), int(m[2]), int(m[3]))
            dt = (m[5] or "RAW").strip()
            sig.dtype = dt.split("(")[0].strip()
            if sig.dtype == "NUMBER":
                a = re.search(r"_MULTIPLIER_:\s*([-+\d.eE]+)", dt)
                b = re.search(r"_OFFSET_:\s*([-+\d.eE]+)", dt)
                sig.mult = f32_short(float(a[1])) if a else 1.0
                sig.off = f32_short(float(b[1])) if b else 0.0
            frame.sigs.append(sig)
        elif l.startswith("ENUM "):
            m = ENUM_RE.match(l)
            sig.enums.append((m[1], int(m[2]), m[3].strip()))
    return ecus


def cname(n):
    if n.strip().lower() == "nicht definiert":
        return "NDEF"
    return re.sub(r"\W", "_", n.strip())


def ctype(bits):
    return "uint8_t" if bits <= 8 else "uint16_t" if bits <= 16 else "uint32_t" if bits <= 32 else "uint64_t"


def lower_first(d):
    return d[0].lower() + d[1:] if len(d) > 1 and d[1].islower() else d


def desc(s):
    d = lower_first(s.desc)
    if s.dtype == "NUMBER":
        d += f" (multiplier: {s.mult}, addend: {s.off})"
    return f"DESC: {s.name} - {d}, OFFSET: {s.offset}, LEN: {s.length}"


def split_bytes(s):
    res, pos, end = [], s.offset, s.offset + s.length
    while pos < end:
        n = min(end, (pos // 8 + 1) * 8) - pos
        res.append((pos, n))
        pos += n
    return res  # MSB first


def pad(a, b):
    out = []
    while a < b:
        if a % 8 == 0 and b - a >= 8:
            n = min((b - a) // 8 * 8, 64)
        else:
            n = min(b, (a // 8 + 1) * 8) - a
        out.append(Item(a, n, None, None, f"Padding, OFFSET: {a}, LEN: {n}"))
        a += n
    return out


def enum_block(s, bits):
    t, tn = ctype(bits), f"{s.name}_c"
    o = ["/*", f"  enum class {tn} : {t} {{".ljust(39) + f"// {desc(s)}"]
    for n, r, d in s.enums:
        n = cname(n)
        o.append(f"    {n:<30}= {r}, // enum: {r} - {d} / {n}")
    o += [f"  }} : {bits}; {tn} {s.name};", "*/"]
    return o


def field_lines(it):
    t = "bool" if (it.bits == 1 and (it.name is None or (it.sig and it.sig.dtype == "BOOL"))) else ctype(it.bits)
    out = [f"  {t} {it.name or ''}".ljust(34) + f": {it.bits}; // {it.comment}"]
    if it.sig and it.sig.dtype == "ENUM" and it.sig.enums:
        out += enum_block(it.sig, it.bits)
    return out


def label(b0, b1):
    return f"// byte {b0}" if b0 == b1 else f"// byte {b0}-{b1}"


def gen_frame(ecu, fr):
    items, extras, post, floats = [], [], [], []
    for s in sorted(fr.sigs, key=lambda s: s.offset):
        if s.dtype == "NUMBER" and (s.mult, s.off) != (1.0, 0.0):
            floats.append(s)
        ch = split_bytes(s)
        if len(ch) == 1 or (s.offset % 8 == 0 and s.length % 8 == 0):
            items.append(Item(s.offset, s.length, s.name, s, desc(s)))
            if s.length > 8 and s.dtype != "ISO_TP":  # ISO-TP payload is raw bytes, no swap
                w = int(ctype(s.length)[4:-2])
                e = f"__builtin_bswap{w}({fr.name}.{s.name})" + (f" >> {w - s.length}" if w != s.length else "")
                post.append([f"{fr.name}.{s.name} = {e};"])
        else:
            n, shift = len(ch), 0
            for i in range(n):
                st, ln = ch[n - 1 - i]
                items.append(Item(st, ln, f"{s.name}_p{i}", None,
                                  f"DESC: {s.name} - {lower_first(s.desc)} (part {i}), OFFSET: {st}, LEN: {ln}"))
                shift += ln
            extras.append(s)

    # fill gaps
    items.sort(key=lambda i: i.start)
    filled, pos = [], 0
    for it in items:
        if it.start < pos:
            print(f"WARNING: overlap in {fr.name} at {it.name} (offset {it.start})", file=sys.stderr)
        elif it.start > pos:
            filled += pad(pos, it.start)
        filled.append(it)
        pos = max(pos, it.start + it.bits)
    if pos % 8:
        filled += pad(pos, pos + (-pos % 8))
        pos += -pos % 8
    frame_bits = pos
    filled.sort(key=lambda i: (i.start // 8, -i.start))

    out = [f"// ECU: {ecu}, NAME: {fr.name}, ID: 0x{fr.can_id:04X}, MSG COUNT: {len(fr.sigs)}",
           f"typedef struct {fr.name}_t {{"]
    last = None
    for it in filled:
        lab = (it.start // 8, (it.start + it.bits - 1) // 8)
        if lab != last:
            out.append(label(*lab))
            last = lab
        out += field_lines(it)
    out.append("// <- End of CAN Frame")

    # extra (re-assembled) fields
    pos, last = frame_bits, None
    if extras or floats:
        out += ["", "// extra bytes -> must set extern"]
    if extras:
        for s in extras:
            n = len(split_bytes(s))
            parts = " + ".join(f"{s.name}_p{i}" for i in range(n))
            lab = (pos // 8, (pos + s.length - 1) // 8)
            if lab != last:
                out.append(label(*lab))
                last = lab
            it = Item(pos, s.length, s.name, s,
                      f"DESC: {s.name} - {lower_first(s.desc)}, (concatenated {parts})")
            out += field_lines(it)
            shift, stmt = 0, [f"{fr.name}.{s.name} ="]
            lens = [ln for _, ln in reversed(split_bytes(s))]
            for i, ln in enumerate(lens):
                e = f"{fr.name}.{s.name}_p{i}" + (f" << {shift}" if shift else "")
                end = ";" if i == len(lens) - 1 else " |"
                stmt.append(f"  ({e}){end} // {lower_first(s.desc)} (part {i})")
                shift += ln
            post.append(stmt)
            pos += s.length
        if pos % 8:
            out += pad(pos, pos + (-pos % 8)) and \
                [f"  uint8_t".ljust(34) + f": {-pos % 8}; // Padding, OFFSET: {pos}, LEN: {-pos % 8}"]
    for s in floats:  # physical value = raw * multiplier + addend
        out.append(f"  float {s.name}_f".ljust(34) + f"; // DESC: {s.name}_f - {lower_first(s.desc)}, "
                   f"physical value = {s.name} * {s.mult} {'+' if s.off >= 0 else '-'} {abs(s.off)}")
        e = f"{fr.name}.{s.name}" + (f" * {s.mult}f" if s.mult != 1.0 else "")
        if s.off:
            e += f" {'+' if s.off > 0 else '-'} {abs(s.off)}f"
        post.append([f"{fr.name}.{s.name}_f = {e};"])
    out.append(f"}} __attribute__((__packed__)) {fr.name}_t;")
    return "\n".join(out), out[0], post


def gen_all(entries):
    return ("// AUTOGENERATED by convert.py\n"
            "#pragma once\n\n"
            + "\n\n".join(e["text"] for e in entries) + "\n")


def aligned(pairs):
    w = max(len(a) for a, _ in pairs)
    return [f"{a.ljust(w)} {b}" for a, b in pairs]


def gen_main(hname, entries):
    groups = {}
    for i, e in enumerate(entries):
        groups.setdefault(e["fr"].group, []).append(i)
    out = [f'#include "{hname}"\n']
    # defines, one block per group
    blocks = []
    for idx in groups.values():
        blocks.append("\n".join(aligned([(f"#define CANID_{entries[i]['fr'].name} 0x{entries[i]['fr'].can_id:04X}", "// " + entries[i]["hdr"])
                                         for i in idx])))
    out.append("\n\n".join(blocks))
    out.append("""
// copy CAN message into bitfield decoder
#define copyMsg(ptr) importMsg(#ptr, ptr, sizeof(*(ptr)), id, msg, len)
""")
    for grp, idx in groups.items():
        out.append(f"// {grp}")
        out.append("\n".join(aligned([(f"struct {entries[i]['fr'].name}_t {entries[i]['fr'].name};",
                                       "// " + entries[i]["hdr"]) for i in idx])))
        out.append("")
    out.append("""SemaphoreHandle_t canMutex;


// import CAN message into bitfield decoder
void importMsg(const char* name, void* dest, size_t destSize, unsigned int id, const uint8_t* msg, uint8_t len) {
  if (len <= destSize) {
    xSemaphoreTake(canMutex, portMAX_DELAY);
    memcpy(dest, msg, len);
    xSemaphoreGive(canMutex);
  } else {
    Serial.printf("WARNING: CANID 0x%04X (%s): frame too long, struct size is %d bytes\\r\\n", id, name, (int)destSize);
    delay(100);
  }
}

// export CAN message into bitfield decoder
void exportMsg(unsigned int id, const uint8_t *msg, uint8_t len) {
  switch(id) {""")
    copies = aligned([(f"copyMsg(&{e['fr'].name});", "// " + e["hdr"]) for e in entries])
    for i, e in enumerate(entries):
        out.append(f"    case CANID_{e['fr'].name}:\n      {copies[i]}")
        for stmt in e["post"]:
            out.append("      " + stmt[0])
            out += ["      " + x for x in stmt[1:]]
        out.append("      break;")
    out.append("""  }
}

void clearCan(unsigned long timeout_ms) {
  static unsigned long now_ms = 0;
  static long long last_flushed = -1;
  if (millis() - now_ms > timeout_ms) {
    now_ms = millis();
    xSemaphoreTake(can0_stats.mutex, portMAX_DELAY);
    long long msg_time = can0_stats.last_msg_time;
    xSemaphoreGive(can0_stats.mutex);
    const bool offline = (esp_timer_get_time() - msg_time) > (long long)timeout_ms * 1000LL || msg_time == 0;
    if (offline && msg_time != last_flushed) {
      xSemaphoreTake(canMutex, portMAX_DELAY);""")
    out += ["      " + l for l in aligned([(f"memset(&{e['fr'].name}, 0, sizeof({e['fr'].name}));", "// " + e["hdr"])
                                          for e in entries])]
    out.append("""      xSemaphoreGive(canMutex);
      last_flushed = msg_time;
    }
  }
}
""")
    return "\n".join(out)


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    src = sys.argv[1]
    base = os.path.splitext(src)[0]
    hpath, mpath = base + ".h", os.path.join(os.path.dirname(os.path.abspath(src)), "main.cpp")
    entries, seen, ids = [], {}, {}
    sig_of = lambda fr: [(x.name, x.offset, x.length, x.dtype, x.mult, x.off, x.enums) for x in fr.sigs]
    for ecu, frames in parse(src).items():
        for fr in frames:
            if fr.name in seen:  # same frame listed under several ECUs (sender/receiver): keep the first
                if sig_of(seen[fr.name]) != sig_of(fr):
                    print(f"WARNING: {fr.name} under ECU {ecu} differs from first definition, skipped", file=sys.stderr)
                continue
            seen[fr.name] = fr
            if fr.can_id in ids:
                print(f"WARNING: CAN ID 0x{fr.can_id:04X} used by {ids[fr.can_id]} and {fr.name} (duplicate case)", file=sys.stderr)
            ids.setdefault(fr.can_id, fr.name)
            if fr.sigs:
                text, hdr, post = gen_frame(ecu, fr)
                entries.append(dict(fr=fr, text=text, hdr=hdr[3:], post=post))
    with open(hpath, "w", encoding="utf-8") as f:
        f.write(gen_all(entries))
    with open(mpath, "w", encoding="utf-8") as f:
        f.write(gen_main(os.path.basename(hpath), entries))
    print(f"{hpath}, {mpath}: {len(entries)} frames")


if __name__ == "__main__":
    main()
