/***************************************************************************
* Copyright (c) 2016, Johan Mabille and Sylvain Corlay                     *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSLICER_SERVER_HPP
#define XSLICER_SERVER_HPP

#include <thread>

#include "xeus/private/xserver_impl.hpp"
#include "xeus/xkernel_configuration.hpp"

#include "qSlicerJupyterKernelModuleWidgetsExport.h"

namespace xeus
{
    class xSlicerServer : public xserver_impl
    {

    public:

        xSlicerServer(zmq::context_t& context,
                     const xconfiguration& config);

        virtual ~xSlicerServer() = default;

        void poll_slot();

    protected:

        void start_impl(zmq::multipart_t& message) override;

        std::thread m_iopub_thread;
        std::thread m_hb_thread;
    };

    Q_SLICER_MODULE_JUPYTERKERNEL_WIDGETS_EXPORT
    std::unique_ptr<xserver> make_xSlicerServer(zmq::context_t& context,
                                                const xconfiguration& config);
}

#endif
