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

// JupyterKernel Logic includes
#include <vtkSlicerJupyterKernelLogic.h>

// JupyterKernel includes
#include "qSlicerJupyterKernelModule.h"
#include "qSlicerJupyterKernelModuleWidget.h"

#include "qSlicerApplication.h"
#include "qSlicerPythonManager.h"

#include "PythonQt.h"

// On Windows, pyerrors.h redefines snprintf to _snprintf
// which causes error C2039: '_snprintf': is not a member of 'std'
// while trying to compile std::snprintf in json.hpp (in nlohmann_json).
// Until this is fixed in Python or made more robust in nlohman_json,
// fix this here by undefining snprintf.
#if defined(WIN32) && defined(snprintf)
  #undef snprintf
#endif

#include <QDebug>
#include <QFile>
#include <QLabel>
#include <QMainWindow>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTextStream>

// XEUS includes
#include "xeus/xkernel.hpp"
#include "xeus/xkernel_configuration.hpp"
#include "xeus-python/xdebugger.hpp"

#include "xSlicerInterpreter.h"
#include "xSlicerServer.h"

// Slicer includes
#include "qSlicerApplication.h"
#include "qSlicerCommandOptions.h"

// Qt includes
#include <QDebug>
#include <QFileInfo>
#include <QProcess>

//-----------------------------------------------------------------------------
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#include <QtPlugin>
Q_EXPORT_PLUGIN2(qSlicerJupyterKernelModule, qSlicerJupyterKernelModule);
#endif

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerJupyterKernelModulePrivate
{
protected:
  qSlicerJupyterKernelModule * const q_ptr;

public:
  qSlicerJupyterKernelModulePrivate(qSlicerJupyterKernelModule& object);

  QProcess InternalJupyterServer;
  bool Started;
  QString ConnectionFile;
  xeus::xkernel * Kernel;
  xeus::xconfiguration Config;
  QLabel* StatusLabel;
};

//-----------------------------------------------------------------------------
// qSlicerJupyterKernelModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerJupyterKernelModulePrivate::qSlicerJupyterKernelModulePrivate(qSlicerJupyterKernelModule& object)
: q_ptr(&object)
, Started(false)
, Kernel(NULL)
, StatusLabel(NULL)
{
}

//-----------------------------------------------------------------------------
// qSlicerJupyterKernelModule methods

//-----------------------------------------------------------------------------
qSlicerJupyterKernelModule::qSlicerJupyterKernelModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerJupyterKernelModulePrivate(*this))
{
}

//-----------------------------------------------------------------------------
qSlicerJupyterKernelModule::~qSlicerJupyterKernelModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerJupyterKernelModule::helpText() const
{
  return "This extension provides a Jupyter kernel, which allows running Jupyter notebooks in 3D Slicer. See <a href=\"https://github.com/Slicer/SlicerJupyter\">extension documentation</a> for more details.";
}

//-----------------------------------------------------------------------------
QString qSlicerJupyterKernelModule::acknowledgementText() const
{
  return "This work was partially funded by CANARIE's Research Software Program";
}

//-----------------------------------------------------------------------------
QStringList qSlicerJupyterKernelModule::contributors() const
{
  QStringList moduleContributors;
  moduleContributors << QString("Jean-Christoph Fillion-Robin (Kitware)") << QString("Andras Lasso (PerkLab)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerJupyterKernelModule::icon() const
{
  return QIcon(":/Icons/JupyterKernel.png");
}

//-----------------------------------------------------------------------------
QStringList qSlicerJupyterKernelModule::categories() const
{
  return QStringList() << "Developer Tools";
}

//-----------------------------------------------------------------------------
QStringList qSlicerJupyterKernelModule::dependencies() const
{
  return QStringList();
}

//-----------------------------------------------------------------------------
void qSlicerJupyterKernelModule::setup()
{
  this->Superclass::setup();
}

//-----------------------------------------------------------------------------
bool qSlicerJupyterKernelModule::updateKernelSpec()
{
  QString kernelFolder = this->resourceFolderPath();
  if (kernelFolder.isEmpty())
  {
    qWarning() << Q_FUNC_INFO << " failed: invalid kernel folder path";
    return false;
  }

  QString kernelJsonTemplatePath = kernelFolder + "/kernel-template.json";
  QFile templateFile(kernelJsonTemplatePath);
  if (!templateFile.exists())
  {
    qWarning() << Q_FUNC_INFO << " kernel template file does not exist: " << kernelJsonTemplatePath;
    return false;
  }
  if (!templateFile.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    qWarning() << Q_FUNC_INFO << " failed to open kernel template file: " << kernelJsonTemplatePath;
    return false;
  }
  QTextStream in(&templateFile);
  QString kernelJson = in.readAll();
  templateFile.close();

  qSlicerApplication* app = qSlicerApplication::application();
  if (kernelJson.indexOf("{slicer_application_name}") != -1)
  {
    kernelJson.replace("{slicer_application_name}", app->applicationName());
  }
  if (kernelJson.indexOf("{slicer_version_full}") != -1)
  {
    kernelJson.replace("{slicer_version_full}", app->applicationVersion());
  }
  if (kernelJson.indexOf("{slicer_version_major}") != -1)
  {
    kernelJson.replace("{slicer_version_major}", QString::number(app->majorVersion()));
  }
  if (kernelJson.indexOf("{slicer_version_minor}") != -1)
  {
    kernelJson.replace("{slicer_version_minor}", QString::number(app->minorVersion()));
  }
  if (kernelJson.indexOf("{slicer_launcher_executable}") != -1)
  {
    QString realExecutable = app->launcherExecutableFilePath();
    if (realExecutable.isEmpty())
      {
      realExecutable = app->applicationFilePath();
      }

    kernelJson.replace("{slicer_launcher_executable}", realExecutable);
  }

  // Compare to existing kernel

  QString kernelJsonPath = kernelFolder + "/kernel.json";
  QFile kernelFile(kernelJsonPath);
  if (!kernelFile.open(QIODevice::ReadWrite | QIODevice::Text))
  {
    qWarning() << Q_FUNC_INFO << " failed: cannot write file " << kernelJsonPath;
    return false;
  }
  QTextStream existingKernelContentStream(&templateFile);
  QString existingKernelJson = existingKernelContentStream.readAll();
  if (existingKernelJson != kernelJson)
  {
    // Kernel modified
    kernelFile.seek(0);
    kernelFile.write(kernelJson.toUtf8());
    kernelFile.resize(kernelFile.pos()); // remove any potential extra content
  }

  kernelFile.close();
  return true;
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation* qSlicerJupyterKernelModule
::createWidgetRepresentation()
{
  return new qSlicerJupyterKernelModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerJupyterKernelModule::createLogic()
{
  return vtkSlicerJupyterKernelLogic::New();
}

//-----------------------------------------------------------------------------
void qSlicerJupyterKernelModule::startKernel(const QString& connectionFile)
{
  Q_D(qSlicerJupyterKernelModule);
  d->ConnectionFile = connectionFile;
  if (!QFileInfo::exists(connectionFile))
  {
    qWarning() << "startKernel" << "connectionFile does not exist" << connectionFile;
    return;
  }
  if (d->Started)
  {
    qWarning() << "Kernel already started";
  }
  else
  {
    d->Config = xeus::load_configuration(connectionFile.toStdString());

    using interpreter_ptr = std::unique_ptr<xSlicerInterpreter>;
    interpreter_ptr interpreter = interpreter_ptr(new xSlicerInterpreter());
    interpreter->set_jupyter_kernel_module(this);

    using history_manager_ptr = std::unique_ptr<xeus::xhistory_manager>;
    history_manager_ptr hist = xeus::make_in_memory_history_manager();

    d->Kernel = new xeus::xkernel(d->Config,
                                  "slicer",
                                  std::move(interpreter),
                                  std::move(hist),
                                  nullptr,
                                  make_xSlicerServer,
                                  xpyt::make_python_debugger);

    d->Kernel->start();

    d->Started = true;

    QString kernelConfigurePy = this->resourceFolderPath() + "/kernel-configure.py";
    QFile kernelConfigurePyFile(kernelConfigurePy);
    if (kernelConfigurePyFile.open(QFile::ReadOnly | QFile::Text))
    {
      QTextStream in(&kernelConfigurePyFile);
      QString kernelConfigurePyContent = in.readAll();
      qSlicerPythonManager* pythonManager = qSlicerApplication::application()->pythonManager();
      pythonManager->executeString(kernelConfigurePyContent);
    }
    else
    {
      qWarning() << Q_FUNC_INFO << " failed: cannot open kernel configure " << kernelConfigurePy;
    }

    QStatusBar* statusBar = NULL;
    if (qSlicerApplication::application()->mainWindow())
    {
      statusBar = qSlicerApplication::application()->mainWindow()->statusBar();
    }
    if (statusBar)
    {
      if (!d->StatusLabel)
      {
        d->StatusLabel = new QLabel;
        statusBar->insertPermanentWidget(0, d->StatusLabel);
      }
      d->StatusLabel->setText(tr("<b><font color=\"red\">Application is managed by Jupyter</font></b>"));
    }
    emit kernelStarted();
  }
}

//-----------------------------------------------------------------------------
void qSlicerJupyterKernelModule::stopKernel()
{
  Q_D(qSlicerJupyterKernelModule);
  // Kernel shutdown requested
  emit kernelStopRequested();
  if (d->StatusLabel)
  {
    d->StatusLabel->setText("");
  }
  qSlicerApplication::application()->exit(0);
}


//-----------------------------------------------------------------------------
bool qSlicerJupyterKernelModule::slicerKernelSpecInstallCommandArgs(QString& executable, QStringList &args)
{
  Q_D(qSlicerJupyterKernelModule);

  vtkSlicerJupyterKernelLogic* kernelLogic = vtkSlicerJupyterKernelLogic::SafeDownCast(this->logic());
  if (!kernelLogic)
  {
    qWarning() << Q_FUNC_INFO << " failed: invalid logic";
    return false;
  }

  // There can be multiple Slicer installations of the same version that all refer to the same extensions folder.
  // Therefore we need to generate the kernel.json file from kernel-template.json file with information
  // specific to the current Slicer instance.
  if (!this->updateKernelSpec())
  {
    qWarning() << Q_FUNC_INFO << " failed: error creating/updating kernel.json file";
    return false;
  }

  executable = "jupyter-kernelspec";
  args = QStringList() << "install" << this->resourceFolderPath() << "--replace" << "--user";

  return true;
}

//-----------------------------------------------------------------------------
bool qSlicerJupyterKernelModule::installInternalJupyterServer()
{
  Q_D(qSlicerJupyterKernelModule);

  PythonQt::init();
  PythonQtObjectPtr context = PythonQt::self()->getMainModule();
  context.evalScript(QString("success=False; import JupyterNotebooks; server=JupyterNotebooks.SlicerJupyterServerHelper(); success=server.installRequiredPackages()"));
  bool success = context.getVariable("success").toBool();
  return success;
  /*

  QString kernelspecExecutable;
  QStringList args;
  if (!this->slicerKernelSpecInstallCommandArgs(kernelspecExecutable, args))
  {
    qWarning() << Q_FUNC_INFO << " failed: slicerKernelSpecInstallCommandArgs failed to determine install command";
    return false;
  }

  QString kernelspecExecutablePath = pythonScriptsFolder + "/" + kernelspecExecutable;

  qDebug() << Q_FUNC_INFO << ": launching " << kernelspecExecutablePath << " " << args.join(" ");

  QProcess kernelSpecProcess;
  qSlicerApplication* app = qSlicerApplication::application();
  kernelSpecProcess.setProcessEnvironment(app->startupEnvironment());
  kernelSpecProcess.setProgram(kernelspecExecutablePath);
  kernelSpecProcess.setArguments(args);
  kernelSpecProcess.start();
  bool finished = kernelSpecProcess.waitForFinished();
  QString output = QString(kernelSpecProcess.readAllStandardOutput());
  QString errorOutput = QString(kernelSpecProcess.readAllStandardError());
  if (!output.isEmpty())
  {
    qDebug() << "Kernelspec install output: " << output;
  }
  if (!errorOutput.isEmpty())
  {
    qWarning() << "Kernelspec install error output: " << errorOutput;
  }
  if (!finished)
  {
    qWarning() << Q_FUNC_INFO << " failed: error launching process " << kernelspecExecutablePath
      << " (code = " << kernelSpecProcess.error() << ")";
    return false;
  }
  if (kernelSpecProcess.exitCode() != 0)
  {
    qWarning() << Q_FUNC_INFO << " failed: process " << kernelspecExecutablePath
      << " returned with exit code " << kernelSpecProcess.exitCode();
    return false;
  }
  return true;
  */
}

//-----------------------------------------------------------------------------
bool qSlicerJupyterKernelModule::startInternalJupyterServer(QString notebookDirectory, bool detached/*=false*/, bool classic/*=false*/)
{
  Q_D(qSlicerJupyterKernelModule);
  QString pythonExecutable = QStandardPaths::findExecutable("PythonSlicer");
  d->InternalJupyterServer.setProgram(pythonExecutable);
  QStringList args;
  if (classic)
  {
    args << "-m" << "notebook";
  }
  else
  {
    args << "-m" << "jupyter" << "lab";
  }
  args << "--notebook-dir" << notebookDirectory;
  d->InternalJupyterServer.setArguments(args);
  bool success = false;
  if (detached)
  {
    success = d->InternalJupyterServer.startDetached();
  }
  else
  {
    d->InternalJupyterServer.start();
    success = d->InternalJupyterServer.waitForStarted();
  }
  return success;
}

//-----------------------------------------------------------------------------
bool qSlicerJupyterKernelModule::isInternalJupyterServerRunning() const
{
  Q_D(const qSlicerJupyterKernelModule);
  return d->InternalJupyterServer.state() == QProcess::Running;
}

//-----------------------------------------------------------------------------
bool qSlicerJupyterKernelModule::stopInternalJupyterServer()
{
  Q_D(qSlicerJupyterKernelModule);
  // TOOD: currently, none of these methods work for stopping a server that is
  // started using AppLauncher.
  // terminate() is not strong enough.
  // kill() immediately kills the launcher but not the launched application.
  d->InternalJupyterServer.terminate();
  //d->InternalJupyterServer.kill();
  return d->InternalJupyterServer.waitForFinished(5000);
}

//---------------------------------------------------------------------------
QString qSlicerJupyterKernelModule::kernelSpecPath()
{
  vtkSlicerJupyterKernelLogic* kernelLogic = vtkSlicerJupyterKernelLogic::SafeDownCast(this->logic());
  if (!kernelLogic)
  {
    qWarning() << Q_FUNC_INFO << " failed: invalid logic";
    return "";
  }
  qSlicerApplication* app = qSlicerApplication::application();
  QString path = QString("%1/%2-%3.%4").arg(kernelLogic->GetModuleShareDirectory().c_str())
    .arg(app->applicationName()).arg(app->majorVersion()).arg(app->minorVersion());
  return path;
}

QString qSlicerJupyterKernelModule::resourceFolderPath()
{
  return this->kernelSpecPath();
}

//---------------------------------------------------------------------------
double qSlicerJupyterKernelModule::pollIntervalSec()
{
  Q_D(qSlicerJupyterKernelModule);
  if (d->Kernel == nullptr)
  {
    qCritical() << Q_FUNC_INFO << " failed: kernel has not started yet";
    return 0.0;
  }
  return reinterpret_cast<xSlicerServer*>(&d->Kernel->get_server())->pollIntervalSec();

  return 0.0;
}

//---------------------------------------------------------------------------
void qSlicerJupyterKernelModule::setPollIntervalSec(double intervalSec)
{
  Q_D(qSlicerJupyterKernelModule);
  if (d->Kernel == nullptr)
  {
    qCritical() << Q_FUNC_INFO << " failed: kernel has not started yet";
    return;
  }

  reinterpret_cast<xSlicerServer*>(&d->Kernel->get_server())->setPollIntervalSec(intervalSec);
}

//---------------------------------------------------------------------------
QString qSlicerJupyterKernelModule::connectionFile()
{
  Q_D(qSlicerJupyterKernelModule);
  return d->ConnectionFile;
}
