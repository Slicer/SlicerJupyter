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

// Qt includes
#include <QDebug>
#include <QClipboard>

// SlicerQt includes
#include "qSlicerJupyterKernelModuleWidget.h"
#include "ui_qSlicerJupyterKernelModuleWidget.h"

#include "vtkSlicerJupyterKernelLogic.h"
#include "qSlicerJupyterKernelModule.h"

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerJupyterKernelModuleWidgetPrivate: public Ui_qSlicerJupyterKernelModuleWidget
{
public:
  qSlicerJupyterKernelModuleWidgetPrivate();
};

//-----------------------------------------------------------------------------
// qSlicerJupyterKernelModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerJupyterKernelModuleWidgetPrivate::qSlicerJupyterKernelModuleWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerJupyterKernelModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerJupyterKernelModuleWidget::qSlicerJupyterKernelModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerJupyterKernelModuleWidgetPrivate )
{
}

//-----------------------------------------------------------------------------
qSlicerJupyterKernelModuleWidget::~qSlicerJupyterKernelModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerJupyterKernelModuleWidget::setup()
{
  Q_D(qSlicerJupyterKernelModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  d->PythonScriptsFolderEdit->setFilters(ctkPathLineEdit::Dirs);
  d->PythonScriptsFolderEdit->setSettingKey("JupyterKernelPythonScriptsDir");

  qSlicerJupyterKernelModule* kernelModule = dynamic_cast<qSlicerJupyterKernelModule*>(this->module());
  if (kernelModule)
  {
    QString executable;
    QStringList args;
    if (kernelModule->slicerKernelSpecInstallCommandArgs(executable, args))
    {
      QString manualInstallCommand = executable + " " + args.join(" ");
      d->ManualInstallCommandTextEdit->setText(manualInstallCommand);
    }
  }

  connect(d->InstallSlicerKernelPushButton, SIGNAL(clicked()), this, SLOT(onInstallSlicerKernel()));
  connect(d->CopyCommandToClipboardPushButton, SIGNAL(clicked()), this, SLOT(onCopyInstallCommandToClipboard()));
  connect(d->StartJupyterNotebookPushButton, SIGNAL(clicked()), this, SLOT(onStartJupyterNotebook()));

  // Hide control section for now.
  // Currently, it has a button for starting jupyter notebook, but it requires Qt-5.10.
  // Also, users may want to run the notebook in a virtual environment.
  d->ControlCollapsibleButton->setVisible(false);
}

//-----------------------------------------------------------------------------
void qSlicerJupyterKernelModuleWidget::onCopyInstallCommandToClipboard()
{
  Q_D(qSlicerJupyterKernelModuleWidget);
  QApplication::clipboard()->setText(d->ManualInstallCommandTextEdit->toPlainText());
}

//-----------------------------------------------------------------------------
void qSlicerJupyterKernelModuleWidget::onInstallSlicerKernel()
{
  Q_D(qSlicerJupyterKernelModuleWidget);
  d->PythonScriptsFolderEdit->addCurrentPathToHistory();
  qSlicerJupyterKernelModule* kernelModule = dynamic_cast<qSlicerJupyterKernelModule*>(this->module());
  if (!kernelModule)
  {
    qWarning() << Q_FUNC_INFO << " failed: invalid module";
    return;
  }

  d->InstallSlicerKernelStatusLabel->setText(tr("Kernel installation in progress..."));
  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
  bool success = kernelModule->installSlicerKernel(d->PythonScriptsFolderEdit->currentPath());
  QApplication::restoreOverrideCursor();
  if (success)
  {
    d->InstallSlicerKernelStatusLabel->setText(tr("Kernel installation completed successfully."));
  }
  else
  {
    d->InstallSlicerKernelStatusLabel->setText(tr("Automatic kernel installation failed. See application log for details."));
  }
}

//-----------------------------------------------------------------------------
void qSlicerJupyterKernelModuleWidget::onStartJupyterNotebook()
{
  Q_D(qSlicerJupyterKernelModuleWidget);
  d->PythonScriptsFolderEdit->addCurrentPathToHistory();
  qSlicerJupyterKernelModule* kernelModule = dynamic_cast<qSlicerJupyterKernelModule*>(this->module());
  if (!kernelModule)
  {
    qWarning() << Q_FUNC_INFO << " failed: invalid module";
    return;
  }
  kernelModule->startJupyterNotebook(d->PythonScriptsFolderEdit->currentPath());
}
