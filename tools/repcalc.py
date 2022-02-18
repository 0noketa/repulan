
# calculates base16 repunit^2

patterns = {
"1": {
"entry": ["1", "12", "123", "1234"],
"header": "123456789ABCDF0",
"center": "56789ABCDEF",
"footer": "0FEDCBA98765432",
"last": "1",
},
"2": {
"entry": ["4", "48", "48C", "48D0"],
"header": "48D159E26AF37C0",
"center": "59D16AE27BF",
"footer": "3FB72EA61D950C8",
"last": "4",
},
"3": {
"entry": ["9", "A2", "A3C", "A3D5"],
"header": "A3D70A3D70A3D70",
"center": "F82C5F82C5F",
"footer": "8F5C28F5C28F5C2",
"last": "9",
},
"4": {
"entry": ["1", "12", "123", "1234"],
"header": "123456789ABCDF0",
"center": "56789ABCDEF",
"footer": "FEDCBA987654321",
"last": "0",
},
"5": {
"entry": ["1", "1C", "1C6", "1C71"],
"header": "1C71C71C71C71C7",
"center": "C61C61C61C6",
"footer": "8E38E38E38E38E3",
"last": "9",
},
"6": {
"entry": ["2", "28", "28F", "28F5"],
"header": "28F5C28F5C28F5C",
"center": "B28F5B28F5B",
"footer": "3D70A3D70A3D70A",
"last": "4",
},
"7": {
"entry": ["3", "37", "37B", "37BF"],
"header": "37C048D159E26AF",
"center": "48C059D16AE",
"footer": "0C83FB72EA61D95",
"last": "1",
},
"8": {
"entry": ["4", "48", "48C", "48D0"],
"header": "48D159E26AF37C0",
"center": "59D16AE27BF",
"footer": "FB72EA61D950C84",
"last": "0",
},
"9": {
"entry": ["5", "5B", "5C1", "5C28"],
"header": "5C28F5C28F5C28F",
"center": "E5B18E5B18E",
"footer": "0A3D70A3D70A3D7",
"last": "1",
},
"A": {
"entry": ["6", "70", "71B", "71C6"],
"header": "71C71C71C71C71C",
"center": "0B60B60B60B",
"footer": "38E38E38E38E38E",
"last": "4",
},
"B": {
"entry": ["7", "88", "899", "89AA"],
"header": "89ABCDF01234567",
"center": "BCDE0123456",
"footer": "87654320FEDCBA9",
"last": "9",
},
"C": {
"entry": ["9", "A2", "A3C", "A3D5"],
"header": "A3D70A3D70A3D70",
"center": "F82C5F82C5F",
"footer": "F5C28F5C28F5C29",
"last": "0",
},
"D": {
"entry": ["A", "BE", "C03", "C047"],
"header": "C048D159E26AF37",
"center": "BF48C059D16",
"footer": "83FB72EA61D950C",
"last": "9",
},
"E": {
"entry": ["C", "DD", "DEE", "DEFF"],
"header": "DF0123456789ABC",
"center": "012345678AB",
"footer": "320FEDCBA987654",
"last": "4",
},
"F": {
"entry": ["E", "FE", "FFE", "FFFE"],
"header": "FFFFFFFFFFFFFFF",
"center": "EEEEEEEEEEE",
"footer": "000000000000000",
"last": "1",
},
}


def calc(c, i, spc=" "):
    global patterns
    p = patterns[c]
    s = ""
    s1 = ""

    if i >= 15:
        n = int(i / 15)
        s = (p["header"] + spc) * n
        s1 = (p["footer"] + spc) * n

    rest = i % 15

    if rest < len(p["entry"]):
        s += p["entry"][rest] + spc
    else:
        s += p["header"][:rest] + spc

        if len(p["entry"]) % 2 == 0:
            s += p["center"][rest - len(p["entry"])] + spc

    s += p["footer"][-rest:] + spc if rest > 0 else ""
    s += s1
    s += p["last"]

    return s

if __name__ == "__main__":
    import sys

    if len(sys.argv) > 2:
        c = sys.argv[1]
        n = int(sys.argv[2])
        spc = " " if len(sys.argv) > 3 and sys.argv[3].startswith("y") else ""
    else:
        c = input("every column is? (1...F) >")[0]
        n = int(input("print upto? (number) >"))
        spc = " " if input("insert spaces? (y|n) >").startswith("y") else ""

    for i in range(n):
        print(calc(c, i, spc))
