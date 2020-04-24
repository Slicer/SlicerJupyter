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
#include <QLabel>
#include <QMainWindow>
#include <QStatusBar>
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

# Copied over from slicer.util.setViewControllersVisible to make this function available for Slicer-4.10
def setViewControllersVisible(visible):
  import slicer
  lm = slicer.app.layoutManager()
  for viewIndex in range(lm.threeDViewCount):
    lm.threeDWidget(viewIndex).threeDController().setVisible(visible)
  for sliceViewName in lm.sliceViewNames():
    lm.sliceWidget(sliceViewName).sliceController().setVisible(visible)
  try:
    for viewIndex in range(lm.tableViewCount):
      lm.tableWidget(viewIndex).tableController().setVisible(visible)
    for viewIndex in range(lm.plotViewCount):
      lm.plotWidget(viewIndex).plotController().setVisible(visible)
  except:
    # this function is not available in this Slicer version
    pass

# Copied over from slicer.util.setViewControllersVisible to make this function available for Slicer-4.10
def forceRenderAllViews():
  import slicer
  lm = slicer.app.layoutManager()
  for viewIndex in range(lm.threeDViewCount):
    lm.threeDWidget(viewIndex).threeDView().forceRender()
  for sliceViewName in lm.sliceViewNames():
    lm.sliceWidget(sliceViewName).sliceView().forceRender()
  for viewIndex in range(lm.tableViewCount):
    lm.tableWidget(viewIndex).tableView().repaint()
  for viewIndex in range(lm.plotViewCount):
    lm.plotWidget(viewIndex).plotView().repaint()

def display(value=None, type=None, binary=False, filename=None):
  """Set data to be displayed in Jupyter notebook as command response.
  If no value is specified then screnshot of the view layout will be used.
  If binary is set to True then value translated to text using base64-encoding.
  """
  if value is None and filename is None:
    # Capture image of view layout and use that as displayed data
    layoutManager = slicer.app.layoutManager()
    try:
      slicer.util.setViewControllersVisible(False)
    except:
      # this function is not available in this Slicer version
      setViewControllersVisible(False)
    slicer.app.processEvents()
    try:
      slicer.util.forceRenderAllViews()
    except:
      # this function is not available in this Slicer version
      forceRenderAllViews()
    screenshot = layoutManager.viewport().grab()
    try:
      slicer.util.setViewControllersVisible(True)
    except:
      # this function is not available in this Slicer version
      setViewControllersVisible(True)
    bArray = qt.QByteArray()
    buffer = qt.QBuffer(bArray)
    buffer.open(qt.QIODevice.WriteOnly)
    screenshot.save(buffer, "PNG")
    slicer.modules.jupyterkernel.executeResultDataValue = bArray.toBase64().data().decode()
    slicer.modules.jupyterkernel.executeResultDataType = "image/png"
  else:
    if value is None:
      if binary:
        value = open(filename, 'rb').read()
      else:
        value = open(filename, 'r').read()
    if binary:
      # encode binary data as base64
      import base64
      import sys
      if sys.version_info.major==3:
        slicer.modules.jupyterkernel.executeResultDataValue = base64.b64encode(value).decode('ascii')
      else:
        slicer.modules.jupyterkernel.executeResultDataValue = base64.b64encode(value).encode('ascii')
    else:      
      slicer.modules.jupyterkernel.executeResultDataValue = value
    if type is None:
      slicer.modules.jupyterkernel.executeResultDataType = "text/plain"
    else:
      slicer.modules.jupyterkernel.executeResultDataType = type

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
    line, column = len(lines), len(lines[-1])

    script = jedi.Interpreter(code, line=line, column=column, namespaces=[globals()])
    completions = script.completions()

    cursor_start = cursor_pos - (len(completions[0].name_with_symbols) - len(completions[0].complete))

    d = json.dumps(
            {
            'matches': [x.name_with_symbols for x in completions],
            'cursor_start': cursor_start,
            'cursor_end': cursor_pos,
            'metadata': {},
            'status': 'ok'
            })
    return d


# Code from IPython to make inspection work inside parentheses after method name

from collections import namedtuple
from io import StringIO
from keyword import iskeyword
import tokenize
Token = namedtuple('Token', ['token', 'text', 'start', 'end', 'line'])

def generate_tokens(readline):
    try:
        for token in tokenize.generate_tokens(readline):
            yield token
    except tokenize.TokenError:
        # catch EOF error
        return

def token_at_cursor(cell, cursor_pos=0):
    names = []
    tokens = []
    call_names = []

    offsets = {1: 0} # lines start at 1
    for tup in generate_tokens(StringIO(cell).readline):

        tok = Token(*tup)

        # token, text, start, end, line = tup
        start_line, start_col = tok.start
        end_line, end_col = tok.end
        if end_line + 1 not in offsets:
            # keep track of offsets for each line
            lines = tok.line.splitlines(True)
            for lineno, line in enumerate(lines, start_line + 1):
                if lineno not in offsets:
                    offsets[lineno] = offsets[lineno-1] + len(line)

        offset = offsets[start_line]
        # allow '|foo' to find 'foo' at the beginning of a line
        boundary = cursor_pos + 1 if start_col == 0 else cursor_pos
        if offset + start_col >= boundary:
            # current token starts after the cursor,
            # don't consume it
            break

        if tok.token == tokenize.NAME and not iskeyword(tok.text):
            if names and tokens and tokens[-1].token == tokenize.OP and tokens[-1].text == '.':
                names[-1] = '%s.%s' % (names[-1], tok.text)
            else:
                names.append(tok.text)
        elif tok.token == tokenize.OP:
            if tok.text == '=' and names:
                # don't inspect the lhs of an assignment
                names.pop(-1)
            if tok.text == '(' and names:
                # if we are inside a function call, inspect the function
                call_names.append(names[-1])
            elif tok.text == ')' and call_names:
                call_names.pop(-1)

        tokens.append(tok)

        if offsets[end_line] + end_col > cursor_pos:
            # we found the cursor, stop reading
            break

    if call_names:
        return call_names[-1]
    elif names:
        return names[-1]
    else:
        return ''


def inspect(code, cursor_pos, detail_level):

    import json
    import jedi
    import jedi.api.environment

    # hack to work around: https://github.com/davidhalter/jedi/issues/1142
    jedi.api.environment.get_default_environment = lambda: jedi.api.environment.SameEnvironment()

    lines = code[:cursor_pos].splitlines() or [code]
    line, column = len(lines), len(lines[-1])

    script = jedi.Interpreter(code, line=line, column=column, namespaces=[globals()])
    definitions = script.goto_definitions()
    found = False
    doc = ''
    if definitions:
        doc = definitions[0].docstring()
        found = True

    if not found:
        # definitions are not always found for wrapped C++ objects, try to get help doc
        try:
          import pydoc
          doc = pydoc.plain(pydoc.render_doc(eval(code), "Help on %s"))
          found = True
        except:
          pass

    d = json.dumps(
            {
            'found': found,
            'data': {'text/plain': doc},
            'metadata': {},
            'status': 'ok'
            })
    return d

slicer.util.py_complete_request = complete
slicer.util.py_token_at_cursor = token_at_cursor
slicer.util.py_inspect_request = inspect
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
protected:
  qSlicerJupyterKernelModule * const q_ptr;

public:
  qSlicerJupyterKernelModulePrivate(qSlicerJupyterKernelModule& object);

  bool Started;
  xeus::xkernel * Kernel;
  xeus::xconfiguration Config;
  QLabel* StatusLabel;

  QString ExecuteResultDataType;
  QString ExecuteResultDataValue;
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
  QString kernelFolder = this->kernelFolderPath();
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
    d->Kernel = new xeus::xkernel(d->Config, "slicer", std::move(interpreter), make_xSlicerServer);

    d->Kernel->start();

    d->Started = true;

    // TODO init where?
    // Initialize the slicer.util.py_complete
    qSlicerPythonManager* pythonManager = qSlicerApplication::application()->pythonManager();
    pythonManager->executeString(QString::fromStdString(complete_code));

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
  }
}

//-----------------------------------------------------------------------------
void qSlicerJupyterKernelModule::stopKernel()
{
  Q_D(qSlicerJupyterKernelModule);
  // Kernel shutdown requested
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

  qSlicerApplication* app = qSlicerApplication::application();

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
  args = QStringList() << "install" << this->kernelFolderPath() << "--replace" << "--user";

  return true;
}

//-----------------------------------------------------------------------------
bool qSlicerJupyterKernelModule::installSlicerKernel(QString pythonScriptsFolder)
{
  Q_D(qSlicerJupyterKernelModule);

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

  QString kernelExecutable = pythonScriptsFolder + "/" + "jupyter-notebook";

  QProcess kernelSpecProcess;
  kernelSpecProcess.setProcessEnvironment(app->startupEnvironment());
  kernelSpecProcess.setProgram(kernelExecutable);

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

//---------------------------------------------------------------------------
QString qSlicerJupyterKernelModule::executeResultDataType()
{
  Q_D(qSlicerJupyterKernelModule);
  return d->ExecuteResultDataType;
}

//---------------------------------------------------------------------------
void qSlicerJupyterKernelModule::setExecuteResultDataType(const QString& str)
{
  Q_D(qSlicerJupyterKernelModule);
  d->ExecuteResultDataType = str;
}

//---------------------------------------------------------------------------
QString qSlicerJupyterKernelModule::executeResultDataValue()
{
  Q_D(qSlicerJupyterKernelModule);
  return d->ExecuteResultDataValue;
}

//---------------------------------------------------------------------------
void qSlicerJupyterKernelModule::setExecuteResultDataValue(const QString& str)
{
  Q_D(qSlicerJupyterKernelModule);
  d->ExecuteResultDataValue = str;
}
