from PIL import Image
import sys
import os
from common import *

files = [
    '0',
    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8',
    '9',
    'colon',
    'slash',
]


cpp_array = ''

for file in files:
    img = Image.open('small_font/' + file + '.png')
    if not img:
        print('Error: can\'t open', file)
        sys.exit(1)
    img_list = compress_list(create_list_from_img(img))
    cpp_array += create_cpp_array_from_list(img_list, 's_' + file)

code = create_cpp_code(cpp_array)
out = open('autogenerated/small_font.h', 'wt')
out.write(code)
out.close()
