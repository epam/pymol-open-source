import re
from collections import defaultdict

class ErrorList:
  def __init__(self):
    self.by_code = defaultdict(set)

  def add(self, line):
    matched = re.match(r'(?:LINK|.*(?:\(\d+\))?)\s*:\s*(?P<code>(?:warning|(?:fatal )?error)\s+\w+)\s*:\s*.*$', line)
    if matched:
      self.by_code[matched.group('code')].add(line.strip())

  def report(self):
    for code, entries in sorted(self.by_code.items(), key=lambda x: len(x[1]), reverse=True):
      print code, ':', len(entries)
      # print list(self.by_code[code])[0]
      print '\n'.join(sorted(entries))
      print

def main():
  errors = ErrorList()
  with open('setup.log') as f:
    for line in f:
      errors.add(line)
  errors.report()

if __name__ == '__main__':
  main()
