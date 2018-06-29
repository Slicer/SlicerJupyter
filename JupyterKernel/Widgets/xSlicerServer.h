#ifndef XSLICER_SERVER_HPP
#define XSLICER_SERVER_HPP

// xeus includes
#include <xeus/xserver_zmq.hpp>
#include <xeus/xkernel_configuration.hpp>

#include "qSlicerJupyterKernelModuleWidgetsExport.h"

class xSlicerServer : public xeus::xserver_zmq
{

public:

    xSlicerServer(zmq::context_t& context,
                 const xeus::xconfiguration& config);

    virtual ~xSlicerServer() = default;

    void poll_slot();

protected:

    void start_impl(zmq::multipart_t& message) override;
};

Q_SLICER_MODULE_JUPYTERKERNEL_WIDGETS_EXPORT
std::unique_ptr<xeus::xserver> make_xSlicerServer(zmq::context_t& context,
                                                  const xeus::xconfiguration& config);

#endif
