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
};

Q_SLICER_QTMODULES_JUPYTERKERNEL_EXPORT
std::unique_ptr<xeus::xserver> make_xSlicerServer(zmq::context_t& context,
                                                  const xeus::xconfiguration& config);

#endif
