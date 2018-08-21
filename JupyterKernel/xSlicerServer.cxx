#include "xSlicerServer.h"

// Slicer includes
#include <qSlicerApplication.h>

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

xSlicerServer::xSlicerServer(zmq::context_t& context,
                           const xeus::xconfiguration& c)
    : xserver_zmq(context, c)
{
}

xSlicerServer::~xSlicerServer()
{
}

xSlicerServer::socket_notifier_ptr
xSlicerServer::make_socket_notifier(const zmq::socket_t& socket, const QString& name)
{
  Q_UNUSED(name);

  socket_notifier_ptr socket_notifier = socket_notifier_ptr(
        new QSocketNotifier(socket.getsockopt<quintptr>(ZMQ_FD), QSocketNotifier::Read));

  QObject::connect(socket_notifier.data(), &QSocketNotifier::activated, [=](int){
    poll(10);
    });

  return socket_notifier;
}

void xSlicerServer::start_impl(zmq::multipart_t& message)
{
    qDebug() << "Starting Jupyter kernel server";

    start_publisher_thread();
    start_heartbeat_thread();

    m_notifiers.append(make_socket_notifier(m_shell, "shell"));
    m_notifiers.append(make_socket_notifier(m_controller, "controller"));
    m_notifiers.append(make_socket_notifier(m_stdin, "stdin"));
    m_notifiers.append(make_socket_notifier(m_controller_pub, "controller_pub"));
    m_notifiers.append(make_socket_notifier(m_publisher_pub, "publisher_pub"));

    m_request_stop = false;

    publish(message);
}

void xSlicerServer::stop_impl()
{
  qDebug() << "Stopping Jupyter kernel server";
  this->xserver_zmq::stop_impl();
  stop_channels();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

std::unique_ptr<xeus::xserver> make_xSlicerServer(zmq::context_t& context,
                                                  const xeus::xconfiguration& config)
{
    return ::xeus::make_unique<xSlicerServer>(context, config);
}

