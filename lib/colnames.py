#!/usr/bin/python

# colnames.py
# Converts a dictionary of color names in YAML format to a scanner-like function

class Node:
  "Represents a single node in a prefix tree. Knows parent, children, char."
  def __init__(self, parent, char, is_end = False):
    self.is_end = is_end
    self.prefix_count = 0
    self.parent = parent
    self.char = char
    self.children = [None for child in range(26)]

  def dump(self):
    "Dumps the node to stdout as a C(++) if statement."
    for i, child in enumerate(self.children):
      if child is not None:
        print("if (*(buf + %d) == '%s') { " % (len(child.parents()) - 1, chr(i + ord('a'))))
        child.dump()
        if child.is_end:
          wordlen = len(child.parents())
          replacement = repl[self.key() + child.char]
          print("memset(buf, 0, %d);" % wordlen)
          print("memcpy(buf, \"#%s\", %d);" % (replacement, len(replacement) + 1))
          print("modified = true;")
          print("buf += %d;" % wordlen)
          print("continue;")
        print("}")

  def parents(self):
    "Returns a list of the node's parents, closest first."
    i = []
    cur = self
    while cur.parent:
      i.append(cur.parent)
      cur = cur.parent
    return i

  def key(self):
    "Returns the complete key this node ends."
    s = ""
    cur = self
    while cur.parent:
      s = cur.char + s
      cur = cur.parent
    return s

class Trie:
  def __init__(self):
    self.head = Node(None, '')

  def insert(self, word):
    current = self.head

    for letter in word:
      int_value = ord(letter) - ord('a')

      if current.children[int_value] is None:
        current.children[int_value] = Node(current, letter)

      current = current.children[int_value]
      current.prefix_count += 1
    current.is_end = True

  def dump(self):
    self.head.dump()

import yaml
repl = yaml.load(open('colnames.yml'))
keys = sorted(repl.keys())
trie = Trie()

for key in keys:
  trie.insert(key)

print("// This mass replacement function is auto-generated and should not")
print("// be used standalone, nor edited. Edit colnames.yml instead.")
print()
print("bool replace_colnames(char* buf) {")
print("bool modified;")
print("while (*(buf) != '\\0') {")
trie.dump()
print("++buf;")
print("}")
print("return modified;")
print("}")
