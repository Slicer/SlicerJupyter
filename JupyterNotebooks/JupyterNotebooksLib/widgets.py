import qt, slicer
from traitlets import CFloat, Unicode, Int, validate, observe
from ipywidgets import Image, FloatSlider, VBox, FileUpload, link
from IPython.display import IFrame

class ViewSliceBaseWidget(Image):
    """This class captures a slice view and makes it available
    for display in the output of a Jupyter notebook cell.
    """

    offsetMin = CFloat(100.0, help="Max value").tag(sync=True)
    offsetMax = CFloat(0.0, help="Min value").tag(sync=True)
    offset = CFloat(0.0, help="Slice offset").tag(sync=True)
    viewName = Unicode(default_value='Red', help="Slice view name.").tag(sync=True)

    def __init__(self, view=None, **kwargs):
        if view:
            self.viewName=view

        self.offsetSlider = FloatSlider(description='Offset')

        self.updateImage()

        self._updateOffsetRange()
        self.l1 = link((self.offsetSlider, 'value'), (self, 'offset'))
        self.l2 = link((self, 'offsetMin'), (self.offsetSlider, 'min'))
        self.l3 = link((self, 'offsetMax'), (self.offsetSlider, 'max'))

        super().__init__(**kwargs)

    @observe('offset')
    def _propagate_offset(self, change):
        offset = change['new']
        sliceWidget = slicer.app.layoutManager().sliceWidget(self.viewName)
        sliceBounds = [0,0,0,0,0,0]
        sliceWidget.sliceController().sliceOffsetSlider().setValue(offset)
        self.updateImage()

    @observe('viewName')
    def _propagate_viewName(self, change):
        self._updateOffsetRange()
        self.updateImage()

    def _updateOffsetRange(self):
        sliceWidget = slicer.app.layoutManager().sliceWidget(self.viewName)
        sliceBounds = [0,0,0,0,0,0]
        sliceWidget.sliceLogic().GetLowestVolumeSliceBounds(sliceBounds)
        positionMin = sliceBounds[4]
        positionMax = sliceBounds[5]
        if self.offsetMin != positionMin:
            self.offsetMin = positionMin
        if self.offsetMax != positionMax:
            self.offsetMax = positionMax
        if self.offset < self.offsetMin:
            self.offset = self.offsetMin
        elif self.offset > self.offsetMax:
            self.offset = self.offsetMax

    def updateImage(self):
        if not self.viewName:
            return
        slicer.app.processEvents()
        sliceWidget = slicer.app.layoutManager().sliceWidget(self.viewName)
        sliceView = sliceWidget.sliceView()
        sliceView.forceRender()
        screenshot = sliceView.grab()
        bArray = qt.QByteArray()
        buffer = qt.QBuffer(bArray)
        buffer.open(qt.QIODevice.WriteOnly)
        screenshot.save(buffer, "PNG")
        self.value = bArray.data()


class ViewSliceWidget(VBox):
    def __init__(self, view=None, **kwargs):
        self.sliceView = ViewSliceBaseWidget(view)
        super().__init__(children=[self.sliceView.offsetSlider, self.sliceView], **kwargs)

class View3DWidget(Image):
    """This class captures a 3D view and makes it available
    for display in the output of a Jupyter notebook cell.
    """

    viewIndex = Int(default_value=0, help="3D view index.").tag(sync=True)

    def __init__(self, view=None, **kwargs):
        if view:
            self.viewIndex=view

        self.updateImage()

        super().__init__(**kwargs)

    @observe('viewIndex')
    def _propagate_viewIndex(self, change):
        self.updateImage()

    def updateImage(self):
        if self.viewIndex == None:
            return
        slicer.app.processEvents()
        widget = slicer.app.layoutManager().threeDWidget(self.viewIndex)
        view = widget.threeDView()
        view.forceRender()
        screenshot = view.grab()
        bArray = qt.QByteArray()
        buffer = qt.QBuffer(bArray)
        buffer.open(qt.QIODevice.WriteOnly)
        screenshot.save(buffer, "PNG")
        self.value = bArray.data()

class FileUploadWidget(FileUpload):
    def __init__(self, **kwargs):
        self.path = None
        self.filename = None
        self.observe(self._handle_upload, names='data')
        super().__init__(**kwargs)

    def _repr_mimebundle_(self, include=None, exclude=None):
      return self.widget

    def _handle_upload(self, change=None, a=None, b=None):
        import os
        metadata = self.widget.metadata[0]
        self.filename = metadata['name']
        baseDir = os.path.dirname(notebookPath())
        self.path = os.path.join(baseDir, self.filename)
        content = self.widget.value[self.filename]['content']
        with open(self.path, 'wb') as f: f.write(content)
        print('Uploaded {0} ({1} bytes)'.format(self.filename, metadata['size']))

class AppWindow(IFrame):
    """Shows interactive screen of a remote desktop session. Requires remte desktop view configured to be displayed at ../desktop URL.
    If multiple kernels are used then the sceen space is shared between them. Make application window full-screen and call `show()`
    to ensure that current window is on top.
    """
    def __init__(self, contents=None, windowScale=None, windowWidth=None, windowHeight=None, **kwargs):
        # Set default size to fill in notebook cell
        if kwargs.get('width', None) is None:
            kwargs['width'] = 960
        if kwargs.get('height', None) is None:
            kwargs['height'] = 768
        AppWindow.setWindowSize(windowWidth, windowHeight, windowScale)
        if contents is None:
            contents = "viewers"
        AppWindow.setContents(contents)
        AppWindow.show()
        super().__init__('../desktop', **kwargs)

    @staticmethod
    def setWindowSize(width=None, height=None, scale=None):
        if width is None:
            width = 1280
        if height is None:
            height = 1024
        if scale is not None:
            width *= scale
            height *= scale
        slicer.util.mainWindow().size = qt.QSize(width, height)

    @staticmethod
    def setContents(contents):
        if contents=="viewers":
            slicer.util.findChild(slicer.util.mainWindow(), "PanelDockWidget").hide()
            slicer.util.setStatusBarVisible(False)
            slicer.util.setMenuBarsVisible(False)
            slicer.util.setToolbarsVisible(False)
        elif contents=="full":
            slicer.util.findChild(slicer.util.mainWindow(), "PanelDockWidget").show()
            slicer.util.setStatusBarVisible(True)
            slicer.util.setMenuBarsVisible(True)
            slicer.util.setToolbarsVisible(True)
        else:
            raise ValueError("contents must be 'viewers' or 'full'")

    @staticmethod
    def show():
        slicer.util.mainWindow().raise_()
