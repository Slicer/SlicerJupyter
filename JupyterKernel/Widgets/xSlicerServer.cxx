#include "xSlicerServer.h"

#include "zmq_addon.hpp"
#include "xeus/make_unique.hpp"
#include "xeus/private/xserver_impl.hpp"

#include <QTimer>

namespace xeus
{
    xSlicerServer::xSlicerServer(zmq::context_t& context,
                               const xconfiguration& c)
        : xserver_impl(context, c)
    {
    }

    void xSlicerServer::poll_slot()
    {
        if (!m_request_stop)
        {
            poll_impl(10);
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
        m_iopub_thread = std::thread (&xpublisher::run, &m_publisher);
        m_iopub_thread.detach();

        m_hb_thread = std::thread(&xheartbeat::run, &m_heartbeat);
        m_hb_thread.detach();

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

