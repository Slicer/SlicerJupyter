# TODO: separate widgets (can be inserted into layouts) from functions (providing data)

# views (widgets and rendering utils)
from .display import displayViews, displaySliceView, display3DView, showVolumeRendering, reset3DView, setViewLayout

# mrml (convert MRML nodes to data types that has nice visualization in notebooks)
from .display import displayModel, displayTable, displayMarkups, displayTransform, displayNode

# cli
from .cli import cliRunSync

# util (file management, useful widgets)
from .files import downloadFromURL, localPath, notebookPath

# widgets
try:
    import ipywidgets
except ImportError:
    pass
else:
    from .widgets import SliceViewWidget, ThreeDViewWidget, FileUploadWidget
    from .interactive_view_widget import InteractiveViewWidget
