a = -6943
print(''.join([str(a >> i & 1) for i in range(32, -1, -1)]))
# 111111111111111111111111110111000
print(hex(int(''.join([str(a >> i & 1) for i in range(32, -1, -1)]), 2)))
