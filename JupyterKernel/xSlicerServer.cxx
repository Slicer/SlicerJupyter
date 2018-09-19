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
#include <QSocketNotifier>
#ifdef JUPYTER_POLL_USING_TIMER
#include <QTimer>
#endif

xSlicerServer::xSlicerServer(zmq::context_t& context,
                           const xeus::xconfiguration& c)
    : xserver_zmq(context, c)
{
#ifdef JUPYTER_POLL_USING_TIMER
  // 50ms interval is sort enough so that users will not notice significant latency
  // yet it is long enough to minimize CPU load caused by polling.
  m_pollTimer = new QTimer();
  m_pollTimer->setInterval(50);
  QObject::connect(m_pollTimer, &QTimer::timeout, [=]() { poll(0); });
#endif
}

xSlicerServer::~xSlicerServer()
{
#ifdef JUPYTER_POLL_USING_TIMER
  m_pollTimer->stop();
  delete m_pollTimer;
#endif
}

xSlicerServer::socket_notifier_ptr
xSlicerServer::make_socket_notifier(const zmq::socket_t& socket, const QString& name)
{
  Q_UNUSED(name);

  socket_notifier_ptr socket_notifier = socket_notifier_ptr(
        new QSocketNotifier(socket.getsockopt<quintptr>(ZMQ_FD), QSocketNotifier::Read));

  QObject::connect(socket_notifier.data(), &QSocketNotifier::activated, [=](int){
    poll(0);
    });

  return socket_notifier;
}

void xSlicerServer::start_impl(zmq::multipart_t& message)
{
    qDebug() << "Starting Jupyter kernel server";

    start_publisher_thread();
    start_heartbeat_thread();

    m_request_stop = false;

#ifdef JUPYTER_POLL_USING_TIMER
    m_pollTimer->start();
#else
    m_notifiers.append(make_socket_notifier(m_shell, "shell"));
    m_notifiers.append(make_socket_notifier(m_controller, "controller"));
    m_notifiers.append(make_socket_notifier(m_stdin, "stdin"));
    m_notifiers.append(make_socket_notifier(m_controller_pub, "controller_pub"));
    m_notifiers.append(make_socket_notifier(m_publisher_pub, "publisher_pub"));
#endif

    publish(message);
}

void xSlicerServer::stop_impl()
{
  qDebug() << "Stopping Jupyter kernel server";
  this->xserver_zmq::stop_impl();
#ifdef JUPYTER_POLL_USING_TIMER
  m_pollTimer->stop();
#endif
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

