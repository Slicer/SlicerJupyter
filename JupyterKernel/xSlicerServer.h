#ifndef XSLICER_SERVER_HPP
#define XSLICER_SERVER_HPP

// xeus includes
#include <xeus/xserver_zmq.hpp>
#include <xeus/xkernel_configuration.hpp>

#include "qSlicerJupyterKernelModuleExport.h"

// Qt includes
#include <QList>
#include <QSharedPointer>
#include <QSocketNotifier>

// On Windows, socket notifier for stdin socket continuously generates signals,
// which would cause 100% CPU usage even when the application is idle.
// It is not clear why stdin socket behaves like this, but using a timer
// to check for inputs at regular intervals solves the issue.
// If there are problems with socket notifications on other platforms, too,
// then polling can be switched to use timer on those platforms as well.
#if defined(WIN32)
  #define JUPYTER_POLL_USING_TIMER
#else
  #undef JUPYTER_POLL_USING_TIMER
#endif


#ifdef JUPYTER_POLL_USING_TIMER
class QTimer;
#endif

class xSlicerServer : public xeus::xserver_zmq
{

public:
    using socket_notifier_ptr = QSharedPointer<QSocketNotifier>;

    xSlicerServer(zmq::context_t& context,
                 const xeus::xconfiguration& config);

    virtual ~xSlicerServer();

protected:

    void start_impl(zmq::multipart_t& message) override;
    void stop_impl() override;

    socket_notifier_ptr make_socket_notifier(const zmq::socket_t& socket, const QString& name);
    QList< socket_notifier_ptr > m_notifiers;

#ifdef JUPYTER_POLL_USING_TIMER
    QTimer* m_pollTimer;
#endif
};

Q_SLICER_QTMODULES_JUPYTERKERNEL_EXPORT
std::unique_ptr<xeus::xserver> make_xSlicerServer(zmq::context_t& context,
                                                  const xeus::xconfiguration& config);

#endif
