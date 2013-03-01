#!/usr/bin/python3.2

import subprocess
import shlex

def process_text(context, section):
  print(section["text"])

def process_section(context, section):
  #print("0x%016x:0x%016x	%s" % (section["start"], section["end"], section["type"]))
  local_context = {}
  local_context.update(context)
  local_context["count"] = section["end"] - section["start"] + 1
  local_context["skip"] = section["start"] - context["file-offset"]
  local_context["offset"] = section["start"]

  process_stdin = subprocess.PIPE
  commands = []
  for command in section_type_command[section["type"]]:
    command = command % local_context
    commands.append(command)
    command = shlex.split(command)
    process = subprocess.Popen(command, stdin=process_stdin, stdout=subprocess.PIPE)
    process_stdin = process.stdout
  result = process.stdout.readlines()
  #print("> %s" % "\n  | ".join(commands))

  print("::")
  print()
  for line in result:
    print("    " + line.decode().strip())
  print()

section_type_command = {
  "CODE32": ["udcli -32 -att -v intel -s %(skip)d -c %(count)d -o 0x%(offset)x %(file)s"],
  "CODE16": ["udcli -16 -att -v intel -s %(skip)d -c %(count)d -o 0x%(offset)x %(file)s"],
  "GDT32":  ["hexdump -v -e '2/4 \"%%08x \" \"\\n\"' -s %(skip)s -n %(count)d %(file)s",
             "python cat.py -s 0x%(offset)x -p 0x08"],
  "DATA32": ["hexdump -v -e '6/4 \"%%08x \" \"\\n\"' -s %(skip)s -n %(count)d %(file)s",
             "python cat.py -s 0x%(offset)x -p 0x18"],
  "DATA16": ["hexdump -v -e '12/2 \"%%04x \" \"\\n\"' -s %(skip)s -n %(count)d %(file)s",
             "python cat.py -s 0x%(offset)x -p 0x18"],
  "DATA08": ["hexdump -v -e '24/1 \"%%02x \" \"\\n\"' -s %(skip)s -n %(count)d %(file)s",
             "python cat.py -s 0x%(offset)x -p 0x18"],
  "TEXT":   ["hexdump -v -e '6/4 \"%%08x \" \"\"' -e '\" \" 24/1 \"%%_p\" \"\\n\"' -s %(skip)s -n %(count)d %(file)s",
             "python cat.py -s 0x%(offset)x -p 4"],
  "GDTR32": ["hexdump -v -e '1/2 \"%%04x-\" 1/4 \"%%08x\" \"\\n\"' -s %(skip)s -n 6 %(file)s",
             "python cat.py -s 0x%(offset)x -p 0x6"]
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
  else:
    text = {}
    text["text"] = line.rstrip()
    text["handler"] = process_text
    sections.append(text)
sections_file.close()

print("Liste des sections traitées")
print("---------------------------")
print()
for section in sections:
  if section["handler"] == process_section:
    print("- ``0x%016x:0x%016x``	%s" % (section["start"], section["end"], section["type"]))
print()
for section in sections:
  section["handler"](context, section)
