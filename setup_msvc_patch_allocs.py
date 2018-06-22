import os
import re

dirs = [
  'layer0',
  'layer1',
  'layer2',
  'layer3',
  'layer4',
  'layer5',
]

names = ['Alloc', 'Calloc', 'Realloc', 'FreeP', 'DeleteP', 'DeleteAP']

for path in dirs:
  for filename in os.listdir(path):
    fullname = os.path.join(path, filename)
    originalText = text = None
    with open(fullname, 'r') as fi:
      originalText = text = fi.read()
      for name in names:
        text = re.sub(r'\b(%s)\b' % name, r'PyMol\1', text)

    if text and text != originalText:
      with open(fullname, 'w') as fo:
        fo.write(text)

            
