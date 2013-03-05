import struct

f = open("data", "rb")
d = f.read()
f.close()

w = 1024
h = len(d) / w

print(w, h, w * h, len(d), len(d) - (w * h))

import numpy

img = numpy.empty((w, h), numpy.uint32)

img.shape = h, w # set the array shape to our image shape; yes i know it seems backwards, but it's not!
#img[10:21,512:1024]=0xFFFF0000
#img[21:401,758:769]=0xFFFF0000
#img[401:411,512:769]=0xFFFF0000

for i in range(w):
  for j in range(h):
    n = struct.unpack("<B", d[j * w + i])[0]
    #print(i, j)
    img[j, i] = n + (n << 8) + (n << 16) + (0xff << 24)


done = [(0x00000000ffc20000,0x00000000ffc40000),
(0x00000000fffe5404,0x00000000fffe5410),
(0x00000000fffe5583,0x00000000fffe55b3),
(0x00000000fffe5671,0x00000000fffe5833),
(0x00000000fffe5afb,0x00000000fffe5b6d),
(0x00000000fffe5d22,0x00000000fffe5d2b),
(0x00000000fffe5d2c,0x00000000fffe5dd2),
(0x00000000fffe5ee5,0x00000000fffe600c),
(0x00000000fffe6083,0x00000000fffe620e),
(0x00000000fffe6274,0x00000000fffe635a),
(0x00000000fffe63e9,0x00000000fffe6401),
(0x00000000fffe64a4,0x00000000fffe64aa),
(0x00000000fffe6624,0x00000000fffe66e2),
(0x00000000fffe66e4,0x00000000fffe67ac),
(0x00000000fffe67b4,0x00000000fffe67d5),
(0x00000000fffff8c0,0x00000000fffff8d4),
(0x00000000fffff8e0,0x00000000fffff8e9),
(0x00000000fffff8f0,0x00000000fffff92e),
(0x00000000fffff930,0x00000000fffffae4),
(0x00000000fffffaf0,0x00000000fffffd53),
(0x00000000fffffd8d,0x00000000fffffe65),
(0x00000000fffffe66,0x00000000ffffff6a),
(0x00000000ffffff70,0x00000000ffffffb7),
(0x00000000ffffffb8,0x00000000ffffffbd),
(0x00000000ffffffe0,0x00000000ffffffe3),
(0x00000000fffffff0,0x00000000fffffff4),]

for r in done:
  a = r[0] - 0xffc00000
  b = r[1] - 0xffc00000
  for x in range(a, b):
    i = x % w
    j = x / w
    img[j, i] = 0xffff0000

from PIL import Image
pilImage = Image.frombuffer('RGBA', (w, h), img, 'raw', 'RGBA', 0, 1)
pilImage.save('my.png')

