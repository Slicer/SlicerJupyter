import slicer

def cliRunSync(module, node=None, parameters=None, delete_temporary_files=True, update_display=True):
  """Run CLI module. If ipywidgets are installed then it reports progress.
  :param module: CLI module object (derived from qSlicerCLIModule), for example `slicer.modules.thresholdscalarvolume`.
    List of all module objects (not just CLI modules): `dir(slicer.modules)`.
  :param node: if a parameter node (vtkMRMLCommandLineModuleNode) is specified here then that is used.
    If left at default then a new node will be created and that node will be returned.
  :param parameters: dicttionary containing list of input nodes and parameters and output nodes.
  :param delete_temproary_files: set it to True to preserve all input files.
    May be useful for debugging, but it consumes disk space.
  :param update_display: show output volumes in slice views (resets view position and zoom factor).
  :return: Used parameter node. If the CLI moddule does not need to be executed again then the
    node can be removed from the scene by calling `slicer.mrmlScene.RemoveNode(parameterNode)`
    to avoid clutter in the scene.
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
