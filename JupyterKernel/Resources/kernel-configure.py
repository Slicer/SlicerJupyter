import sys
import distutils.spawn
sys.executable = distutils.spawn.find_executable('python-real') or distutils.spawn.find_executable('python')

class displayModel():
  def __init__(self, modelNode, imageSize=None, orientation=None, zoom=None, showFeatureEdges=False):
    self.modelNode = modelNode
    # rollPitchYawDeg
    self.orientation = [0,0,0] if orientation is None else orientation
    self.zoom = 1.0 if zoom is None else zoom
    self.imageSize = [300,300] if imageSize is None else imageSize
    self.showFeatureEdges = showFeatureEdges

  def _repr_mimebundle_(self, include=None, exclude=None):
    modelPolyData = self.modelNode.GetPolyData()

    renderer = vtk.vtkRenderer()
    renderer.SetBackground(1,1,1)
    renderer.SetUseDepthPeeling(1)
    renderer.SetMaximumNumberOfPeels(100)
    renderer.SetOcclusionRatio(0.1)
    renWin = vtk.vtkRenderWindow()
    renWin.OffScreenRenderingOn()
    renWin.SetSize(self.imageSize[0], self.imageSize[1])
    renWin.SetAlphaBitPlanes(1); # for depth peeling
    renWin.SetMultiSamples(0); # for depth peeling
    renWin.AddRenderer(renderer)

    # Must be called after iren and renderer are linked or there will be problems
    renderer.Render()

    modelNormals = vtk.vtkPolyDataNormals()
    modelNormals.SetInputData(modelPolyData)

    modelMapper = vtk.vtkPolyDataMapper()
    modelMapper.SetInputConnection(modelNormals.GetOutputPort())

    modelActor = vtk.vtkActor()
    modelActor.SetMapper(modelMapper)
    modelActor.GetProperty().SetColor(0.9, 0.9, 0.9)
    modelActor.GetProperty().SetOpacity(0.8)
    renderer.AddActor(modelActor)

    triangleFilter = vtk.vtkTriangleFilter()
    triangleFilter.SetInputConnection(modelNormals.GetOutputPort())

    if self.showFeatureEdges:

      edgeExtractor = vtk.vtkFeatureEdges()
      edgeExtractor.SetInputConnection(triangleFilter.GetOutputPort())
      edgeExtractor.ColoringOff()
      edgeExtractor.BoundaryEdgesOn()
      edgeExtractor.ManifoldEdgesOn()
      edgeExtractor.NonManifoldEdgesOn()

      modelEdgesMapper = vtk.vtkPolyDataMapper()
      modelEdgesMapper.SetInputConnection(edgeExtractor.GetOutputPort())
      modelEdgesMapper.SetResolveCoincidentTopologyToPolygonOffset()
      modelEdgesActor = vtk.vtkActor()
      modelEdgesActor.SetMapper(modelEdgesMapper)
      modelEdgesActor.GetProperty().SetColor(0.0, 0.0, 0.0)
      renderer.AddActor(modelEdgesActor)

    # Set projection to parallel to enable estimate distances
    renderer.GetActiveCamera().ParallelProjectionOn()
    renderer.GetActiveCamera().Roll(self.orientation[0])
    renderer.GetActiveCamera().Pitch(self.orientation[1])
    renderer.GetActiveCamera().Yaw(self.orientation[2])
    renderer.ResetCamera()
    renderer.GetActiveCamera().Zoom(self.zoom)

    windowToImageFilter = vtk.vtkWindowToImageFilter()
    windowToImageFilter.SetInput(renWin)
    windowToImageFilter.Update()

    screenshot = ctk.ctkVTKWidgetsUtils.vtkImageDataToQImage(windowToImageFilter.GetOutput())

    bArray = qt.QByteArray()
    buffer = qt.QBuffer(bArray)
    buffer.open(qt.QIODevice.WriteOnly)
    screenshot.save(buffer, "PNG")
    dataValue = bArray.toBase64().data().decode()
    dataType = "image/png"

    return { dataType: dataValue }

def displayTable(tableNode):
  """Display table node by converting to pandas dataframe object"""
  return slicer.util.dataframeFromTable(tableNode)

def displayMarkups(markupsNode):
  """Display markups node by converting to pandas dataframe object"""
  return slicer.util.dataframeFromMarkups(markupsNode)

class displayTransform():
  """This class displays information about a transform in a Jupyter notebook cell.
  """
  def __init__(self, transform):
    if transform.IsLinear():
      # Always print linear transforms as transform to parent matrix
      self.dataValue = "Transform to parent:<br><pre>"+np.array2string(slicer.util.arrayFromTransformMatrix(transform))+"</pre>"
    else:
      # Non-linear transform
      if transform.GetTransformToParentAs('vtkAbstractTransform', False, True):
        # toParent is set (fromParent is just computed)
        self.dataValue = 'Transform to parent:<br>'
        self.dataValue += transform.GetTransformToParentInfo().replace('\n','<br>')
      else:
        # fromParent is set (toParent is just computed)
        self.dataValue = 'Transform from parent:<br>'
        self.dataValue += transform.GetTransformFromParentInfo().replace('\n','<br>')
  def _repr_mimebundle_(self, include=None, exclude=None):
    return { "text/html": self.dataValue }

def autoDisplayNode(node):
  if not hasattr(node, "IsA"):
    # not a MRML node
    return None

  try:

    if node.IsA("vtkMRMLMarkupsNode"):
      return displayMarkups(node)
    elif node.IsA("vtkMRMLModelNode"):
      return displayModel(node)
    elif node.IsA("vtkMRMLTableNode"):
      return displayTable(node)
    elif node.IsA("vtkMRMLTransformNode"):
      return displayTransform(node)

  except:
    # Error occurred, most likely the input was not a MRML node
    return None

class displayViews:
  """This class captures current views and makes it available
  for display in the output of a Jupyter notebook cell.
  :param viewLayout: FourUp, Conventional, OneUp3D, OneUpRedSlice,
    OneUpYellowSlice, OneUpGreenSlice, Compare, Dual3D,
    ConventionalWidescreen, CompareWidescreen, Triple3DEndoscopy,
    ThreeOverThree, FourOverFour, CompareGrid, TwoOverTwo,
    SideBySide, FourByThreeSlice, FourByTwoSlice, FiveByTwoSlice,
    ThreeByThreeSlice, FourUpTable, 3DTable, ConventionalPlot,
    FourUpPlot, FourUpPlotTable, OneUpPlot, ThreeOverThreePlot,
    DicomBrowser
  :param center: re-center slice and 3D views
  """
  def __init__(self, viewLayout=None, center=True):
    self.viewLayout = viewLayout
    self.center = center
  def _repr_mimebundle_(self, include=None, exclude=None):
    layoutManager = slicer.app.layoutManager()
    if self.viewLayout:
      setViewLayout(self.viewLayout)
    if self.center:
      slicer.util.resetSliceViews()
      for viewId in range(layoutManager.threeDViewCount):
        slicer.nb.reset3DView(viewId)
    slicer.util.setViewControllersVisible(False)
    slicer.app.processEvents()
    slicer.util.forceRenderAllViews()
    screenshot = layoutManager.viewport().grab()
    slicer.util.setViewControllersVisible(True)
    bArray = qt.QByteArray()
    buffer = qt.QBuffer(bArray)
    buffer.open(qt.QIODevice.WriteOnly)
    screenshot.save(buffer, "PNG")
    dataValue = bArray.toBase64().data().decode()
    dataType = "image/png"
    return { dataType: dataValue }

class displaySliceView():
  """This class captures a slice view and makes it available
  for display in the output of a Jupyter notebook cell.
  """
  def __init__(self, viewName=None, positionPercent=None):
    self.viewName = viewName if viewName else "Red"
    self.positionPercent = positionPercent
  def _repr_mimebundle_(self, include=None, exclude=None):
    layoutManager = slicer.app.layoutManager()
    slicer.app.processEvents()
    sliceWidget = layoutManager.sliceWidget(self.viewName)
    if self.positionPercent is not None:
      sliceBounds = [0,0,0,0,0,0]
      sliceWidget.sliceLogic().GetLowestVolumeSliceBounds(sliceBounds)
      positionMin = sliceBounds[4]
      positionMax = sliceBounds[5]
      position = positionMin + self.positionPercent / 100.0 * (positionMax - positionMin)
      sliceController=sliceWidget.sliceController().sliceOffsetSlider().setValue(position)
    sliceView = sliceWidget.sliceView()
    sliceView.forceRender()
    screenshot = sliceView.grab()
    bArray = qt.QByteArray()
    buffer = qt.QBuffer(bArray)
    buffer.open(qt.QIODevice.WriteOnly)
    #screenshot.save(buffer, "PNG")
    screenshot.save(buffer, "JPG")
    dataValue = bArray.toBase64().data().decode()
    #dataType = "image/png"
    dataType = "image/jpeg"
    return { dataType: dataValue }

class display3DView():
  """This class captures a 3D view and makes it available
  for display in the output of a Jupyter notebook cell.
  """
  def __init__(self, viewID=0, orientation=None):
    self.viewID = viewID
    self.orientation = orientation
  def _repr_mimebundle_(self, include=None, exclude=None):
    slicer.app.processEvents()
    widget = slicer.app.layoutManager().threeDWidget(self.viewID)
    view = widget.threeDView()
    if self.orientation is not None:
      camera = view.interactorStyle().GetCameraNode().GetCamera()
      cameraToWorld = vtk.vtkTransform()
      cameraToWorld.RotateX(90)
      cameraToWorld.RotateY(180)
      cameraToWorld.RotateY(self.orientation[2])
      cameraToWorld.RotateX(self.orientation[1])
      cameraToWorld.RotateZ(self.orientation[0])
      cameraToWorld.Translate(0, 0, camera.GetDistance())
      viewUp = [0,1,0,0]
      slicer.vtkAddonMathUtilities.GetOrientationMatrixColumn(cameraToWorld.GetMatrix(), 1, viewUp)
      position = cameraToWorld.GetPosition()
      focalPoint = camera.GetFocalPoint()
      camera.SetPosition(focalPoint[0]+position[0], focalPoint[1]+position[1], focalPoint[2]+position[2])
      camera.SetViewUp(viewUp[0:3])
      camera.OrthogonalizeViewUp()
    view.forceRender()
    screenshot = view.grab()
    bArray = qt.QByteArray()
    buffer = qt.QBuffer(bArray)
    buffer.open(qt.QIODevice.WriteOnly)
    #screenshot.save(buffer, "PNG")
    screenshot.save(buffer, "JPG")
    dataValue = bArray.toBase64().data().decode()
    #dataType = "image/png"
    dataType = "image/jpeg"
    return { dataType: dataValue }

def downloadFromURL(uris=None, fileNames=None, nodeNames=None, checksums=None, loadFiles=None,
  customDownloader=None, loadFileTypes=None, loadFileProperties={}):
  """Download data from custom URL with progress bar.
  See API description in SampleData.downloadFromURL.
  """
  import SampleData
  sampleDataLogic = SampleData.SampleDataLogic()

  from ipywidgets import IntProgress
  from IPython.display import display
  progress = IntProgress()

  def reporthook(msg, level=None):
    # Download will only account for 90 percent of the time
    # (10% is left for loading time).
    progress.value=sampleDataLogic.downloadPercent*0.9

  sampleDataLogic.logMessage = reporthook

  display(progress) # show progress bar
  downloaded = sampleDataLogic.downloadFromURL(uris, fileNames, nodeNames, checksums, loadFiles,
    customDownloader, loadFileTypes, loadFileProperties)
  progress.layout.display = 'none' # hide progress bar

  return downloaded

def showVolumeRendering(volumeNode, presetName=None):
  volRenLogic = slicer.modules.volumerendering.logic()
  displayNode = volRenLogic.CreateDefaultVolumeRenderingNodes(volumeNode)
  displayNode.SetVisibility(True)
  scalarRange = volumeNode.GetImageData().GetScalarRange()
  if not presetName:
    if scalarRange[1]-scalarRange[0] < 1500:
      # small dynamic range, probably MRI
      presetName = 'MR-Default'
    else:
      # larger dynamic range, probably CT
      presetName = 'CT-Chest-Contrast-Enhanced'
  displayNode.GetVolumePropertyNode().Copy(volRenLogic.GetPresetByName(presetName))

def reset3DView(viewID=0):
  threeDWidget = slicer.app.layoutManager().threeDWidget(viewID)
  threeDView = threeDWidget.threeDView()
  threeDView.resetFocalPoint()

def setViewLayout(layoutName):
  layoutId = eval("slicer.vtkMRMLLayoutNode.SlicerLayout"+layoutName+"View")
  slicer.app.layoutManager().setLayout(layoutId)

def localPath(filename=None):
  import os
  notebookDir = os.path.dirname(slicer.nb.notebookPath())
  if not filename:
    return notebookDir
  else:
    return os.path.join(notebookDir, filename)

def notebookPath(connection_file=connection_file, verbose=False):
  """Returns the absolute path of the Notebook or None if it cannot be determined
  NOTE: works only when the security is token-based or there is also no password.
  """
  # Code is extracted from notebook\notebookapp.py to avoid requiring installation of notebook on server

  import urllib.request
  import json
  import io, os, re

  def check_pid(pid):

    def _check_pid_win32(pid):
      import ctypes
      # OpenProcess returns 0 if no such process (of ours) exists
      # positive int otherwise
      return bool(ctypes.windll.kernel32.OpenProcess(1,0,pid))

    def _check_pid_posix(pid):
      """Copy of IPython.utils.process.check_pid"""
      try:
        os.kill(pid, 0)
      except OSError as err:
        if err.errno == errno.ESRCH:
          return False
        elif err.errno == errno.EPERM:
          # Don't have permission to signal the process - probably means it exists
          return True
        raise
      else:
        return True

    if sys.platform == 'win32':
      return _check_pid_win32(pid)
    else:
      return _check_pid_posix(pid)

  def list_running_servers():
      # Iterate over the server info files of running notebook servers.
      # Given a runtime directory, find nbserver-* files in the security directory,
      # and yield dicts of their information, each one pertaining to
      # a currently running notebook server instance.
      runtime_dir = os.path.dirname(connection_file)

      for file_name in os.listdir(runtime_dir):
        if not re.match('nbserver-(.+).json', file_name):
          continue
        if verbose: print(file_name)
        with io.open(os.path.join(runtime_dir, file_name), encoding='utf-8') as f:
          info = json.load(f)
        # Simple check whether that process is really still running
        if ('pid' in info) and check_pid(info['pid']):
          yield info

  kernel_id = connection_file.split('-', 1)[1].split('.')[0]

  for srv in list_running_servers():
    try:
      if verbose: print(srv)
      if srv['token']=='' and not srv['password']:  # No token and no password, ahem...
        req = urllib.request.urlopen(srv['url']+'api/sessions')
      else:
        req = urllib.request.urlopen(srv['url']+'api/sessions?token='+srv['token'])
      sessions = json.load(req)
      if verbose: print("sessions: "+repr(sessions))
      for sess in sessions:
        if sess['kernel']['id'] == kernel_id:
          return os.path.join(srv['notebook_dir'],sess['notebook']['path'])
    except Exception as e:
      if verbose: print("exception: "+str(e))
      pass  # There may be stale entries in the runtime directory

  return None


class FileUploadWidget(object):
    def __init__(self):
        self.path = None
        self.filename = None

        from ipywidgets import FileUpload
        self.widget = FileUpload()
        self.widget.observe(self._handle_upload, names='data')

    def _repr_mimebundle_(self, include=None, exclude=None):
      return self.widget

    def _handle_upload(self, change=None, a=None, b=None):
        import os
        metadata = self.widget.metadata[0]
        self.filename = metadata['name']
        baseDir = os.path.dirname(slicer.nb.notebookPath())
        self.path = os.path.join(baseDir, self.filename)
        content = self.widget.value[self.filename]['content']
        with open(self.path, 'wb') as f: f.write(content)
        print('Uploaded {0} ({1} bytes)'.format(self.filename, metadata['size']))

class InteractiveView(object):

  def __init__(self, renderView):
    from ipyevents import Event
    from ipycanvas import Canvas

    self.renderView = renderView

    # Quality vs performance
    self.compressionQuality = 50
    self.trackMouseMove = False  # refresh if mouse is just moving (not dragging)

    # If not receiving new rendering request for 10ms then a render is requested
    self.fullRenderRequestTimer = qt.QTimer()
    self.fullRenderRequestTimer.setSingleShot(True)
    self.fullRenderRequestTimer.setInterval(500)
    self.fullRenderRequestTimer.connect('timeout()', self.fullRender)

    # If not receiving new rendering request for 10ms then a render is requested
    self.quickRenderRequestTimer = qt.QTimer()
    self.quickRenderRequestTimer.setSingleShot(True)
    self.quickRenderRequestTimer.setInterval(20)
    self.quickRenderRequestTimer.connect('timeout()', self.quickRender)
    
    # Get image size
    image = self.getImage()
    self.canvas = Canvas(width=image.width, height=image.height)
    self.canvasHeight=int(image.height)
    self.canvas.draw_image(image)

    self.interactor = self.renderView.interactorStyle().GetInteractor()
    
    self.dragging=False
    
    self.interactionEvents = Event()
    self.interactionEvents.source = self.canvas
    self.interactionEvents.watched_events = [
      'dragstart', 'mouseenter', 'mouseleave',
      'mousedown', 'mouseup', 'mousemove',
      #'wheel',  # commented out so that user can scroll through the notebook using mousewheel
      'keyup', 'keydown'
      ]
    #self.interactionEvents.msg_throttle = 1  # does not seem to have effect
    self.interactionEvents.prevent_default_action = True
    self.interactionEvents.on_dom_event(self.handleInteractionEvent)

    self.keyToSym = {
      'ArrowLeft': 'Left',
      'ArrowRight': 'Right',
      'ArrowUp': 'Up',
      'ArrowDown': 'Down',
      # TODO: more key codes could be added
      }

    # Errors are not displayed when a widget is displayed,
    # this variable can be used to retrieve error messages
    self.error = None

    # Enable logging of UI events
    self.logEvents = False
    self.loggedEvents = []
      
  def getImage(self, compress=True, forceRender=True):
    from ipywidgets import Image
    slicer.app.processEvents()
    if forceRender:
      self.renderView.forceRender()
    screenshot = self.renderView.grab()
    bArray = qt.QByteArray()
    buffer = qt.QBuffer(bArray)
    buffer.open(qt.QIODevice.WriteOnly)
    if compress:
      screenshot.save(buffer, "JPG", self.compressionQuality)
    else:
      screenshot.save(buffer, "PNG")
    return Image(value=bArray.data(), width=screenshot.width(), height=screenshot.height())

  def fullRender(self):
    self.fullRenderRequestTimer.stop()
    self.quickRenderRequestTimer.stop()
    self.canvas.draw_image(self.getImage(compress=False, forceRender=True))

  def quickRender(self):
    self.fullRenderRequestTimer.stop()
    self.quickRenderRequestTimer.stop()
    self.canvas.draw_image(self.getImage(compress=True, forceRender=False))
    self.fullRenderRequestTimer.start()

  def updateInteractorEventData(self, event):
    if event['event']=='keydown' or event['event']=='keyup':
      key = event['key']
      sym = self.keyToSym[key] if key in self.keyToSym.keys() else key
      self.interactor.SetKeySym(sym)
      if len(key) == 1:
        self.interactor.SetKeyCode(key)
      self.interactor.SetRepeatCount(1)
    else:
      self.interactor.SetEventPosition(event['offsetX'], self.canvasHeight-event['offsetY'])
    self.interactor.SetShiftKey(event['shiftKey'])
    self.interactor.SetControlKey(event['ctrlKey'])
    self.interactor.SetAltKey(event['altKey'])

  def handleInteractionEvent(self, event):
    try:
      if self.logEvents:
        self.loggedEvents.append(event)
      renderNow=True
      if event['event']=='mousemove':
        if not self.dragging and not self.trackMouseMove:
          return
        self.updateInteractorEventData(event)
        self.interactor.MouseMoveEvent()
        self.quickRenderRequestTimer.start()
      elif event['event']=='mouseenter':
        self.updateInteractorEventData(event)
        self.interactor.EnterEvent()
        self.quickRenderRequestTimer.start()
      elif event['event']=='mouseleave':
        self.updateInteractorEventData(event)
        self.interactor.LeaveEvent()
        self.quickRenderRequestTimer.start()
      elif event['event']=='mousedown':
        self.dragging=True
        self.updateInteractorEventData(event)
        if event['button'] == 0:
          self.interactor.LeftButtonPressEvent()
        elif event['button'] == 2:
          self.interactor.RightButtonPressEvent()
        elif event['button'] == 1:
          self.interactor.MiddleButtonPressEvent()
        self.fullRender()
      elif event['event']=='mouseup':
        self.updateInteractorEventData(event)
        if event['button'] == 0:
          self.interactor.LeftButtonReleaseEvent()
        elif event['button'] == 2:
          self.interactor.RightButtonReleaseEvent()
        elif event['button'] == 1:
          self.interactor.MiddleButtonReleaseEvent()
        self.dragging=False
        self.fullRender()
      elif event['event']=='keydown':
        self.updateInteractorEventData(event)
        self.interactor.KeyPressEvent()
        self.interactor.CharEvent()
        self.fullRender()
      elif event['event']=='keyup':
        self.updateInteractorEventData(event)
        self.interactor.KeyReleaseEvent()
        self.fullRender()
    except Exception as e:
        self.error = str(e)


def cliRunSync(module, node=None, parameters=None, delete_temporary_files=True, update_display=True):

  node = slicer.cli.run(module, node=node, parameters=parameters, wait_for_completion=False,
    delete_temporary_files=delete_temporary_files, update_display=update_display)
  from ipywidgets import IntProgress
  from IPython.display import display
  import time

  progress = IntProgress()
  display(progress) # display progress bar
  while node.IsBusy():
      progress.value = node.GetProgress()
      slicer.app.processEvents()
      time.sleep(.3)
  progress.layout.display = 'none' # hide progress bar

  return node

# empty object to store Slicer notebook helper functions
slicer.nb = type('', (), {})()

slicer.nb.displayViews = displayViews
slicer.nb.displaySliceView = displaySliceView
slicer.nb.display3DView = display3DView
slicer.nb.InteractiveView = InteractiveView

slicer.nb.displayMarkups = displayMarkups
slicer.nb.displayModel = displayModel
slicer.nb.displayTable = displayTable
slicer.nb.displayTransform = displayTransform
slicer.nb.autoDisplayNode =  autoDisplayNode

slicer.nb.showVolumeRendering = showVolumeRendering
slicer.nb.reset3DView = reset3DView
slicer.nb.setViewLayout = setViewLayout
slicer.nb.downloadFromURL = downloadFromURL
slicer.nb.notebookPath = notebookPath
slicer.nb.localPath = localPath
slicer.nb.cliRunSync = cliRunSync

slicer.nb.FileUploadWidget = FileUploadWidget

# Set up display hook to display execution result
# as raw text in Slicer's Python console and as
# rich output in Jupyter notebook.

def slicerDisplayHook(value):
  # show raw output in Slicer's Python console
  pythonConsole = slicer.app.pythonConsole()
  if pythonConsole and (value is not None):
    pythonConsole.printOutputMessage(repr(value))

  # Convert Slicer data objects to objects that the notebook
  # can display better.
  nodeValue = slicer.nb.autoDisplayNode(value)
  if nodeValue is not None:
    value = nodeValue

  # forward value to xeus-python display hook
  slicer.xeusPythonDisplayHook(value)

import sys
sys.displayhook = slicerDisplayHook
