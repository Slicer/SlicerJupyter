import logging
import os
import sys
import unittest
import vtk, qt, ctk, slicer
from slicer.ScriptedLoadableModule import *
from slicer.util import TESTING_DATA_URL

#
# JupyterNotebooks
#

class JupyterNotebooks(ScriptedLoadableModule):
  def __init__(self, parent):
    ScriptedLoadableModule.__init__(self, parent)
    import string
    parent.title = "JupyterNotebooks"
    parent.categories = ["Developer Tools"]
    parent.contributors = ["Andras Lasso (PerkLab)"]
    parent.hidden = True  # don't show this module, there is no user interface yet
    parent.helpText = string.Template("""
This module adds utility functions to make Slicer features conveniently available in Jupyter notebooks.
    """)
    parent.acknowledgementText = """
The module is originally developed by Andras Lasso.
    """

class SlicerJupyterServerHelper:
  def installRequiredPackages(self, force=False):
    """Installed required Python packages for running a Jupyter server in Slicer's Python environment."""
    # Need to install if forced or any packages cannot be imported
    needToInstall = force
    if not needToInstall:
      try:
        import jupyter
        import jupyterlab
        import ipywidgets
        import pandas
        import ipyevents
        import ipycanvas
      except:
        needToInstall = True

    if needToInstall:
      # Install required packages
      import os
      if os.name != 'nt':
        # PIL may be corrupted on linux, reinstall from pillow
        slicer.util.pip_install('--upgrade pillow --force-reinstall')

      slicer.util.pip_install("jupyter jupyterlab ipywidgets pandas ipyevents ipycanvas --no-warn-script-location")

    # Install Slicer Jupyter kernel
    # Create Slicer kernel
    slicer.modules.jupyterkernel.updateKernelSpec()
    # Install Slicer kernel
    import jupyter_client
    jupyter_client.kernelspec.KernelSpecManager().install_kernel_spec(slicer.modules.jupyterkernel.kernelSpecPath(), user=True, replace=True)

class JupyterNotebooksTest(ScriptedLoadableModuleTest):
  """
  This is the test case for your scripted module.
  Uses ScriptedLoadableModuleTest base class, available at:
  https://github.com/Slicer/Slicer/blob/master/Base/Python/slicer/ScriptedLoadableModule.py
  """

  def setUp(self):
    """ Do whatever is needed to reset the state - typically a scene clear will be enough.
    """
    pass

  def runTest(self):
    """Run as few or as many tests as needed here.
    """
    self.setUp()
    self.test_JupyterNotebooks1()

  def test_JupyterNotebooks1(self):
    """ Ideally you should have several levels of tests.  At the lowest level
    tests should exercise the functionality of the logic with different inputs
    (both valid and invalid).  At higher levels your tests should emulate the
    way the user would interact with your code and confirm that it still works
    the way you intended.
    One of the most important features of the tests is that it should alert other
    developers when their changes will have an impact on the behavior of your
    module.  For example, if a developer removes a feature that you depend on,
    your test should break so they know that the feature is needed.
    """

    self.delayDisplay("Starting the test")

    # TODO: implement test

    self.delayDisplay('Test passed!')
