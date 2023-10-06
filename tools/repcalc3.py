
# calculates base3 repunit^2  (stub)

patterns = {
"1": {
"header": "20",
"center": "12",
"footer": "02",
"last": "1",
},
"2": {
"header": "2",
"center": "1",
"footer": "0",
"last": "1",
},
}


def calc(c, i, spc=" "):
    global patterns
    p = patterns[c]
    s = ""
    s1 = ""

    if c == "1":
        n = (i - 1) // 2
        s = p["header"] * n
        s1 = p["footer"] * n

        if i > 0 and i % 2 == 0:
            s2 = p["center"]
        else:
            s2 = ""
    else:  # if c == "1":
        n = (i - 1)
        s = p["header"] * n
        s1 = p["footer"] * n

        s2 = p["center"]
        

    s = s + spc + s2 + spc + s1 + spc + p["last"]

    return s

if __name__ == "__main__":
    import sys

    if len(sys.argv) > 2:
        c = sys.argv[1]
        n = int(sys.argv[2])
        spc = " " if len(sys.argv) > 3 and sys.argv[3].startswith("y") else ""
    else:
        c = input("every column is? (1...2) >")[0]
        n = int(input("print upto? (number) >"))
        spc = " " if input("insert spaces? (y|n) >").startswith("y") else ""


    for i in range(1, n + 1):
        print(f"{i:04d}:", calc(c, i, spc))
