
# Set up display hook to display execution result
# as raw text in Slicer's Python console and as
# rich output in Jupyter notebook.

from JupyterNotebooksLib import displayNode as __displayNode

def slicerDisplayHook(value):
  # show raw output in Slicer's Python console
  pythonConsole = slicer.app.pythonConsole()
  if pythonConsole and (value is not None):
    pythonConsole.printOutputMessage(repr(value))

  # Convert Slicer data objects to objects that the notebook
  # can display better.
  nodeValue = __displayNode(value)
  if nodeValue is not None:
    value = nodeValue

  # forward value to xeus-python display hook
  slicer.xeusPythonDisplayHook(value)

import sys
sys.displayhook = slicerDisplayHook
