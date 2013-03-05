#!/usr/bin/python
# python3.2 ./disasm-to-graph.py -o 0xffc00000 -s 0xfffffd8d -e fffffe61 -b 32 -f d.bios.rom

import sys
import re
import shlex
import subprocess
import getopt

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

command = "objdump -D -b binary -mi386 -Maddr%(bit)d,data%(bit)d --adjust-vma=0x%(file-offset)x --start-address=0x%(start)x --stop-address=0x%(end)x --no-show-raw-insn %(binary-file)s"
command = command % context
command = shlex.split(command)
process = subprocess.Popen(command, stdout=subprocess.PIPE)
instructions = process.stdout.readlines()
instructions = instructions[7:]
instructions = [instruction.decode().strip().split(":\t", 1) for instruction in instructions]

address_to_line = dict([(int(instruction[0], 16), index) for index, instruction in enumerate(instructions)])

#for i in address_to_line:
#  print("%x %d" % (i, address_to_line[i]))
#
#for instruction in instructions:
#  print(instruction)

jumps = []
starts = [0]
for index, instruction in enumerate(instructions):
  #print("processing ", index, instruction)
  if instruction_pattern_jump.match(instruction[1]):
    starts.append(index + 1)
    jump_address = int(instruction[1].split(" ", 1)[1].split("x")[1], 16)
    #print("jmp: %x" % jump_address, jump_address in address_to_line)
    #print(starts)
    if jump_address in address_to_line:
      jump_line = address_to_line[jump_address]
      starts.append(jump_line)
  elif instruction_pattern_cond_jump.match(instruction[1]):
    jump_address = int(instruction[1].split(" ", 1)[1].split("x")[1], 16)
    jump_line = address_to_line[jump_address]
    starts.append(index + 1)
    starts.append(jump_line)
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
#print(starts)
#starts = starts[:-1]
#print(starts)

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
    #address_to = int(instruction[1].split(" ")[1], 16)
    address_to = int(instruction[1].split(" ", 1)[1].split("x")[1], 16)
    #print("address to: %x" % address_to)
    if address_to in address_to_block:
      block_to = address_to_block[address_to]
      block_from = address_to_block[int(instruction[0], 16)]
      jumps.append((block_from, block_to))
  elif instruction_pattern_cond_jump.match(instruction[1]):
    #address_to = int(instruction[1].split(" ")[1], 16)
    address_to = int(instruction[1].split(" ", 1)[1].split("x")[1], 16)
    block_to = address_to_block[address_to]
    block_from = address_to_block[int(instruction[0], 16)]
    jumps.append((block_from, block_to))
    jumps.append((block_from, block_from + 1))
  else:
    block_from = address_to_block[int(instruction[0], 16)]
    jumps.append((block_from, block_from + 1))

blocs = []
blocs_id = []
for index, start in enumerate(starts):
  end = len(instructions)
  if index < len(starts) - 1:
    end = starts[index + 1] - 1
  blocs.append(instructions[start:(end + 1)])
  blocs_id.append(index)

PRINT = False

if PRINT:
  print("blocs:")
  for block_id in range(len(blocs)):
    bloc = blocs[block_id]
    if len(bloc) > 0:
      print()
      print("block ", block_id)
      for (block_from, block_to) in jumps:
        if block_to == block_id:
          print("          jump from ", block_from)
      for ins in bloc:
        print("  ".join(ins))
      for (block_from, block_to) in jumps:
        if block_from == block_id:
          print("          jump to ", block_to)

def get_out(block_id):
  global jumps
  ret = []
  for (block_from, block_to) in jumps:
    if block_from == block_id:
      ret.append(block_to)
  return ret

def get_in(block_id):
  global jumps
  ret = []
  for (block_from, block_to) in jumps:
    if block_to == block_id:
      ret.append(block_from)
  return ret

#for (block_from, block_to) in jumps:
#  print("> ", block_from, block_to)
#print("====")

# Tout d'abord, pour tous les blocs n'ayant pas de sortie, ajouter le marqueur STOP.
for bloc_a in blocs_id:
  blocs_a_out = get_out(bloc_a)
  if len(blocs_a_out) == 0:
    bloc = blocs[bloc_a]
    bloc.append(["        ", "STOP"])

def process_if_then():
  # Traitement des if-then
  # Recherche des motifs :
  #     a -> b -> c
  #     a -> c
  #     a != b
  #     a != c
  #     b != c
  #    |X -> b| == 1
  #    |b -> X| == 1
  #    |a -> X| == 2
  # Integration de b dans a
  # Suppression de a -> b et b -> c
  # Suppression de b
  for bloc_b in blocs_id:
    blocs_out_b = get_out(bloc_b)
    blocs_in_b = get_in(bloc_b)
    if len(blocs_out_b) == 1 and len(blocs_in_b) == 1:
      bloc_a = blocs_in_b[0]
      bloc_c = blocs_out_b[0]
      if bloc_a != bloc_b and bloc_b != bloc_c and bloc_a != bloc_c:
        blocs_out_a = get_out(bloc_a)
        blocs_in_c = get_in(bloc_c)
        if len(blocs_out_a) == 2 and bloc_c in blocs_out_a:
          jumps.remove((bloc_a, bloc_b))
          jumps.remove((bloc_b, bloc_c))
          blocs_id.remove(bloc_b)
          new_instructions = []
          new_instructions.extend([[ins[0], ins[1]] for ins in blocs[bloc_a]])
          new_instructions.append(["        ", "IF-THEN"])
          new_instructions.extend([[ins[0], "  " + ins[1]] for ins in blocs[bloc_b]])
          new_instructions.append(["        ", "ENDIF"])
          blocs[bloc_a] = new_instructions
          if PRINT:
            print("Changed case if-then:", bloc_a, bloc_b, bloc_c)
          return True

def process_if_then_else():
  # Traitement des if-then-else
  # Recherche des motifs :
  #     a -> b -> c
  #     a -> d -> c
  #     a != b
  #     a != d
  #     b != c
  #     b != d
  #     c != d
  #    |X -> b| == 1
  #    |b -> X| == 1
  #    |a -> X| == 2
  #    |X -> d| == 1
  #    |d -> X| == 1
  # Integration de b et d dans a
  # Suppression de a -> b, b -> c, a -> d et d -> c
  # Suppression de b et d
  # Ajout de a -> c
  for bloc_a in blocs_id:
    blocs_out_a = get_out(bloc_a)
    if len(blocs_out_a) == 2:
      bloc_b = blocs_out_a[0]
      bloc_d = blocs_out_a[1]
      blocs_out_b = get_out(bloc_b)
      blocs_out_d = get_out(bloc_d)
      blocs_in_b = get_in(bloc_b)
      blocs_in_d = get_in(bloc_d)
      if len(blocs_out_b) == 1 and blocs_out_b == blocs_out_d and len(blocs_in_b) == 1 and len(blocs_in_d) == 1:
        bloc_c = blocs_out_b[0]
        if bloc_a != bloc_b and bloc_a != bloc_d and bloc_b != bloc_c and bloc_b != bloc_d and bloc_d != bloc_c:
          jumps.remove((bloc_a, bloc_b))
          jumps.remove((bloc_b, bloc_c))
          jumps.remove((bloc_a, bloc_d))
          jumps.remove((bloc_d, bloc_c))
          jumps.append((bloc_a, bloc_c))
          blocs_id.remove(bloc_b)
          blocs_id.remove(bloc_d)
          new_instructions = []
          new_instructions.extend([[ins[0], ins[1]] for ins in blocs[bloc_a]])
          new_instructions.append(["        ", "IF-THEN"])
          new_instructions.extend([[ins[0], "  " + ins[1]] for ins in blocs[bloc_b]])
          new_instructions.append(["        ", "ELSE"])
          new_instructions.extend([[ins[0], "  " + ins[1]] for ins in blocs[bloc_d]])
          new_instructions.append(["        ", "ENDIF"])
          blocs[bloc_a] = new_instructions
          if PRINT:
            print("Changed case if-then-else:", bloc_a, bloc_b, bloc_d, bloc_c)
          return True

def process_self_boucles():
  # Traitement des boucles locales
  # Recherche des motifs :
  #     a -> a
  # Suppression de a -> a
  for bloc_a in blocs_id:
    blocs_out_a = get_out(bloc_a)
    if bloc_a in blocs_out_a:
      jumps.remove((bloc_a, bloc_a))
      new_instructions = []
      new_instructions.append(["        ", "LOOP"])
      new_instructions.extend([[ins[0], "  " + ins[1]] for ins in blocs[bloc_a]])
      new_instructions.append(["        ", "LOOP-WHILE"])
      blocs[bloc_a] = new_instructions
      if PRINT:
        print("Changed case self boucle:", bloc_a)
      return True

def process_seq_fixed():
  # Traitement des sequences fixes
  # Recherche des motifs :
  #     a -> b
  #     a != b
  #    |X -> b| == 1
  #    |a -> X| == 1
  # Integration de b dans a
  # Remplacement de chaque b -> X par a -> X
  # Suppression de a -> b
  # Suppression de b
  for bloc_b in blocs_id:
    blocs_in_b = get_in(bloc_b)
    if len(blocs_in_b) == 1:
      bloc_a = blocs_in_b[0]
      blocs_out_a = get_out(bloc_a)
      if bloc_a != bloc_b and len(blocs_out_a) == 1:
          jumps.remove((bloc_a, bloc_b))
          blocs_out_b = get_out(bloc_b)
          for bloc_out_b in blocs_out_b:
            jumps.remove((bloc_b, bloc_out_b))
            jumps.append((bloc_a, bloc_out_b))
          blocs_id.remove(bloc_b)
          new_instructions = []
          new_instructions.extend([[ins[0], ins[1]] for ins in blocs[bloc_a]])
          new_instructions.extend([[ins[0], ins[1]] for ins in blocs[bloc_b]])
          blocs[bloc_a] = new_instructions
          if PRINT:
            print("Changed case seq fixe:", bloc_a, bloc_b)
          return True

def process_final_cond_strict():
  # Traitement des sequences finales conditionnelles
  # Recherche des motifs :
  #     a -> b
  #    |b -> X| == 0
  #    |X -> b| == 1
  #    |a -> X| == 2
  # Integration de b dans a
  # Suppression de a -> b
  # Suppression de b
  for bloc_b in blocs_id:
    blocs_in_b = get_in(bloc_b)
    blocs_out_b = get_out(bloc_b)
    if len(blocs_in_b) == 1 and len(blocs_out_b) == 0:
      bloc_a = blocs_in_b[0]
      jumps.remove((bloc_a, bloc_b))
      blocs_id.remove(bloc_b)
      new_instructions = []
      new_instructions.extend([[ins[0], ins[1]] for ins in blocs[bloc_a]])
      new_instructions.append(["        ", "IF-THEN"])
      new_instructions.extend([[ins[0], "  " + ins[1]] for ins in blocs[bloc_b]])
      new_instructions.append(["        ", "END-IF"])
      blocs[bloc_a] = new_instructions
      if PRINT:
        print("Changed case final cond strict:", bloc_a, bloc_b)
      return True




def process_duplicate_smaller_node():
  result = None
  best = 0xffffffff
  for bloc_b in blocs_id:
    if len(blocs[bloc_b]) < best:
      blocs_in_b = get_in(bloc_b)
      if len(blocs_in_b) > 1:
        for bloc_a in blocs_in_b:
          blocs_out_a = get_out(bloc_a)
          if len(blocs_out_a) > 1:
          #if len(blocs_out_a) == 1:
            best = len(blocs[bloc_b])
            result = (bloc_a, bloc_b)
            break
  if result is not None:
    bloc_a = result[0]
    bloc_b = result[1]
    jumps.remove((bloc_a, bloc_b))
    blocs_out_b = get_out(bloc_b)
    new_bloc_b = len(blocs_id)
    blocs_id.append(new_bloc_b)
    jumps.append((bloc_a, new_bloc_b))
    for bloc_out_b in blocs_out_b:
      jumps.append((new_bloc_b, bloc_out_b))
    blocs.append(blocs[bloc_b][:])
    return True
  return False












def process_node_bigger_in():
  result = None
  best = 0xffffffff
  for bloc_b in blocs_id:
    if len(blocs[bloc_b]) < best:
      blocs_in_b = get_in(bloc_b)
      if len(blocs_in_b) > 2:
        for bloc_a in blocs_in_b:
          blocs_out_a = get_out(bloc_a)
          if len(blocs_out_a) == 1:
            best = len(blocs[bloc_b])
            result = (bloc_a, bloc_b)
            break
  if result is not None:
    bloc_a = result[0]
    bloc_b = result[1]
    jumps.remove((bloc_a, bloc_b))
    blocs_out_b = get_out(bloc_b)
    for bloc_out_b in blocs_out_b:
      jumps.append((bloc_a, bloc_out_b))
    new_instructions = []
    new_instructions.extend([[ins[0], ins[1]] for ins in blocs[bloc_a]])
    new_instructions.extend([[ins[0], ins[1]] for ins in blocs[bloc_b]])
    blocs[bloc_a] = new_instructions
    return True
  return False

def process_final_doubles():
  # Traitement des sequences finales doubles
  # Recherche des motifs :
  #     a -> b
  #     a -> c
  #    |b -> X| == 0
  #    |c -> X| == 0
  #    |a -> X| == 2
  # Integration de b et c dans a
  # Suppression de a -> b et a -> c
  # Suppression de b si |X -> b| == 0
  # Suppression de c si |X -> c| == 0
  for bloc_a in blocs_id:
    blocs_out_a = get_out(bloc_a)
    if len(blocs_out_a) == 2:
      bloc_b = blocs_out_a[0]
      bloc_c = blocs_out_a[1]
      blocs_out_b = get_out(bloc_b)
      blocs_out_c = get_out(bloc_c)
      if len(blocs_out_b) == 0 and len(blocs_out_c) == 0:
        jumps.remove((bloc_a, bloc_b))
        jumps.remove((bloc_a, bloc_c))
        new_instructions = []
        new_instructions.extend([[ins[0], ins[1]] for ins in blocs[bloc_a]])
        new_instructions.append(["        ", "IF-THEN"])
        new_instructions.extend([[ins[0], "  " + ins[1]] for ins in blocs[bloc_b]])
        new_instructions.append(["        ", "ELSE"])
        new_instructions.extend([[ins[0], "  " + ins[1]] for ins in blocs[bloc_c]])
        new_instructions.append(["        ", "ENDIF"])
        blocs[bloc_a] = new_instructions
        blocs_in_b = get_in(bloc_b)
        if len(blocs_in_b) == 0:
          blocs_id.remove(bloc_b)
        blocs_in_c = get_in(bloc_c)
        if len(blocs_in_c) == 0:
          blocs_id.remove(bloc_c)
        if PRINT:
          print("Changed case finales doubles:", bloc_a, bloc_b, bloc_c)
        return True

def process_final_mixes():
  # Traitement des sequences finales
  # Recherche des motifs :
  #     a -> b
  #     a -> c
  #    |b -> X| == 0
  #    |c -> X| != 0
  #    |a -> X| == 2
  # Integration de b dans a
  # Suppression de a -> b
  # Suppression de b si |X -> b| == 0
  # Suppression de b
  for bloc_a in blocs_id:
    blocs_out_a = get_out(bloc_a)
    if len(blocs_out_a) == 2:
      bloc_b = blocs_out_a[0]
      bloc_c = blocs_out_a[1]
      blocs_out_b = get_out(bloc_b)
      blocs_out_c = get_out(bloc_c)
      if len(blocs_out_c) == 0:
        bloc_tmp = bloc_b
        blocs_out_tmp = blocs_out_b
        bloc_b = bloc_c
        blocs_out_b = blocs_out_c
        bloc_c = bloc_tmp
        blocs_out_c = blocs_out_tmp
      if len(blocs_out_b) == 0 and len(blocs_out_c) != 0:
        jumps.remove((bloc_a, bloc_b))
        new_instructions = []
        new_instructions.extend([[ins[0], ins[1]] for ins in blocs[bloc_a]])
        new_instructions.append(["        ", "IF-THEN"])
        new_instructions.extend([[ins[0], "  " + ins[1]] for ins in blocs[bloc_b]])
        new_instructions.append(["        ", "ENDIF"])
        blocs[bloc_a] = new_instructions
        blocs_in_b = get_in(bloc_b)
        if len(blocs_in_b) == 0:
          blocs_id.remove(bloc_b)
        if PRINT:
          print("Changed case final mixes:", bloc_a, bloc_b, bloc_c)
        return True

###
###
###
###def process_final_mixes():
###  # Traitement des sequences finales
###  # Recherche des motifs :
###  #     a -> b
###  #     a -> c
###  #    |b -> X| == 0
###  #    |c -> X| != 0
###  #    |a -> X| == 2
###  # Integration de b dans a
###  # Suppression de a -> b
###  # Suppression de b si |X -> b| == 0
###  # Suppression de b
###  for bloc_a in blocs_id:
###    blocs_out_a = get_out(bloc_a)
###    if len(blocs_out_a) == 2:
###      bloc_b = blocs_out_a[0]
###      bloc_c = blocs_out_a[1]
###      blocs_out_b = get_out(bloc_b)
###      blocs_out_c = get_out(bloc_c)
###      if len(blocs_out_c) == 0:
###        bloc_tmp = bloc_b
###        blocs_out_tmp = blocs_out_b
###        bloc_b = bloc_c
###        blocs_out_b = blocs_out_c
###        bloc_c = bloc_tmp
###        blocs_out_c = blocs_out_tmp
###      if len(blocs_out_b) == 0 and len(blocs_out_c) != 0:
###        jumps.remove((bloc_a, bloc_b))
###        new_instructions = []
###        new_instructions.extend([[ins[0], ins[1]] for ins in blocs[bloc_a]])
###        new_instructions.append(["        ", "IF-THEN"])
###        new_instructions.extend([[ins[0], "  " + ins[1]] for ins in blocs[bloc_b]])
###        new_instructions.append(["        ", "ENDIF"])
###        blocs[bloc_a] = new_instructions
###        blocs_in_b = get_in(bloc_b)
###        if len(blocs_in_b) == 0:
###          blocs_id.remove(bloc_b)
###        if PRINT:
###          print("Changed case final mixes:", bloc_a, bloc_b, bloc_c)
###        return True
###
###
###def process_seq_fixed():
###  # Traitement des sequences fixes
###  # Recherche des motifs :
###  #     a -> b -> c
###  #     a != b
###  #     a != c
###  #     b != c
###  #    |X -> b| == 1
###  #    |b -> X| == 1
###  #    |a -> X| == 1
###  #    |X -> c| == 1
###  # Integration de b et c dans a
###  # Suppression de a -> b et b -> c
###  # Remplacement de chaque c -> X par a -> X
###  # Suppression de b et c
###  for bloc_b in blocs_id:
###    blocs_out_b = get_out(bloc_b)
###    blocs_in_b = get_in(bloc_b)
###    if len(blocs_out_b) == 1 and len(blocs_in_b) == 1:
###      bloc_a = blocs_in_b[0]
###      bloc_c = blocs_out_b[0]
###      if bloc_a != bloc_b and bloc_b != bloc_c and bloc_a != bloc_c:
###        blocs_out_a = get_out(bloc_a)
###        blocs_in_c = get_in(bloc_c)
###        if len(blocs_out_a) == 1 and bloc_b in blocs_out_a and len(blocs_in_c) == 1 and bloc_b in blocs_in_c:
###          jumps.remove((bloc_a, bloc_b))
###          jumps.remove((bloc_b, bloc_c))
###          blocs_out_c = get_out(bloc_c)
###          for bloc_out_c in blocs_out_c:
###            jumps.remove((bloc_c, bloc_out_c))
###            jumps.append((bloc_a, bloc_out_c))
###          blocs_id.remove(bloc_b)
###          blocs_id.remove(bloc_c)
###
###          new_instructions = []
###          new_instructions.extend([[ins[0], ins[1]] for ins in blocs[bloc_a]])
###          new_instructions.extend([[ins[0], ins[1]] for ins in blocs[bloc_b]])
###          new_instructions.extend([[ins[0], ins[1]] for ins in blocs[bloc_c]])
###          blocs[bloc_a] = new_instructions
###          if PRINT:
###            print("Changed case seq_fixes:", bloc_a, bloc_b, bloc_c)
###          return True
###
###def process_boucles_generic():
###  # Traitement des boucles
###  # Recherche des motifs :
###  #     a -> b
###  #     b -> a
###  #     a != b
###  #    |X -> b| == 1
###  #    |b -> X| == 1
###  # Integration de b dans a
###  # Suppression de b
###  for bloc_b in blocs_id:
###    blocs_out_b = get_out(bloc_b)
###    blocs_in_b = get_in(bloc_b)
###    if len(blocs_out_b) == 1 and len(blocs_in_b) == 1 and blocs_in_b == blocs_out_b:
###      bloc_a = blocs_in_b[0]
###      jumps.remove((bloc_a, bloc_b))
###      jumps.remove((bloc_b, bloc_a))
###      blocs_id.remove(bloc_b)
###      new_instructions = []
###      new_instructions.append(["        ", "TEST-LOOP"])
###      new_instructions.extend([[ins[0], "  " + ins[1]] for ins in blocs[bloc_a]])
###      new_instructions.append(["        ", "TEST-LOOP-THEN"])
###      new_instructions.extend([[ins[0], "  " + ins[1]] for ins in blocs[bloc_b]])
###      new_instructions.append(["        ", "END-TEST-LOOP"])
###      blocs[bloc_a] = new_instructions
###      if PRINT:
###        print("Changed case boucles generic:", bloc_a, bloc_b, bloc_c)
###      return True


#PRINT = True

while True:
  if process_seq_fixed():
    continue
  if process_if_then():
    continue
  if process_if_then_else():
    continue
  if process_self_boucles():
    continue
  if process_final_cond_strict():
    continue


  #if process_duplicate_smaller_node():
  #  continue
  #if process_final_doubles():
  #  continue
  if process_final_mixes():
    continue

  #if process_node_bigger_in():
  #  continue
  break

#for (block_from, block_to) in jumps:
#  print("> ", block_from, block_to)

for block_id in blocs_id:
  bloc = blocs[block_id]
  if len(bloc) > 0 and (len(bloc) > 1 or (len(bloc) == 1 and bloc[0][1] != "STOP")):
    print("# BLOCK ", block_id)
    for (block_from, block_to) in jumps:
      if block_to == block_id:
        print("# jump from", block_from)
    for (block_from, block_to) in jumps:
      if block_from == block_id:
        print("# jump to", block_to)
    for ins in bloc:
      print("  ".join(ins))
