/***************************************************************************
* Copyright (c) 2016, Johan Mabille and Sylvain Corlay                     *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XKERNEL_HPP
#define XKERNEL_HPP

#include "xeus/xeus.hpp"
#include "xeus/xinterpreter.hpp"
#include "xeus/xkernel_configuration.hpp"
#include "xeus/xkernel_core.hpp"
#include "xeus/xserver.hpp"
#include <memory>
#include <string>

#include "qSlicerJupyterKernelModuleWidgetsExport.h"

namespace xeus
{

    class Q_SLICER_MODULE_JUPYTERKERNEL_WIDGETS_EXPORT xSlicerKernel
    {
    public:

        using interpreter_ptr = std::unique_ptr<xinterpreter>;
        using server_ptr = std::unique_ptr<xserver>;
        using server_builder = server_ptr (*)(zmq::context_t& context,
                                              const xconfiguration& config);
        using kernel_core_ptr = std::unique_ptr<xkernel_core>;

        xSlicerKernel(const xconfiguration& config,
                      const std::string& user_name,
                      interpreter_ptr interpreter,
                      server_builder builder = make_xserver);

        void start();

    private:

        std::string m_kernel_id;
        std::string m_session_id;
        xconfiguration m_config;
        std::string m_user_name;
        interpreter_ptr p_interpreter;
        server_builder m_builder;

        zmq::context_t m_context;
        server_ptr p_server;
        kernel_core_ptr p_core;
    };
}

#endif
