#include "xSlicerServer.h"

// Slicer includes
#include <qSlicerApplication.h>
#include <qSlicerModuleManager.h>
#include "qSlicerJupyterKernelModule.h"

// STL includes
#include <thread>

// zmq includes
#include <zmq_addon.hpp>

// xeus includes
#include <xeus/make_unique.hpp>
#include <xeus/xserver_zmq.hpp>

// Qt includes
#include <QDebug>
#include <QTimer>

xSlicerServer::xSlicerServer(zmq::context_t& context,
                           const xeus::xconfiguration& c)
    : xserver_zmq(context, c)
{
  // 50ms interval is sort enough so that users will not notice significant latency
  // yet it is long enough to minimize CPU load caused by polling.
  m_pollTimer = new QTimer();
  m_pollTimer->setInterval(50);
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

    publish(message);
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
    return ::xeus::make_unique<xSlicerServer>(context, config);
}

