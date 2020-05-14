# TODO: separate widgets (can be inserted into layouts) from functions (providing data)

# views (widgets and rendering utils)
from .display import displayViews, displaySliceView, display3DView, displayViewLightbox
from .display import setViewLayout, showSliceViewAnnotations, showVolumeRendering, reset3DView

# mrml (convert MRML nodes to data types that has nice visualization in notebooks)
from .display import displayModel, displayTable, displayMarkups, displayTransform, displayNode

# cli
from .cli import cliRunSync

# util (file management, useful widgets)
from .files import downloadFromURL, localPath, notebookPath, installExtensions

# widgets
try:
    import ipywidgets
except ImportError:
    pass
else:
    from .widgets import SliceViewWidget, SliceViewBaseWidget, ThreeDViewWidget, FileUploadWidget, AppWindow
    from .interactive_view_widget import InteractiveViewWidget
