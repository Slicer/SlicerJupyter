import ctk, qt, slicer, vtk

from traitlets import Unicode

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

def displayNode(node):
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
        reset3DView(viewId)
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
      sliceWidget.sliceController().sliceOffsetSlider().setValue(position)
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
