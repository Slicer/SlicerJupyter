#include "xSlicerServer.h"

// Slicer includes
#include <qSlicerApplication.h>
#include <qSlicerModuleManager.h>
#include "qSlicerJupyterKernelModule.h"

// STL includes
#include <memory>
#include <thread>

// zmq includes
#include <zmq_addon.hpp>

// Qt includes
#include <QDebug>
#include <QTimer>

xSlicerServer::xSlicerServer(xeus::xcontext& context,
                           const xeus::xconfiguration& c,
                           nl::json::error_handler_t eh)
    : xserver_zmq(context, c, eh)
{
  // 10ms interval is short enough so that users will not notice significant latency
  // yet it is long enough to minimize CPU load caused by polling.
  // 50ms causes too long delay in interactive widgets that handle mousemove events.
  m_pollTimer = new QTimer();
  m_pollTimer->setInterval(10);
  QObject::connect(m_pollTimer, &QTimer::timeout, [=]() { this->poll(); });
}

xSlicerServer::~xSlicerServer()
{
  m_pollTimer->stop();
  delete m_pollTimer;
}

void xSlicerServer::start_impl(xeus::xpub_message msg)
{
    qDebug() << "Starting Jupyter kernel server";

    start_publisher_thread();
    start_heartbeat_thread();
    set_request_stop(false);

    publish(std::move(msg), xeus::channel::SHELL);

    m_pollTimer->start();
}

void xSlicerServer::stop_impl()
{
  qDebug() << "Stopping Jupyter kernel server";
  //this->xserver_zmq::stop_impl();
  m_pollTimer->stop();
  stop_channels();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  // Notify JupyterKernel module about kernel stop.
  qSlicerJupyterKernelModule* kernelModule = qobject_cast<qSlicerJupyterKernelModule*>(qSlicerCoreApplication::application()->moduleManager()->module("JupyterKernel"));
  if (kernelModule)
  {
    kernelModule->stopKernel();
  }
}

std::unique_ptr<xeus::xserver> make_xSlicerServer(xeus::xcontext& context,
                                                  const xeus::xconfiguration& config,
                                                  nl::json::error_handler_t eh)
{
  return std::make_unique<xSlicerServer>(context, config, eh);
}

void xSlicerServer::setPollIntervalSec(double intervalSec)
{
  m_pollTimer->setInterval(intervalSec*1000.0);
}

double xSlicerServer::pollIntervalSec()
{
  return m_pollTimer->interval() / 1000.0;
}

void xSlicerServer::poll()
{
  auto msg = poll_channels(-1);
  if (msg)
  {
    if (msg.value().second == xeus::channel::SHELL)
    {
      notify_shell_listener(std::move(msg.value().first));
    }
    else
    {
      notify_control_listener(std::move(msg.value().first));
    }
  }
}
