/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Jean-Christophe Fillion-Robin, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// JupyterKernelControl Widgets includes
#include "qSlicerJupyterKernelControlWidget.h"
#include "ui_qSlicerJupyterKernelControlWidget.h"

// Qt includes
#include <QDebug>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_JupyterKernel
class qSlicerJupyterKernelControlWidgetPrivate
  : public Ui_qSlicerJupyterKernelControlWidget
{
  Q_DECLARE_PUBLIC(qSlicerJupyterKernelControlWidget);
protected:
  qSlicerJupyterKernelControlWidget* const q_ptr;

public:
  qSlicerJupyterKernelControlWidgetPrivate(
    qSlicerJupyterKernelControlWidget& object);
  virtual void setupUi(qSlicerJupyterKernelControlWidget*);
};

// --------------------------------------------------------------------------
qSlicerJupyterKernelControlWidgetPrivate
::qSlicerJupyterKernelControlWidgetPrivate(
  qSlicerJupyterKernelControlWidget& object)
  : q_ptr(&object)
{

}

// --------------------------------------------------------------------------
void qSlicerJupyterKernelControlWidgetPrivate
::setupUi(qSlicerJupyterKernelControlWidget* widget)
{
  this->Ui_qSlicerJupyterKernelControlWidget::setupUi(widget);
}

//-----------------------------------------------------------------------------
// qSlicerJupyterKernelControlWidget methods

//-----------------------------------------------------------------------------
qSlicerJupyterKernelControlWidget
::qSlicerJupyterKernelControlWidget(QWidget* parentWidget)
  : Superclass( parentWidget )
  , d_ptr( new qSlicerJupyterKernelControlWidgetPrivate(*this) )
{
  Q_D(qSlicerJupyterKernelControlWidget);
  d->setupUi(this);
}

//-----------------------------------------------------------------------------
qSlicerJupyterKernelControlWidget
::~qSlicerJupyterKernelControlWidget()
{
}
