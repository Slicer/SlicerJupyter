import slicer

def getFileNameFromURL(url):
    """Get filename from a download URL.
    Attempts to query the recommended filename from the server,
    and if it fails then is uses
    """
    import urllib
    req = urllib.request.Request(url, method='HEAD')
    r = urllib.request.urlopen(req)
    filename = r.info().get_filename()
    if not filename:
      # No filename is available, try to get it from the url
      import os
      from urllib.parse import urlparse
      parsedUrl = urlparse(url)
      filename = os.path.basename(parsedUrl.path)
    return filename

def downloadFromURL(uris=None, fileNames=None, nodeNames=None, checksums=None, loadFiles=None,
  customDownloader=None, loadFileTypes=None, loadFileProperties={}):
  """Download data from custom URL with progress bar.
  :param uris: Download URL(s).
  :param fileNames: File name(s) that will be downloaded (and loaded).
  :param nodeNames: Node name(s) in the scene.
  :param checksums: Checksum(s) formatted as ``<algo>:<digest>`` to verify the downloaded file(s). For example, ``SHA256:cc211f0dfd9a05ca3841ce1141b292898b2dd2d3f08286affadf823a7e58df93``.
  :param loadFiles: Boolean indicating if file(s) should be loaded. By default, the function decides.
  :param customDownloader: Custom function for downloading.
  :param loadFileTypes: file format name(s) ('VolumeFile' by default).
  :param loadFileProperties: custom properties passed to the IO plugin.

  If the given ``fileNames`` are not found in the application cache directory, they
  are downloaded using the associated URIs.
  See ``slicer.mrmlScene.GetCacheManager().GetRemoteCacheDirectory()``

  If not explicitly provided or if set to ``None``, the ``loadFileTypes`` are
  guessed based on the corresponding filename extensions.

  If a given fileName has the ``.mrb`` or ``.mrml`` extension, it will **not** be loaded
  by default. To ensure the file is loaded, ``loadFiles`` must be set.

  The ``loadFileProperties`` are common for all files. If different properties
  need to be associated with files of different types, downloadFromURL must
  be called for each.
  """
  import SampleData
  sampleDataLogic = SampleData.SampleDataLogic()

  try:
    from ipywidgets import IntProgress
    from IPython.display import display
    progress = IntProgress()
  except ImportError:
    progress = None

  def reporthook(msg, level=None):
    # Download will only account for 90 percent of the time
    # (10% is left for loading time).
    progress.value=sampleDataLogic.downloadPercent*0.9

  if progress:
    sampleDataLogic.logMessage = reporthook
    display(progress) # show progress bar

  computeFileNames = not fileNames
  computeNodeNames = not nodeNames
  if computeFileNames or computeNodeNames:
    urisList = uris if type(uris) == list else [uris]
    if computeFileNames:
      fileNames = []
    else:
      filenamesList = fileNames if type(fileNames) == list else [fileNames]
    if computeNodeNames:
      nodeNames = []
    else:
      nodeNamesList = nodeNames if type(nodeNamesList) == list else [nodeNames]
    import os
    for index, uri in enumerate(urisList):
      if computeFileNames:
        fileName = getFileNameFromURL(uri)
        fileNames.append(fileName)
      else:
        fileName = fileNames[index]
      if computeNodeNames:
        fileNameWithoutExtension, _ = os.path.splitext(fileName)
        nodeNames.append(fileNameWithoutExtension)

  if type(uris) != list:
    if type(fileNames) == list:
      fileNames = fileNames[0]
    if type(nodeNames) == list:
      nodeNames = nodeNames[0]

  downloaded = sampleDataLogic.downloadFromURL(uris, fileNames, nodeNames, checksums, loadFiles,
    customDownloader, loadFileTypes, loadFileProperties)

  if progress:
    progress.layout.display = 'none' # hide progress bar

  return downloaded[0] if len(downloaded) == 1 else downloaded

def localPath(filename=None):
  """Create a full path from a filename, relative to the notebook location.
  This is useful for creating file path for saving nodes.

  Example: `slicer.util.saveNode(modelNode, slicernb.localPath("MyModel.ply"))`
  """
  import os
  notebookDir = slicer.app.startupWorkingPath
  if not filename:
    return notebookDir
  else:
    return os.path.join(notebookDir, filename)

def notebookPath(verbose=False):
  """Returns the absolute path of the Notebook.
  """
  from jupyter_server import serverapp as app
  import json
  import os
  import urllib
  connection_file = os.path.basename(slicer.modules.jupyterkernel.connectionFile)
  kernel_id = connection_file.split('-', 1)[1].split('.')[0]
  for srv in app.list_running_servers():
    try:
      if srv['token']=='' and not srv['password']:  # No token and no password, ahem...
        req = urllib.request.urlopen(srv['url']+'api/sessions')
      else:
        req = urllib.request.urlopen(srv['url']+'api/sessions?token='+srv['token'])
      sessions = json.load(req)
      for sess in sessions:
        if sess['kernel']['id'] == kernel_id:
          return os.path.join(srv['root_dir'],sess['notebook']['path'])
    except:
     pass  # There may be stale entries in the runtime directory
  return None

def notebookSaveCheckpoint():
    """Save a checkpoint of current notebook. Returns True on success."""
    try:
      from IPython.display import Javascript
      from IPython.display import display
    except ModuleNotFoundError:
      import logging
      logging.error("notebookSaveCheckpoint requires ipywidgets. It can be installed by running this command:\n\n    pip_install('ipywidgets')\n")
      return False

    script = '''
    require(["base/js/namespace"],function(Jupyter) {
        Jupyter.notebook.save_checkpoint();
    });
    '''
    display(Javascript(script))
    return True

def notebookExportToHtml(outputFilePath=None):
    """Export current notebook to HTML.
    If outputFilePath is not specified then filename will be generated from the notebook filename
    with current timestamp appended.
    It returns full path of the saved html file.
    It requires nbformat and nbconvert packages. You can use this command to install them::

      pip_install("nbformat nbconvert")

    """
    try:
      import nbformat
      from nbconvert import HTMLExporter
    except ModuleNotFoundError:
      import logging
      logging.error("notebookExportToHtml requires nbformat and nbconvert. They can be installed by running this command:\n\n    pip_install('nbformat nbconvert')\n")

    import datetime, json, os

    notebook_path = notebookPath()

    # Generate output file path from notebook name and timestamp (if not specified)
    if not outputFilePath:
      this_notebook_name = os.path.splitext(os.path.basename(notebook_path))[0]
      save_timestamp = datetime.datetime.now().strftime('%Y-%m-%d_%H-%M-%S')
      save_file_name = this_notebook_name + "_" + save_timestamp + ".html"
      notebooks_save_path = os.path.dirname(notebook_path)
      outputFilePath = os.path.join(notebooks_save_path, save_file_name)

    with open(notebook_path, mode="r") as f:
        file_json = json.load(f)

    notebook_content = nbformat.reads(json.dumps(file_json), as_version=4)

    html_exporter = HTMLExporter()
    (body, resources) = html_exporter.from_notebook_node(notebook_content)

    f = open(outputFilePath, 'wb')
    f.write(body.encode())
    f.close()

    return outputFilePath

def installExtensions(extensionNames):
  """Download and install extensions. All extensions required by the listed extensions
  will be automatically installed, too.
  :param extensionNames: list of strings containing the extension names.
  """
  success = True
  import logging
  emm = slicer.app.extensionsManagerModel()
  if hasattr(emm,'interactive'):
    # Disable popups asking to confirm installation of required extensions,
    # as a popup would block the application.
    emm.interactive = False
  installedExtensions = []
  failedToInstallExtensions = []
  notFoundExtensions = []
  for extensionName in extensionNames:
    if emm.isExtensionInstalled(extensionName):
      continue
    extensionMetaData = emm.retrieveExtensionMetadataByName(extensionName)
    if slicer.app.majorVersion*100+slicer.app.minorVersion < 413:
      # Slicer-4.11
      if not extensionMetaData or 'item_id' not in extensionMetaData:
        logging.debug(f"{extensionName} extension was not found on Extensions Server")
        notFoundExtensions.append(extensionName)
        continue
      itemId = extensionMetaData['item_id']
      url = f"{emm.serverUrl().toString()}/download?items={itemId}"
    else:
      # Slicer-4.13
      if not extensionMetaData or '_id' not in extensionMetaData:
        logging.debug(f"{extensionName} extension was not found on Extensions Server")
        notFoundExtensions.append(extensionName)
        continue
      itemId = extensionMetaData['_id']
      url = f"{emm.serverUrl().toString()}/api/v1/item/{itemId}/download"
    extensionPackageFilename = slicer.app.temporaryPath+'/'+itemId
    try:
      slicer.util.downloadFile(url, extensionPackageFilename)
    except:
      logging.debug(f"{extensionName} download failed from {url}")
      failedToInstallExtensions.append(extensionName)
      continue
    if not emm.installExtension(extensionPackageFilename):
      logging.debug(f"{extensionName} install failed")
      failedToInstallExtensions.append(extensionName)
      continue
    installedExtensions.append(extensionName)

  if notFoundExtensions:
    logging.warning("Extensions not found: " + ", ".join(notFoundExtensions))
    success = False
  if failedToInstallExtensions:
    logging.warning("Extensions failed to install: " + ", ".join(failedToInstallExtensions))
    success = False
  if installedExtensions:
    print("Extensions installed: " + ", ".join(installedExtensions))
    logging.warning("Restart the kernel to make the installed extensions available in this notebook.")

  return success
