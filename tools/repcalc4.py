
# calculates base4 repunit^2

patterns = {
"1": {
"start": [2, 1, 1],
"header": "130",
"center": "012",
"footer": "032",
"last": "1",
},
"2": {
"start": [2, 1, 1],
"header": "130",
"center": "012",
"footer": "032",
"last": "10",
},
"3": {
"start": [1, 0, 1],
"header": "333",
"center": "222",
"footer": "000",
"last": "1",
},
}


def calc(c, i, spc=" "):
    global patterns
    p = patterns[c]
    start = p["start"]
    idx0 = i - start[0]
    s = (p["header"] * (i + 1))[:idx0] if idx0 > 0 else ""
    idx1 = i - start[1]
    s1 = p["center"][idx1 % 3] if idx1 > 0 else ""
    idx2 = i - start[2]
    s2 = (p["footer"] * (i + 1))[-idx2:] if idx2 > 0 else ""

    s = s + spc + s1 + spc + s2 + spc + p["last"]

    return s

if __name__ == "__main__":
    import sys

    if len(sys.argv) > 2:
        c = sys.argv[1]
        n = int(sys.argv[2])
        spc = " " if len(sys.argv) > 3 and sys.argv[3].startswith("y") else ""
    else:
        c = input("every column is? (1...3) >")[0]
        n = int(input("print upto? (number) >"))
        spc = " " if input("insert spaces? (y|n) >").startswith("y") else ""

    for i in range(1, n + 1):
        print(f"{i:04d}:", calc(c, i, spc))
