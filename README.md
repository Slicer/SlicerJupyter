# SlicerJupyter
Extension for 3D Slicer that allows the application to be used from Jupyter notebook

*This project is under active development. Its content, API and behavior may change at any time.*

# Usage

## Option 1. Run using Binder

You can use this option for a quick start. No installation or setup is needed, just click the link below and start using Slicer via Jupyter notebook in your web browser.

[![Binder](https://mybinder.org/badge.svg)](https://mybinder.org/v2/gh/slicer/SlicerNotebooks/master)

Whn you click on the link, Binder launches 3D Slicer with SlicerJupyter extension on their cloud servers. Binder is a free service and server resources are quite limited. Also, there is no interactive access to the graphical user interface. Therefore, this option is only recommended for testing, demos, or simple computations or visualizations.

## Option 2. Run Slicer and Jupyter on your own computer

### Setup

* [Install Python and Jupyter notebook](http://jupyter.org/install)
  * You can choose any Python version (2.x or 3.x.) and any bitness (32-bit or 64-bit)
  * Adding Python to your PATH environment variable or registering as default Python is not required.
* Install [3D Slicer](https://download.slicer.org/) and SlicerJupyter extension, restart Slicer
* Switch to JupyterKernel module in Slicer
* Select _Python Scripts folder_. This is the folder where _jupyter-kernelspec_ executable is located.
  * On Windows, by default it is a location such as _C:/Users/(username)/Anaconda3/Scripts_ or _c:\Users\(username)\AppData\Local\Programs\Python\Python37-32\Scripts_. If you use virtual environment, specify Script folder of your virtual environment.
* Create and install kernel by clicking _Install Slicer kernel in Jupyter_.
  * If you use Anaconda, then kernel installation may fail with the message _fatal error C1083: Cannot open include file: 'sys/un.h' ... ImportError: DLL load failed: The specified module could not be found._ It is due to [this issue](https://github.com/zeromq/pyzmq/issues/852) and a fix is to reinstall ZeroMQ: open a command prompt in your Scripts folder, type `pip uninstall pyzmq` (answer _Yes_ if asked) then type `pip install pyzmq`.
  * If automatic installation of the kernel fails for any reason, then kernel can be installed manually using this command: `jupyter-kernelspec install <kernelfolder>` (replace <kernelfolder> with the path that is shown in "Slicer kernel folder" field in JupyterKernel module in Slicer)
* Start Jupyter notebook. For example, by runnning _jupyter-notebook_ executable.

## Using Slicer from a notebook

* Create a new notebook, selecting _Slicer 4.x_ kernel (for example, _Slicer 4.9_). Jupyter will open a new Slicer instance automatically when kernel start is requested; and this instance will be closed when kernel shutdown is requested.

![Select Slicer kernel](doc/StartKernel.png)

* While the kernel is starting, "Kernel starting, please wait.." message is displayed. After a few ten seconds Slicer kernel should start.
* Add `display()` at the end of a cell to display Slicer viewer content in the notebook.
* Hit `Tab` key for auto-complete
* Hit `Shift`+`Tab` for showing documentation for a method (hit multiple times to show more details). Note: method name must be complete (you can use `Tab` key to complete the name) and the cursor must be inside the name or right after it (not in the parentheses). For example, type `slicer.util.getNode` and hit `Shift`+`Tab`.

![Hit Tab key to auto-complete](doc/AutoComplete.png)

![Hit Shift-Tab key to inspect](doc/Inspect.png)

## Example

You can get started by looking at [example Slicer notebooks here](https://github.com/Slicer/SlicerNotebooks) or looking at sample scripts below.

Load data and show an axial slice view:

<pre>
slicer.mrmlScene.Clear(False)
import SampleData
sampleDataLogic = SampleData.SampleDataLogic()
volume = sampleDataLogic.downloadCTChest()
slicer.app.layoutManager().setLayout(slicer.vtkMRMLLayoutNode.SlicerLayoutOneUpRedSliceView)
display()
</pre>

Create a surface mesh from the image:

<pre>
parameters = {}
parameters["InputVolume"] = volume.GetID()
parameters["Threshold"] = 220
outModel = slicer.vtkMRMLModelNode()
slicer.mrmlScene.AddNode( outModel )
parameters["OutputGeometry"] = outModel.GetID()
grayMaker = slicer.modules.grayscalemodelmaker
slicer.cli.runSync(grayMaker, None, parameters)
slicer.app.layoutManager().setLayout(slicer.vtkMRMLLayoutNode.SlicerLayoutOneUp3DView)
slicer.app.layoutManager().threeDWidget(0).threeDView().resetCamera()
display()
</pre>



# For developers

## Build instructions

* Build the extension against the newly built Slicer with Qt5 and VTK9 enabled.

* Install Jupyter

```
mkvirtualenv -p python3.5 jupyter_env  # Create and activare virtual environment

pip install jupyter
```

* Install kernel

```
jupyter-kernelspec install /tmp/SlicerJupyter-build/inner-build/share/Slicer-4.9/qt-loadable-modules/JupyterKernel/Slicer-4.9/ --replace --user
```

* Start notebook

```
workon jupyter_env
python -m jupyter notebook
```

## Special commands

These commands must be the last commands in a cell.

- `__kernel_debug_enable()`: enable detailed logging of all incoming Jupyter requests
- `__kernel_debug_disable()`: enable detailed logging of all incoming Jupyter requests
