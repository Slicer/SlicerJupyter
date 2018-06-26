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
  qSlicerJupyterKernelModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerJupyterKernelModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerJupyterKernelModulePrivate::qSlicerJupyterKernelModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerJupyterKernelModule methods

//-----------------------------------------------------------------------------
qSlicerJupyterKernelModule::qSlicerJupyterKernelModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerJupyterKernelModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerJupyterKernelModule::~qSlicerJupyterKernelModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerJupyterKernelModule::helpText() const
{
  return "This is a loadable module that can be bundled in an extension";
}

//-----------------------------------------------------------------------------
QString qSlicerJupyterKernelModule::acknowledgementText() const
{
  return "This work was partially funded by NIH grant NXNNXXNNNNNN-NNXN";
}

//-----------------------------------------------------------------------------
QStringList qSlicerJupyterKernelModule::contributors() const
{
  QStringList moduleContributors;
  moduleContributors << QString("John Doe (AnyWare Corp.)");
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
  return QStringList() << "Examples";
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
