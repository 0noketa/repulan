
# allocation-log reader

x = []
any_wrong_free = False

try:
    while True:
        src = input().strip()
        if src == "":
            continue

        v = src.split()[0]
        if v.startswith("+"):
            if v[1:] in x:
                print(f"whet? {v[1:]}")
            x.append(v[1:])
        elif v.startswith("-"):
            if v[1:] not in x:
                any_wrong_free = True
                print(f"double free? {v[1:]}")
            x.remove(v[1:])
        else:
            pass
            # print(f"? {src}")
except:
    pass

if any_wrong_free:
    print("any bad object was deallocated")
else:
    print("no wrong deallocation (maybe)")

print("leaked:")
print("\n".join(x))
