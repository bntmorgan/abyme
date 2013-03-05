#!/usr/bin/python3.2

import subprocess
import shlex

def process_text(context, section):
  print(section["text"])

def process_image(context, section):
  local_context = {}
  local_context.update(context)
  local_context["count"] = section["end"] - section["start"] + 1
  local_context["skip"] = section["start"] - context["file-offset"]
  local_context["file-offset"] = context["file-offset"]
  local_context["start"] = section["start"]
  local_context["dest"] = section["dest"]
  local_context["end"] = section["end"]

  process_stdin = subprocess.PIPE
  commands = []
  for command in section_type_command[section["type"]]:
    command = command % local_context
    commands.append(command)
    command = shlex.split(command)
    process = subprocess.Popen(command, stdin=process_stdin, stdout=subprocess.PIPE)
    process_stdin = process.stdout
  print()
  print("    " + "|".join(commands))
  result = process.stdout.readlines()
  #print("::")
  #print()
  #print("    " + " | ".join(commands))
  print()
  print(".. image:: %s.png" % (section["dest"]))

def process_section(context, section):
  local_context = {}
  local_context.update(context)
  local_context["count"] = section["end"] - section["start"] + 1
  local_context["skip"] = section["start"] - context["file-offset"]
  local_context["file-offset"] = context["file-offset"]
  local_context["start"] = section["start"]
  local_context["end"] = section["end"]

  process_stdin = subprocess.PIPE
  commands = []
  for command in section_type_command[section["type"]]:
    command = command % local_context
    commands.append(command)
    command = shlex.split(command)
    process = subprocess.Popen(command, stdin=process_stdin, stdout=subprocess.PIPE)
    process_stdin = process.stdout
    process.wait()
  result = process.stdout.readlines()

  #print("::")
  #print()
  #print("    " + " | ".join(commands))
  print()
  print("::")
  print()
  #print("    " + "|".join(commands))
  for line in result:
    print("    " + line.decode().rstrip())
  print()

section_type_command = {
  "PCODE32": ["python3.2 ./disasm-to-pseudocode.py -o 0x%(file-offset)x -s 0x%(start)x -e 0x%(end)x -b 32 -f %(file)s"],
  "CODE32": ["objdump -D -b binary --adjust-vma=0x%(file-offset)x --start-address=0x%(start)x --stop-address=0x%(end)x -mi386 -Maddr32,data32 %(file)s"],
  "CODE16": ["objdump -D -b binary --adjust-vma=0x%(file-offset)x --start-address=0x%(start)x --stop-address=0x%(end)x -mi386 -Maddr16,data16 %(file)s"],
  "GRAPH32": ["python3.2 ./disasm-to-graph.py -o 0x%(file-offset)x -s 0x%(start)x -e %(end)x -b 32 -f %(file)s", "dot -Tpng -o %(dest)s.png"],
  "GDT32":  ["hexdump -v -e '2/4 \"%%08x \" \"\\n\"' -s %(skip)s -n %(count)d %(file)s",
             "python cat.py -s 0x%(start)x -p 0x08"],
  "DATA32": ["hexdump -v -e '6/4 \"%%08x \" \"\\n\"' -s %(skip)s -n %(count)d %(file)s",
             "python cat.py -s 0x%(start)x -p 0x18"],
  "DATA16": ["hexdump -v -e '12/2 \"%%04x \" \"\\n\"' -s %(skip)s -n %(count)d %(file)s",
             "python cat.py -s 0x%(start)x -p 0x18"],
  "DATA08": ["hexdump -v -e '24/1 \"%%02x \" \"\\n\"' -s %(skip)s -n %(count)d %(file)s",
             "python cat.py -s 0x%(start)x -p 0x18"],
  "COUNT08": ["echo '%(count)d octets'",
             "python cat.py -s 0x%(start)x -p 0x18"],
  "TEXT":   ["hexdump -v -e '6/4 \"%%08x \" \"\"' -e '\" \" 24/1 \"%%_p\" \"\\n\"' -s %(skip)s -n %(count)d %(file)s",
             "python cat.py -s 0x%(start)x -p 4"],
  "GDTR32": ["hexdump -v -e '1/2 \"%%04x-\" 1/4 \"%%08x\" \"\\n\"' -s %(skip)s -n 6 %(file)s",
             "python cat.py -s 0x%(start)x -p 0x6"]
}

context_file = open("context", "r")
context = context_file.readlines()
context = [line.strip() for line in context]
context = dict([configuration.split("=") for configuration in context])
context["file-offset"] = int(context["file-offset"], 16)
context_file.close()

sections_file = open("sections", "r")
sections = []
section = None
for line in sections_file:
  if line.startswith(":section	"):
    section = line[len(":section	"):]
    section = section.strip().split("\t")
    section = section[0].split(":") + [section[1]]
    section = dict(zip(['start', 'end', 'type'], section))
    section["start"] = int(section["start"], 16)
    section["end"] = int(section["end"], 16)
    section["handler"] = process_section
    sections.append(section)
  elif line.startswith(":image	"):
    section = line[len(":image	"):]
    section = section.strip().split("\t")
    section = section[0].split(":") + section[1:]
    section = dict(zip(['start', 'end', 'type', 'dest'], section))
    section["start"] = int(section["start"], 16)
    section["end"] = int(section["end"], 16)
    section["handler"] = process_image
    sections.append(section)
  else:
    text = {}
    text["text"] = line.rstrip()
    text["handler"] = process_text
    sections.append(text)
sections_file.close()

covered = []
for section in sections:
  if section["handler"] == process_section:
    covered.append([section["start"], section["end"], section["type"]])

print("Liste des sections traitées")
print("---------------------------")
print()
for a in covered:
  print("- ``0x%016x:0x%016x``	%s" % (a[0], a[1], a[2]))
print()

changed = True
while changed:
  changed = False
  new_covered = []
  for i in range(len(covered)):
    a = covered[i]
    append = True
    for j in range(len(new_covered)):
      b = new_covered[j]
      if a[0]  <= b[0] and b[0] <= a[1]:
        del new_covered[j]
        new_covered.append([a[0], max(a[1], b[1])])
        append = False
        changed = True
        break
      elif b[0]  <= a[0] and a[0] <= b[1]:
        del new_covered[j]
        new_covered.append([b[0], max(a[1], b[1])])
        append = False
        changed = True
        break
    if append:
      new_covered.append(a)
  covered = new_covered
size = 0
covered = sorted(covered, key=lambda x: x[0])
for a in covered:
  print("- ``0x%016x:0x%016x``" % (a[0], a[1]))
  size = size + (a[1] - a[0] + 1)
print("- Total : %d octets" % size)
print()
print()
for a in covered:
  print("``(0x%016x,0x%016x),``" % (a[0], a[1]))
  size = size + (a[1] - a[0] + 1)
print()
for section in sections:
  section["handler"](context, section)
