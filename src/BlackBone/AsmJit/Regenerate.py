#!/usr/bin/env python

# =============================================================================
# [Regenerate.py - Description]
#
# This Python script will regenerate instruction names in Assembler{$ARCH}.cpp
# files. Idea is that there can be big instruction table and this script tries
# to minimize runtime relocation data of AsmJit library.
# =============================================================================

# =============================================================================
# [Configuration]
# =============================================================================

# Files to process.
FILES = [
  "DefsX86X64.cpp"
]

# =============================================================================
# [Imports]
# =============================================================================

import os, re, string

# =============================================================================
# [Helpers]
# =============================================================================

def readFile(fileName):
  handle = open(fileName, "rb")
  data = handle.read()
  handle.close()
  return data

def writeFile(fileName, data):
  handle = open(fileName, "wb")
  handle.truncate()
  handle.write(data)
  handle.close()

# =============================================================================
# [Main]
# =============================================================================

def processFile(fileName):
  data = readFile(fileName);

  din = data
  r = re.compile(r"instructionDescription\[\][\s]*=[\s]*{(?P<BLOCK>[^}])*}")
  m = r.search(din)

  if not m:
    print "Cannot match instruction data in " + fileName
    exit(0)

  din = din[m.start():m.end()]
  dout = ""

  dinst = []
  daddr = []
  hinst = {}

  r = re.compile(r'\"(?P<INST>[A-Za-z0-9_ ]+)\"')
  dpos = 0
  for m in r.finditer(din):
    inst = m.group("INST")
    
    if not inst in hinst:
      dinst.append(inst)
      hinst[inst] = dpos

      daddr.append(dpos)
      dpos += len(inst) + 1

  dout += "const char instructionName[] =\n"
  for i in xrange(len(dinst)):
    dout += "  \"" + dinst[i] + "\\0\"\n"
  dout += "  ;\n"

  dout += "\n"

  for i in xrange(len(dinst)):
    dout += "#define INST_" + dinst[i].upper().replace(" ", "_") + "_INDEX" + " " + str(daddr[i]) + "\n"

  mb_string = "// ${INSTRUCTION_DATA_BEGIN}\n"
  me_string = "// ${INSTRUCTION_DATA_END}\n"

  mb = data.index(mb_string)
  me = data.index(me_string)

  data = data[:mb + len(mb_string)] + dout + data[me:]

  writeFile(fileName, data)

for fileName in FILES:
  processFile(fileName)
