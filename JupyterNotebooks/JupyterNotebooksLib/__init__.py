# TODO: separate widgets (can be inserted into layouts) from functions (providing data)

# Displayable objects from views
from .display import ViewDisplay, ViewSliceDisplay, View3DDisplay, ViewLightboxDisplay

# Rendering utility functions
from .display import setViewLayout, showSliceViewAnnotations, showVolumeRendering, reset3DView

# Convert MRML nodes and other common data types to objects that can be
# nicely displayed in notebooks
from .display import displayable, ModelDisplay, TransformDisplay, MatplotlibDisplay

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
    from .widgets import ViewSliceWidget, ViewSliceBaseWidget, View3DWidget, FileUploadWidget, AppWindow
    from .interactive_view_widget import ViewInteractiveWidget
