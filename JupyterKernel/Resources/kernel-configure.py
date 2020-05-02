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

class displayViews:
  """This class captures current views and makes it available
  for display in the output of a Jupyter notebook cell.
  """
  def _repr_mimebundle_(self, include=None, exclude=None):
    layoutManager = slicer.app.layoutManager()
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

def autoDisplayNode(node):
  if not hasattr(node, "IsA"):
    # not a MRML node
    return None

  try:

    if node.IsA("vtkMRMLTableNode"):
      return displayTable(node)
    elif node.IsA("vtkMRMLModelNode"):
      return displayModel(node)

  except:
    # Error occurred, most likely the input was not a MRML node
    return None

# empty object to store Slicer notebook helper functions
slicer.nb = type('', (), {})()

slicer.nb.displayViews = displayViews
slicer.nb.displayModel = displayModel
slicer.nb.displayTable = displayTable
slicer.nb.autoDisplayNode =  autoDisplayNode

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
