#!/usr/bin/python
# python % -o 0xffc00000 -s 0x00000000fffffd8d -e 0x00000000fffffe61 -b 32 -f bios.rom

import sys
import re
import shlex
import subprocess
import getopt

#instruction_pattern_jump = re.compile(r'^(jmp)  *0x[0-9a-f]*$')
#instruction_pattern_cond_jump = re.compile(r'^(je|jz|jb|jae|jne|jbe|loop)  *0x[0-9a-f]*$')
#instruction_pattern_end = re.compile(r'^(ret|jmp  *\*%esi)$')
#instruction_pattern_normal_a = re.compile(r'^(out|call|movl|pushl|in|push|inc|mov|or|orl|and|bt|movzbl|sub|movzwl|bts|dec|movd|bsf|bsr|not|stos|btr|xor|test|lea|add|shl|shr|cmp|cmpl|repz cmpsl|rep stos|movqi|movq2dq|pshufd|movdqa|movq|pop) .*$')
#instruction_pattern_normal_b = re.compile(r'^(cpuid|rdmsr|wrmsr|invd|sfence|cld|leave|emms)$')
instruction_pattern_jump = re.compile(r'^(jmp)  *0x[0-9a-f]*$')
instruction_pattern_cond_jump = re.compile(r'^(je|jz|jb|jae|jle|jne|jbe|jge|jl|jg|loop)  *0x[0-9a-f]*$')
instruction_pattern_end = re.compile(r'^(ret|jmp  *\*%esi)$')
instruction_pattern_normal_a = re.compile(r'^(andl|incl|decl|out|testb|call|movl|addl|subl|pushl|in|push|inc|mov|or|orl|and|bt|movzbl|sub|movzwl|bts|dec|movd|bsf|bsr|not|stos|btr|xor|test|lea|add|shl|shr|cmp|cmpl|repz cmpsl|rep stos|movqi|movq2dq|pshufd|movdqa|movq|pop) .*$')
instruction_pattern_normal_b = re.compile(r'^(cpuid|rdmsr|wrmsr|invd|sfence|cld|leave|emms)$')

argv = sys.argv[1:]

start = 0
end = 0
file_offset = 0
binary_file = None
bit = 0

try:
  opts, args = getopt.getopt(argv, "o:s:e:b:f:")
except getopt.GetoptError:
  print("%s -o <file-offset-hex> -s <start-hex> -e <end-hex> -b <bit-dec> -f <binary-file>" % sys.argv[0])
  sys.exit(2)
for opt, arg in opts:
  if opt == '-s':
    start = int(arg, 16)
  elif opt == "-e":
    end = int(arg, 16)
  elif opt == "-b":
    bit = int(arg)
  elif opt == "-o":
    file_offset = int(arg, 16)
  elif opt == "-f":
    binary_file = arg

context = {}
context["count"] = end - start + 1
context["skip"] = start - file_offset
context["file-offset"] = file_offset
context["start"] = start
context["end"] = end
context["bit"] = bit
context["binary-file"] = binary_file

#command = "udcli -nohex -%(bit)d -att -v intel -s %(skip)d -c %(count)d -o 0x%(offset)x %(binary-file)s"
command = "objdump -D -b binary -mi386 -Maddr%(bit)d,data%(bit)d --adjust-vma=0x%(file-offset)x --start-address=0x%(start)x --stop-address=0x%(end)x --no-show-raw-insn %(binary-file)s"
command = command % context
command = shlex.split(command)
process = subprocess.Popen(command, stdout=subprocess.PIPE)
instructions = process.stdout.readlines()
instructions = instructions[7:]
instructions = [instruction.decode().strip().split(":\t", 1) for instruction in instructions]

address_to_line = dict([(int(instruction[0], 16), index) for index, instruction in enumerate(instructions)])

jumps = []
starts = [0]
for index, instruction in enumerate(instructions):
  if instruction_pattern_jump.match(instruction[1]):
    jump_address = int(instruction[1].split(" ", 1)[1].split("x")[1], 16)
    #print(instruction, jump_address)
    jump_line = address_to_line[jump_address]
    starts.append(index + 1)
    starts.append(jump_line)
    #jumps.append((index, jump_line))
  elif instruction_pattern_cond_jump.match(instruction[1]):
    jump_address = int(instruction[1].split(" ", 1)[1].split("x")[1], 16)
    #print(instruction)
    jump_line = address_to_line[jump_address]
    starts.append(index + 1)
    starts.append(jump_line)
    #jumps.append((index, jump_line))
    #jumps.append((index, index + 1))
  elif instruction_pattern_normal_a.match(instruction[1]):
    pass
  elif instruction_pattern_normal_b.match(instruction[1]):
    pass
  elif instruction_pattern_end.match(instruction[1]):
    starts.append(index + 1)
  else:
    print("erreur: ", instruction)

starts = list(set(starts))
starts.sort()
starts = starts[:-1]

address_to_block = {}
block = 0
for index in range(len(instructions)):
  instruction = instructions[index]
  address = int(instruction[0], 16)
  if block < len(starts) - 1:
    if index < starts[block + 1]:
      address_to_block[address] = block
    else:
      block = block + 1
      address_to_block[address] = block
  else:
    address_to_block[address] = block

for index in range(len(starts) - 1):
  start = starts[index]
  end = starts[index + 1] - 1
  instruction = instructions[end]
  if instruction_pattern_jump.match(instruction[1]):
    ###address_to = int(instruction[1].split(" ")[1], 16)
    address_to = int(instruction[1].split(" ", 1)[1].split("x")[1], 16)
    block_to = address_to_block[address_to]
    block_from = address_to_block[int(instruction[0], 16)]
    jumps.append((block_from, block_to))
  elif instruction_pattern_cond_jump.match(instruction[1]):
    ###address_to = int(instruction[1].split(" ")[1], 16)
    address_to = int(instruction[1].split(" ", 1)[1].split("x")[1], 16)
    block_to = address_to_block[address_to]
    block_from = address_to_block[int(instruction[0], 16)]
    jumps.append((block_from, block_to))
    jumps.append((block_from, block_from + 1))
  else:
    block_from = address_to_block[int(instruction[0], 16)]
    jumps.append((block_from, block_from + 1))

print("digraph g {")
print("  graph [fontsize=30 labelloc=\"t\" label=\"\" splines=true overlap=false rankdir = \"LR\"];")
print("  ratio = auto;")

for index, start in enumerate(starts):
  end = len(instructions)
  if index < len(starts) - 1:
    end = starts[index + 1] - 1
  print('"state%d" [ style="filled,bold" penwidth=5 fillcolor="white"' % index)
  print('  fontname="Courier New" shape="Mrecord"')
  print('  label=<<table border="0" cellborder="0" cellpadding="3" bgcolor="white">')
  print('  <tr><td bgcolor="black" align="center" colspan="2"><font color="white">%x</font></td></tr>' % int(instructions[start][0], 16))
  for instruction in instructions[start:(end + 1)]:
    print('  <tr><td align="left">%s</td></tr>' % instruction[1])
  print('              </table>> ];')

for (block_from, block_to) in jumps:
  print('state%d -> state%d [ penwidth=5 fontsize=28 fontcolor="black" ];' % (block_from, block_to))
print("}")
