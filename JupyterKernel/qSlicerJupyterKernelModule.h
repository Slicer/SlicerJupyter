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
  Q_PROPERTY(QString executeResultDataType READ executeResultDataType WRITE setExecuteResultDataType)
  Q_PROPERTY(QString executeResultDataValue READ executeResultDataValue WRITE setExecuteResultDataValue)

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

  Q_INVOKABLE virtual bool slicerKernelSpecInstallCommandArgs(QString& executable, QStringList& args);
  Q_INVOKABLE virtual bool installSlicerKernel(QString pythonScriptsFolder);
  Q_INVOKABLE virtual bool startJupyterNotebook(QString pythonScriptsFolder);

  Q_INVOKABLE virtual QString resourceFolderPath();

  QString executeResultDataType();
  void setExecuteResultDataType(const QString& str);
  QString executeResultDataValue();
  void setExecuteResultDataValue(const QString& str);
  
public slots:

  void startKernel(const QString& connectionFile);
  void stopKernel();

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
