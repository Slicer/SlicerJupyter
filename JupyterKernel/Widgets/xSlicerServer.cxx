#include "xSlicerServer.h"

#include <thread>
#include "zmq_addon.hpp"
#include "xeus/make_unique.hpp"
#include "xeus/xserver_zmq.hpp"

#include <QTimer>

namespace xeus
{
    xSlicerServer::xSlicerServer(zmq::context_t& context,
                               const xconfiguration& c)
        : xserver_zmq(context, c)
    {
    }

    void xSlicerServer::poll_slot()
    {
        if (!m_request_stop)
        {
            poll(10);
            QTimer::singleShot(0, std::bind(&xSlicerServer::poll_slot, this));
        }
        else
        {
            stop_channels();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    void xSlicerServer::start_impl(zmq::multipart_t& message)
    {
        start_publisher_thread();
        start_heartbeat_thread();

        m_request_stop = false;

        publish(message);

        QTimer::singleShot(0, std::bind(&xSlicerServer::poll_slot, this));
    }

    std::unique_ptr<xserver> make_xSlicerServer(zmq::context_t& context, 
                                                const xconfiguration& config)
    {
        return ::xeus::make_unique<xSlicerServer>(context, config);
    }
}

