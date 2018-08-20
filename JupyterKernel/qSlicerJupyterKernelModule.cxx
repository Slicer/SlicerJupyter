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
#include <QTextStream>

// XEUS includes
#include "xeus/xkernel.hpp"
#include "xeus/xkernel_configuration.hpp"

#include "xSlicerInterpreter.h"
#include "xSlicerServer.h"

// Slicer includes
#include "qSlicerApplication.h"
#include "qSlicerCommandOptions.h"

// Qt includes
#include <QDebug>
#include <QFileInfo>
#include <QProcess>

// TODO move this code somewhere
auto complete_code = R"(

# Create a placeholder for display() helper function to make it available for auto-completion
# and specify documentation. This display method will not executed but the Slicer Jupyter kernel
# will intercept the call and performs the necessary action.
def display():
  """Display view layout in a Jupyter notebook"""
  pass

# TODO this should be done in Slicer core init
import sys
import distutils.spawn
sys.executable = distutils.spawn.find_executable('python-real') or distutils.spawn.find_executable('python')

# TODO put everything in try block and return errors on side channel

def complete(code, cursor_pos):

    import json
    import jedi
    import jedi.api.environment

    # hack to work around: https://github.com/davidhalter/jedi/issues/1142
    jedi.api.environment.get_default_environment = lambda: jedi.api.environment.SameEnvironment()

    lines = code[:cursor_pos].splitlines() or [code]
    line, column = len(lines), len(lines[-1].strip())

    script = jedi.Interpreter(code, line=line, column=column, namespaces=[globals()])
    completions = [x.complete for x in script.completions()]

    d = json.dumps(
            {
            'matches': completions,
            'cursor_start': cursor_pos,
            'cursor_end': cursor_pos,
            'metadata': {},
            'status': 'ok'
            })
    return d

slicer.util.py_complete_request = complete
)";

//-----------------------------------------------------------------------------
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#include <QtPlugin>
Q_EXPORT_PLUGIN2(qSlicerJupyterKernelModule, qSlicerJupyterKernelModule);
#endif

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerJupyterKernelModulePrivate
{
public:
  qSlicerJupyterKernelModulePrivate(qSlicerJupyterKernelModule& object);

  bool Started;
  xeus::xkernel * Kernel;
  xeus::xconfiguration Config;

protected:
  qSlicerJupyterKernelModule* const q_ptr;

};

//-----------------------------------------------------------------------------
// qSlicerJupyterKernelModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerJupyterKernelModulePrivate::qSlicerJupyterKernelModulePrivate(qSlicerJupyterKernelModule& object)
: q_ptr(&object)
, Started(false)
, Kernel(NULL)
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
  return "This extension provides a Jupyter kernel, which allows running Jupyter notebooks in 3D Slicer.";
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
  this->updateKernelSpec();
}

//-----------------------------------------------------------------------------
void qSlicerJupyterKernelModule::updateKernelSpec()
{
  QString kernelFolder = this->kernelFolderPath();
  if (kernelFolder.isEmpty())
  {
    qWarning() << Q_FUNC_INFO << " failed: invalid kernel folder path";
    return;
  }

  QString kernelJsonPath = kernelFolder + "/kernel.json";
  QFile file(kernelJsonPath);
  if (!file.open(QIODevice::ReadWrite | QIODevice::Text))
  {
    qWarning() << Q_FUNC_INFO << " failed: cannot read file " << kernelJsonPath;
    return;
  }
  QTextStream in(&file);
  QString kernelJson = in.readAll();

  bool wasModified = false;

  qSlicerApplication* app = qSlicerApplication::application();
  if (kernelJson.indexOf("{slicer_application_name}") != -1)
  {
    kernelJson.replace("{slicer_application_name}", app->applicationName());
    wasModified = true;
  }
  if (kernelJson.indexOf("{slicer_version_full}") != -1)
  {
    kernelJson.replace("{slicer_version_full}", app->applicationVersion());
    wasModified = true;
  }
  if (kernelJson.indexOf("{slicer_version_major}") != -1)
  {
    kernelJson.replace("{slicer_version_major}", QString::number(app->majorVersion()));
    wasModified = true;
  }
  if (kernelJson.indexOf("{slicer_version_minor}") != -1)
  {
    kernelJson.replace("{slicer_version_minor}", QString::number(app->minorVersion()));
    wasModified = true;
  }

  if (kernelJson.indexOf("{slicer_launcher_executable}") != -1)
  {
    QString realExecutable = app->launcherExecutableFilePath();
    if (realExecutable.isEmpty())
      {
      realExecutable = app->applicationFilePath();
      }

    kernelJson.replace("{slicer_launcher_executable}", realExecutable);
    wasModified = true;
  }

  if (!wasModified)
  {
    // already up-to-date
    return;
  }

  file.seek(0);
  file.write(kernelJson.toUtf8());
  file.resize(file.pos()); // remove any potential extra content

  file.close();
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
    d->Kernel = new xeus::xkernel(d->Config, "slicer", std::move(interpreter), make_xSlicerServer);

    d->Kernel->start();

    d->Started = true;

    // TODO init where?
    // Initialize the slicer.util.py_complete
    qSlicerPythonManager* pythonManager = qSlicerApplication::application()->pythonManager();
    pythonManager->executeString(QString::fromStdString(complete_code));
  }
}

//-----------------------------------------------------------------------------
bool qSlicerJupyterKernelModule::installSlicerKernel(QString pythonScriptsFolder)
{
  Q_D(qSlicerJupyterKernelModule);
  qSlicerApplication* app = qSlicerApplication::application();

  vtkSlicerJupyterKernelLogic* kernelLogic = vtkSlicerJupyterKernelLogic::SafeDownCast(this->logic());
  if (!kernelLogic)
  {
    qWarning() << Q_FUNC_INFO << " failed: invalid logic";
    return false;
  }

  QString kernelspecExecutable = pythonScriptsFolder + "/" + "jupyter-kernelspec";
  QStringList args;
  args << "install" << this->kernelFolderPath() << "--replace" << "--user";

  QProcess kernelSpecProcess;
  kernelSpecProcess.setProcessEnvironment(app->startupEnvironment());
  kernelSpecProcess.setProgram(kernelspecExecutable);
  kernelSpecProcess.setArguments(args);
  kernelSpecProcess.start();
  bool finished = kernelSpecProcess.waitForFinished();
  if (!finished)
  {
    qWarning() << Q_FUNC_INFO << " failed: time out while launching process " << kernelspecExecutable;
    return false;
  }

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

  bool success = (kernelSpecProcess.exitCode() == 0);
  return success;
}

//-----------------------------------------------------------------------------
bool qSlicerJupyterKernelModule::startJupyterNotebook(QString pythonScriptsFolder)
{
  Q_D(qSlicerJupyterKernelModule);
  qSlicerApplication* app = qSlicerApplication::application();

  vtkSlicerJupyterKernelLogic* kernelLogic = vtkSlicerJupyterKernelLogic::SafeDownCast(this->logic());
  if (!kernelLogic)
  {
    qWarning() << Q_FUNC_INFO << " failed: invalid logic";
    return false;
  }

  QString kernelspecExecutable = pythonScriptsFolder + "/" + "jupyter-notebook";

  QProcess kernelSpecProcess;
  kernelSpecProcess.setProcessEnvironment(app->startupEnvironment());
  kernelSpecProcess.setProgram(kernelspecExecutable);

  // TODO: decide if we want to allow users to start notebook from Slicer.
  // Detached start would require Qt-5.10 and users might want to start the notebook manually, in a virtual environment.
  // kernelSpecProcess.startDetached();
  //return true;
  return false;
}

//---------------------------------------------------------------------------
QString qSlicerJupyterKernelModule::kernelFolderPath()
{
  vtkSlicerJupyterKernelLogic* kernelLogic = vtkSlicerJupyterKernelLogic::SafeDownCast(this->logic());
  if (!kernelLogic)
  {
    qWarning() << Q_FUNC_INFO << " failed: invalid logic";
    return "";
  }
  qSlicerApplication* app = qSlicerApplication::application();
  QString kernelFolderPath = QString("%1/%2-%3.%4").arg(kernelLogic->GetModuleShareDirectory().c_str())
    .arg(app->applicationName()).arg(app->majorVersion()).arg(app->minorVersion());
  return kernelFolderPath;
}
