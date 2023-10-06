
# calculates base10 repunit^2  (stub)

patterns = {
"1": {
"noises": ["8", "", "",  "", "", "",  "", "", ""],
"header": "123456790",
"center": "",
"footer": "098765432",
"last": "1",
},
"2": {
"noises": ["5", "", "",  "", "", "",  "", "", ""],
"header": "493827160",
"center": "",
"footer": "395061728",
"last": "4",
},

"3": {
"noises": ["", "", "",  "", "", "",  "", "", ""],
"header": "111111111",
"center": "000000000",
"footer": "888888888",
"last": "9",
},
"4": {
"noises": ["", "", "",  "", "", "",  "", "", ""],
"header": "197530864",
"center": "319742086",
"footer": "580246913",
"last": "6",
},
"5": {
"noises": ["", "", "",  "", "", "",  "", "", ""],
"header": "308641975",
"center": "420853196",
"footer": "469135802",
"last": "5",
},
"6": {
"noises": ["", "", "",  "", "", "",  "", "", ""],
"header": "444444444",
"center": "333333333",
"footer": "555555555",
"last": "6",
},
"7": {
"noises": ["", "", "5",  "", "", "",  "", "", ""],
"header": "604938271",
"center": "049382715",
"footer": "839506172",
"last": "9",
},
"8": {
"noises": ["", "", "",  "8", "89", "",  "", "", ""],
"header": "790123456",
"center": "567890124",
"footer": "320987654",
"last": "4",
},
"9": {
"noises": ["", "", "",  "", "", "",  "", "", ""],
"header": "999999999",
"center": "888888888",
"footer": "000000000",
"last": "1",
},
}


def calc(c, i, spc=" "):
    global patterns
    p = patterns[c]
    s = ""
    s1 = ""

    n = i // 9 + 1
    s = p["header"] * n
    s1 = p["footer"] * n


    if len(p["center"]):
        rest = i % 9
        s = s[:-(9 - rest)]
        s1 = s1[(9 - rest + 1):]

        noise = p["noises"][i % 9]
        s2 = noise + p["center"][i % 9]

        s = s[:-len(s2)] + spc + s2 + spc + s1 + spc + p["last"]
    else:
        rest = i % 9
        s = s[:i - 1]
        s1 = s1[len(s1) - i + 1:]

        noise = p["noises"][i % 9]
        s2 = noise
        
        s = s[:len(s) - len(s2)] + spc + s2 + spc + s1 + spc + p["last"]

    return s

if __name__ == "__main__":
    import sys

    if len(sys.argv) > 2:
        c = sys.argv[1]
        n = int(sys.argv[2])
        spc = " " if len(sys.argv) > 3 and sys.argv[3].startswith("y") else ""
    else:
        c = input("every column is? (1...9) >")[0]
        n = int(input("print upto? (number) >"))
        spc = " " if input("insert spaces? (y|n) >").startswith("y") else ""


    for i in range(1, n + 1):
        print(f"{i:04d}:", calc(c, i, spc))
