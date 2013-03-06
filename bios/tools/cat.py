#!/usr/bin/python3.2

import sys
import getopt

def main(argv):
  start = 0
  step = 1
  nformat = "%016x %s"

  try:
    opts, args = getopt.getopt(argv, "s:p:f:")
  except getopt.GetoptError:
    print("%s -s <start-hex> -p <step-hex> [-f <format>]")
    sys.exit(2)
  for opt, arg in opts:
    if opt == '-s':
      start = int(arg, 16)
    elif opt == "-p":
      step = int(arg, 16)
    elif opt == "-f":
      nformat = arg
  line = sys.stdin.readline()
  while line:
    line = line.strip()
    print(nformat % (start, line))
    start = start + step
    line = sys.stdin.readline()
  
if __name__ == "__main__":
  main(sys.argv[1:])
