
# Set up display hook to display execution result
# as raw text in Slicer's Python console and as
# rich output in Jupyter notebook.

from JupyterNotebooksLib import displayable as __JupyterNotebooksLib_displayable

def slicerDisplayHook(value):
  # show raw output in Slicer's Python console
  pythonConsole = slicer.app.pythonConsole()
  if pythonConsole and (value is not None):
    pythonConsole.printOutputMessage(repr(value))

  # Convert Slicer data objects to objects that the notebook
  # can display better.
  # This applied when displaying the last command of a cell automatically.
  value = __JupyterNotebooksLib_displayable(value)
  # forward value to xeus-python display hook
  slicer.xeusPythonDisplayHook(value)

import sys
sys.displayhook = slicerDisplayHook
