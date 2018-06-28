# SlicerJupyter
Extension for 3D Slicer that allows the application to be used from Jupyter notebook

*This project is under active development. Its content, API and behavior may change at any time. We mean it.*

## Usage

* Build the extension against the newly built Slicer with Qt5 and VTK9 enabled.

* Install Jupyter

```
mkvirtualenv -p python3.5 jupyter_env  # Create and activare virtual environment

pip install jupyter
```

* Install kernel

```
cd /tmp/SlicerJupyter-build/inner-build\
jupyter-kernelspec install ./slicer_kernel/ --replace --user
```

* Start notebook

```
workon jupyter_env
python -m jupyter notebook
```

* Start Slicer kernel
