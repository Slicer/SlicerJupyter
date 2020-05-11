import qt, slicer
from traitlets import CFloat, Unicode, Int, validate, observe
from ipywidgets import Image, FloatSlider, VBox, FileUpload, link

class SliceViewBaseWidget(Image):
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


class SliceViewWidget(VBox):
    def __init__(self, view=None, **kwargs):
        self.sliceView = SliceViewBaseWidget(view)
        super().__init__(children=[self.sliceView.offsetSlider, self.sliceView], **kwargs)


class ThreeDViewWidget(Image):
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
