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

public:

  typedef qSlicerLoadableModule Superclass;
  explicit qSlicerJupyterKernelModule(QObject *parent=0);
  virtual ~qSlicerJupyterKernelModule();

  qSlicerGetTitleMacro(QTMODULE_TITLE);

  virtual QString helpText()const;
  virtual QString acknowledgementText()const;
  virtual QStringList contributors()const;

  virtual QIcon icon()const;

  virtual QStringList categories()const;
  virtual QStringList dependencies()const;

  Q_INVOKABLE virtual void updateKernelSpec();

  Q_INVOKABLE virtual bool installSlicerKernel(QString pythonScriptsFolder);
  Q_INVOKABLE virtual bool startJupyterNotebook(QString pythonScriptsFolder);

  Q_INVOKABLE virtual QString kernelFolderPath();

public slots:

  void startKernel(const QString& connectionFile);
  void stopKernel();

protected:

  /// Initialize the module. Register the volumes reader/writer
  virtual void setup();

  /// Create and return the widget representation associated to this module
  virtual qSlicerAbstractModuleRepresentation * createWidgetRepresentation();

  /// Create and return the logic associated to this module
  virtual vtkMRMLAbstractLogic* createLogic();

protected:
  QScopedPointer<qSlicerJupyterKernelModulePrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerJupyterKernelModule);
  Q_DISABLE_COPY(qSlicerJupyterKernelModule);

};

#endif
