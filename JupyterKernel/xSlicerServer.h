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

class QTimer;

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

    // Socket notifier for stdin socket continuously generates signals
    // on Windows and on some Linux distributions, which would cause 100% CPU
    // usage even when the application is idle.
    // It is not clear why stdin socket behaves like this, but using a timer
    // to check for inputs at regular intervals solves the issue.
    QTimer* m_pollTimer;
};

Q_SLICER_QTMODULES_JUPYTERKERNEL_EXPORT
std::unique_ptr<xeus::xserver> make_xSlicerServer(zmq::context_t& context,
                                                  const xeus::xconfiguration& config);

#endif
