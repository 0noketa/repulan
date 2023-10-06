
# calculates base5 repunit^2

patterns = {
"1": {
"start": [2, 2, 2, 2],
"header": "1240",
"center0": "1230",
"center1": "2340",
"footer": "0432",
"last": "1",
},
"2": {
"start": [2, 2, 2, 1],
"header": "1111",
"center0": "1111",
"center1": "0000",
"footer": "3333",
"last": "4",
},
"3": {
"start": [2, 2, 1, 1],
"header": "2401",
"center0": "2301",
"center1": "1240",
"footer": "3204",
"last": "4",
},
"4": {
"start": [2, 2, 1, 1],
"header": "4444",
"center0": "4444",
"center1": "3333",
"footer": "0000",
"last": "1",
},
}


def calc(c, i, spc=" "):
    global patterns
    p = patterns[c]
    start = p["start"]
    idx0 = i - start[0]
    s = (p["header"] * ((i + 2) // 4))[:idx0] if idx0 >= 0 else ""
    idx1 = i - start[1]
    s1 = p["center0"][idx1 % 4] if idx1 >= 0 else ""
    idx2 = i - start[2]
    s2 = p["center1"][idx2 % 4] if idx2 >= 0 else ""
    idx3 = i - start[3]
    s3 = (p["footer"] * ((i + 2) // 4))
    s3 = s3[len(s3) - idx3:] if idx3 >= 0 else ""

    s = s + spc + s1 + spc + s2 + spc + s3 + spc + p["last"]

    return s

if __name__ == "__main__":
    import sys

    if len(sys.argv) > 2:
        c = sys.argv[1]
        n = int(sys.argv[2])
        spc = " " if len(sys.argv) > 3 and sys.argv[3].startswith("y") else ""
    else:
        c = input("every column is? (1...4) >")[0]
        n = int(input("print upto? (number) >"))
        spc = " " if input("insert spaces? (y|n) >").startswith("y") else ""

    for i in range(1, n + 1):
        print(f"{i:04d}:", calc(c, i, spc))
