import slicer

def cliRunSync(module, node=None, parameters=None, delete_temporary_files=True, update_display=True):
  """Run CLI module. If ipywidgets are installed then it reports progress.
  """

  try:
    from ipywidgets import IntProgress
    from IPython.display import display

    # Asynchronous run, with progerss reporting using widget
    node = slicer.cli.run(module, node=node, parameters=parameters, wait_for_completion=False,
    delete_temporary_files=delete_temporary_files, update_display=update_display)
    import time
    progress = IntProgress()
    display(progress) # display progress bar
    while node.IsBusy():
        progress.value = node.GetProgress()
        slicer.app.processEvents()
        time.sleep(.3)
    progress.layout.display = 'none' # hide progress bar

  except ImportError:
    # No widgets, therefore no progress reporting - do just a simpe synchronous CLI run
    node = slicer.cli.runSync(module, node=node, parameters=parameters, wait_for_completion=False,
      delete_temporary_files=delete_temporary_files, update_display=update_display)

  return node
