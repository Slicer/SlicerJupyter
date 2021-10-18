/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

#ifndef __qSlicerJupyterKernelModule_h
#define __qSlicerJupyterKernelModule_h

// SlicerQt includes
#include "qSlicerLoadableModule.h"

#include "qSlicerJupyterKernelModuleExport.h"

class qSlicerJupyterKernelModulePrivate;

/// \ingroup Slicer_QtModules_ExtensionTemplate
class Q_SLICER_QTMODULES_JUPYTERKERNEL_EXPORT
qSlicerJupyterKernelModule
  : public qSlicerLoadableModule
{
  Q_OBJECT
#ifdef Slicer_HAVE_QT5
  Q_PLUGIN_METADATA(IID "org.slicer.modules.loadable.qSlicerLoadableModule/1.0");
#endif
  Q_INTERFACES(qSlicerLoadableModule);
  Q_PROPERTY(double pollIntervalSec READ pollIntervalSec WRITE setPollIntervalSec)
  Q_PROPERTY(QString connectionFile READ connectionFile)
  Q_PROPERTY(bool internalJupyterServerRunning READ isInternalJupyterServerRunning)
public:

  typedef qSlicerLoadableModule Superclass;
  explicit qSlicerJupyterKernelModule(QObject *parent=0);
  virtual ~qSlicerJupyterKernelModule();

  qSlicerGetTitleMacro(QTMODULE_TITLE);

  QString helpText()const override;
  QString acknowledgementText()const override;
  QStringList contributors()const override;

  QIcon icon()const override;

  QStringList categories()const override;
  QStringList dependencies()const override;

  Q_INVOKABLE virtual bool updateKernelSpec();

  /// Get path where KernelSpec is created.
  Q_INVOKABLE virtual QString kernelSpecPath();

  Q_INVOKABLE virtual bool slicerKernelSpecInstallCommandArgs(QString& executable, QStringList& args);

  /// Install Jupyter server in Slicer's Python environment
  Q_INVOKABLE virtual bool installInternalJupyterServer();

  /// Start Jupyter server in Slicer's Python environment.
  /// Set detached=false to run the server as a child process and shutdown along with the application.
  /// Set classic=true to launch a classic notebook server instead of JupyterLab.
  Q_INVOKABLE virtual bool startInternalJupyterServer(QString notebookDirectory, bool detached=true, bool classic=false);

  /// Stop Jupyter server in Slicer's Python environment.
  /// Currently it does not work (it only terminates the launcher and not the actual application).
  /// In the future, this method will be fixed or removed.
  Q_INVOKABLE virtual bool stopInternalJupyterServer();

  /// Returns true if internal Jupyter server is successfully started and still running.
  /// Only applicable to server that is started with detached=false.
  bool isInternalJupyterServerRunning() const;

  /// Deprecated. Use kernelSpecPath() instead.
  Q_INVOKABLE virtual QString resourceFolderPath();

  double pollIntervalSec();

  QString connectionFile();

public slots:

  void startKernel(const QString& connectionFile);
  void stopKernel();
  void setPollIntervalSec(double intervalSec);

signals:
  // Called after kernel has successfully started
  void kernelStarted();

  // Called when Jupyter requested stopping of the kernel.
  void kernelStopRequested();

protected:

  /// Initialize the module. Register the volumes reader/writer
  void setup() override;

  /// Create and return the widget representation associated to this module
  qSlicerAbstractModuleRepresentation * createWidgetRepresentation() override;

  /// Create and return the logic associated to this module
  vtkMRMLAbstractLogic* createLogic() override;

protected:
  QScopedPointer<qSlicerJupyterKernelModulePrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerJupyterKernelModule);
  Q_DISABLE_COPY(qSlicerJupyterKernelModule);

};

#endif
