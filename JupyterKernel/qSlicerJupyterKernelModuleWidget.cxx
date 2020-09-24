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

  d->NotebookPathLineEdit->setFilters(ctkPathLineEdit::Dirs);
  d->NotebookPathLineEdit->setSettingKey("JupyterKernelNotebookDir");

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

  // Stopping of the server does not work and it is not really needed either.
  // We hide it for now, later the features will be either fixed or completely removed.
  d->StopJupyterNotebookPushButton->hide();

  connect(d->StartJupyterNotebookPushButton, SIGNAL(clicked()), this, SLOT(startJupyterServer()));
  connect(d->StopJupyterNotebookPushButton, SIGNAL(clicked()), this, SLOT(stopJupyterServer()));

  connect(d->CopyCommandToClipboardPushButton, SIGNAL(clicked()), this, SLOT(copyInstallCommandToClipboard()));
}

//-----------------------------------------------------------------------------
void qSlicerJupyterKernelModuleWidget::copyInstallCommandToClipboard()
{
  Q_D(qSlicerJupyterKernelModuleWidget);
  QApplication::clipboard()->setText(d->ManualInstallCommandTextEdit->toPlainText());
}

//-----------------------------------------------------------------------------
bool qSlicerJupyterKernelModuleWidget::installJupyterServer()
{
  Q_D(qSlicerJupyterKernelModuleWidget);
  qSlicerJupyterKernelModule* kernelModule = dynamic_cast<qSlicerJupyterKernelModule*>(this->module());
  if (!kernelModule)
  {
    qWarning() << Q_FUNC_INFO << " failed: invalid module";
    return false;
  }

  d->JupyterServerStatusLabel->setText(tr("Jupyter installation is in progress... it may take 10-15 minutes."));
  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
  bool success = kernelModule->installInternalJupyterServer();
  QApplication::restoreOverrideCursor();
  if (success)
  {
    d->JupyterServerStatusLabel->setText(tr("Jupyter installation completed successfully."));
  }
  else
  {
    d->JupyterServerStatusLabel->setText(tr("Jupyter installation failed. See application log for details."));
  }
  return success;
}

//-----------------------------------------------------------------------------
void qSlicerJupyterKernelModuleWidget::startJupyterServer()
{
  Q_D(qSlicerJupyterKernelModuleWidget);
  d->NotebookPathLineEdit->addCurrentPathToHistory();
  qSlicerJupyterKernelModule* kernelModule = dynamic_cast<qSlicerJupyterKernelModule*>(this->module());
  if (!kernelModule)
  {
    d->JupyterServerStatusLabel->setText(tr("Jupyter server start failed."));
    qWarning() << Q_FUNC_INFO << " failed: invalid module";
    return;
  }
  if (kernelModule->isInternalJupyterServerRunning())
  {
    d->JupyterServerStatusLabel->setText(tr("Jupyter server is already running."));
    return;
  }
  bool installationSuccess = this->installJupyterServer();
  if (installationSuccess)
  {
    d->JupyterServerStatusLabel->setText(tr("Starting Jupyter server..."));
  }
  else
  {
    d->JupyterServerStatusLabel->setText(tr("Error occurred during installation, attempting to start Jupyter server anyway..."));
  }
  // Start in detached mode so that Slicer can be shut down without impacting the server.
  bool detached = true;
  if (kernelModule->startInternalJupyterServer(d->NotebookPathLineEdit->currentPath(), detached))
  {
    if (detached)
    {
      d->JupyterServerStatusLabel->setText(tr("Jupyter server started successfully.\nThis application can be stopped now. Jupyter server will keep running."));
    }
    else
    {
      d->JupyterServerStatusLabel->setText(tr("Jupyter server started successfully.")); 
    }
  }
  else
  {
    d->JupyterServerStatusLabel->setText(tr("Jupyter server start failed. See application log for details."));
  }
}

//-----------------------------------------------------------------------------
void qSlicerJupyterKernelModuleWidget::stopJupyterServer()
{
  Q_D(qSlicerJupyterKernelModuleWidget);
  qSlicerJupyterKernelModule* kernelModule = dynamic_cast<qSlicerJupyterKernelModule*>(this->module());
  if (!kernelModule)
  {
    qWarning() << Q_FUNC_INFO << " failed: invalid module";
    d->JupyterServerStatusLabel->setText(tr("Jupyter server stop failed."));
    return;
  }
  if (!kernelModule->isInternalJupyterServerRunning())
  {
    d->JupyterServerStatusLabel->setText(tr("Jupyter server is already stopped."));
    return;
  }
  kernelModule->stopInternalJupyterServer();
  if (kernelModule->isInternalJupyterServerRunning())
  {
    d->JupyterServerStatusLabel->setText(tr("Jupyter server stop failed. See application log for details."));
  }
}
