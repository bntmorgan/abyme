import sys
import os
import re

def render(template, __namespace=None, **kw):
  file = open('templates' + os.sep + template, 'r')
  template = file.read()
  file.close()
  template = Template(template)
  namespace = {}
  if __namespace:
    namespace.update(__namespace)
  if kw:
    namespace.update(kw)
  return template.apply(namespace)

class Template(object):

  def __init__(self, template):
    pydelimiters = re.compile('%s(.*?)%s' % (re.escape('${'), re.escape('}$')), re.DOTALL)
    prdelimiters = re.compile('%s(.*?)%s' % (re.escape('$['), re.escape(']$')), re.DOTALL)
    depth = 0
    tokens = []
    for line in template.split("\n"):
      line = line.strip()
      for i, part in enumerate(pydelimiters.split(line)):
        if i % 2 == 1 and len(part) > 0:
          if any([part.startswith(e) for e in ["#end", "elif ", "else:"]]):
            depth -= 2
          tokens.append('%s%s' % (' ' * depth, part))
          if any([part.startswith(e) for e in ["if ", "for ", "while ", "elif ", "else:"]]):
            depth += 2
        elif i % 2 == 0  and len(part) > 0:
          for j, subpart in enumerate(prdelimiters.split(part)):
            if j % 2 == 0:
              subpart = subpart.replace('\\', '\\\\').replace('"', '\\"')
              tokens.append('%semit("%s")' % (' ' * depth, subpart))
            else:
              tokens.append('%semit(%s)' % (' ' * depth, subpart))
    tokens.append("")
    self.__code = compile('\n'.join(tokens), '<template %r>' % template[:20], 'exec')

  def apply(self, namespace):
    namespace['emit'] = self.write
    namespace['render'] = render
    __stdout = sys.stdout
    self.__output = []
    sys.stdout = self
    eval(self.__code, namespace)
    sys.stdout = __stdout
    return ''.join(self.__output)
  
  def write(self, *args):
    self.__output.extend([str(a) for a in args])
