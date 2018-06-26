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

// FooBar Widgets includes
#include "qSlicerJupyterKernelFooBarWidget.h"
#include "ui_qSlicerJupyterKernelFooBarWidget.h"

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_JupyterKernel
class qSlicerJupyterKernelFooBarWidgetPrivate
  : public Ui_qSlicerJupyterKernelFooBarWidget
{
  Q_DECLARE_PUBLIC(qSlicerJupyterKernelFooBarWidget);
protected:
  qSlicerJupyterKernelFooBarWidget* const q_ptr;

public:
  qSlicerJupyterKernelFooBarWidgetPrivate(
    qSlicerJupyterKernelFooBarWidget& object);
  virtual void setupUi(qSlicerJupyterKernelFooBarWidget*);
};

// --------------------------------------------------------------------------
qSlicerJupyterKernelFooBarWidgetPrivate
::qSlicerJupyterKernelFooBarWidgetPrivate(
  qSlicerJupyterKernelFooBarWidget& object)
  : q_ptr(&object)
{
}

// --------------------------------------------------------------------------
void qSlicerJupyterKernelFooBarWidgetPrivate
::setupUi(qSlicerJupyterKernelFooBarWidget* widget)
{
  this->Ui_qSlicerJupyterKernelFooBarWidget::setupUi(widget);
}

//-----------------------------------------------------------------------------
// qSlicerJupyterKernelFooBarWidget methods

//-----------------------------------------------------------------------------
qSlicerJupyterKernelFooBarWidget
::qSlicerJupyterKernelFooBarWidget(QWidget* parentWidget)
  : Superclass( parentWidget )
  , d_ptr( new qSlicerJupyterKernelFooBarWidgetPrivate(*this) )
{
  Q_D(qSlicerJupyterKernelFooBarWidget);
  d->setupUi(this);
}

//-----------------------------------------------------------------------------
qSlicerJupyterKernelFooBarWidget
::~qSlicerJupyterKernelFooBarWidget()
{
}
