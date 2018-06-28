#include "xSlicerServer.h"

#include <chrono>
#include "zmq_addon.hpp"
#include "xeus/make_unique.hpp"
#include "xeus/xmiddleware.hpp"

#include <QTimer>

namespace xeus
{
    void init_socket(zmq::socket_t& socket,
        const std::string& end_point)
    {
        socket.setsockopt(ZMQ_LINGER, get_socket_linger());
        socket.bind(end_point);
    }

    xSlicerServer::xSlicerServer(zmq::context_t& context,
                               const xconfiguration& c)
        : m_shell(context, zmq::socket_type::router),
          m_controller(context, zmq::socket_type::router),
          m_stdin(context, zmq::socket_type::router),
          m_publisher_pub(context, zmq::socket_type::pub),
          m_controller_pub(context, zmq::socket_type::pub),
          m_publisher(context, c.m_transport, c.m_ip, c.m_iopub_port),
          m_heartbeat(context, c.m_transport, c.m_ip, c.m_hb_port),
          m_request_stop(false)
    {
        init_socket(m_shell, get_end_point(c.m_transport, c.m_ip, c.m_shell_port));
        init_socket(m_controller, get_end_point(c.m_transport, c.m_ip, c.m_control_port));
        init_socket(m_stdin, get_end_point(c.m_transport, c.m_ip, c.m_stdin_port));
        init_socket(m_publisher_pub, get_publisher_end_point());
        init_socket(m_controller_pub, get_controller_end_point());
    }

    void xSlicerServer::send_shell_impl(zmq::multipart_t& message)
    {
        message.send(m_shell);
    }

    void xSlicerServer::send_control_impl(zmq::multipart_t& message)
    {
        message.send(m_controller);
    }

    void xSlicerServer::send_stdin_impl(zmq::multipart_t& message)
    {
        message.send(m_stdin);
        zmq::multipart_t wire_msg;
        wire_msg.recv(m_stdin);
        xserver::notify_stdin_listener(wire_msg);
    }

    void xSlicerServer::publish_impl(zmq::multipart_t& message)
    {
        message.send(m_publisher_pub);
    }

    void xSlicerServer::poll_impl()
    {
        zmq::pollitem_t items[] = {
            { m_controller, 0, ZMQ_POLLIN, 0 },
            { m_shell, 0, ZMQ_POLLIN, 0 }
        };

        unsigned int timeout_ms = 250;
        zmq::poll(&items[0], 2, timeout_ms * 1000);

        if (items[0].revents & ZMQ_POLLIN)
        {
            zmq::multipart_t wire_msg;
            wire_msg.recv(m_controller);
            xserver::notify_control_listener(wire_msg);
        }
        if (!m_request_stop && (items[1].revents & ZMQ_POLLIN))
        {
            zmq::multipart_t wire_msg;
            wire_msg.recv(m_shell);
            xserver::notify_shell_listener(wire_msg);
        }
    }

    void xSlicerServer::poll_slot()
    {
        if (!m_request_stop)
        {
            poll_impl();

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

    void xSlicerServer::abort_queue_impl(const listener& l, long polling_interval)
    {
        while (true)
        {
            zmq::multipart_t wire_msg;
            bool msg = wire_msg.recv(m_shell, ZMQ_NOBLOCK);
            if (!msg)
            {
                return;
            }

            l(wire_msg);
            std::this_thread::sleep_for(std::chrono::milliseconds(polling_interval));
        }
    }

    void xSlicerServer::stop_impl()
    {
        m_request_stop = true;
    }

    void xSlicerServer::stop_channels()
    {
        zmq::message_t stop_msg("stop", 4);
        m_controller_pub.send(stop_msg);
    }

    std::unique_ptr<xserver> make_xSlicerServer(zmq::context_t& context, 
                                                const xconfiguration& config)
    {
        return ::xeus::make_unique<xSlicerServer>(context, config);
    }
}

