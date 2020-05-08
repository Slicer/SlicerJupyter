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

// xeus includes
#include <xeus/xserver_zmq.hpp>

// Qt includes
#include <QDebug>
#include <QTimer>

xSlicerServer::xSlicerServer(zmq::context_t& context,
                           const xeus::xconfiguration& c)
    : xserver_zmq(context, c)
{
  // 10ms interval is short enough so that users will not notice significant latency
  // yet it is long enough to minimize CPU load caused by polling.
  // 50ms causes too long delay in interactive widgets that handle mousemove events.
  m_pollTimer = new QTimer();
  m_pollTimer->setInterval(10);
  QObject::connect(m_pollTimer, &QTimer::timeout, [=]() { poll(0); });
}

xSlicerServer::~xSlicerServer()
{
  m_pollTimer->stop();
  delete m_pollTimer;
}

void xSlicerServer::start_impl(zmq::multipart_t& message)
{
    qDebug() << "Starting Jupyter kernel server";

    start_publisher_thread();
    start_heartbeat_thread();

    m_request_stop = false;

    m_pollTimer->start();

    publish(message, xeus::channel::SHELL);
}

void xSlicerServer::stop_impl()
{
  qDebug() << "Stopping Jupyter kernel server";
  this->xserver_zmq::stop_impl();
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

std::unique_ptr<xeus::xserver> make_xSlicerServer(zmq::context_t& context,
                                                  const xeus::xconfiguration& config)
{
    return std::make_unique<xSlicerServer>(context, config);
}

void xSlicerServer::setPollIntervalSec(double intervalSec)
{
  m_pollTimer->setInterval(intervalSec*1000.0);
}

double xSlicerServer::pollIntervalSec()
{
  return m_pollTimer->interval() / 1000.0;
}
