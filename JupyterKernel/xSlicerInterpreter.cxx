#include <iostream>

#include "xSlicerInterpreter.h"

#include <xeus/xguid.hpp>

#include <qSlicerApplication.h>
#include <qSlicerPythonManager.h>

#include "qSlicerJupyterKernelModule.h"

#include <QObject>

#include <PythonQt.h>

xSlicerInterpreter::xSlicerInterpreter()
  : xpyt::interpreter(false, false)
  // Disable built-in output and display redirection, as it would prevent
  // console output and execution results from appearing in Slicer's
  // Python console and application log.
  // We call publish_stream instead when PythonQt emits outputs and
  // use a custom display hook.
{
  // GIL is already released, so we need to prevent
  // the interpreter from attempting to release it again.
  m_release_gil_at_startup = false;
}


void xSlicerInterpreter::configure_impl()
{
  xpyt::interpreter::configure_impl();

  auto handle_comm_opened = [](xeus::xcomm&& comm, const xeus::xmessage&) {
    std::cout << "Comm opened for target: " << comm.target().name() << std::endl;
  };
  comm_manager().register_comm_target("echo_target", handle_comm_opened);

  // Custom output redirection
  QObject::connect(PythonQt::self(), &PythonQt::pythonStdOut,
    [=](const QString& text) {
    publish_stream("stdout", text.toStdString());
  });
  QObject::connect(PythonQt::self(), &PythonQt::pythonStdErr,
    [=](const QString& text) {
    publish_stream("stderr", text.toStdString());
  });

  // Custom display redirection
  // Make xeus-python display hook available as slicer.xeusPythonDisplayHook
  py::module slicer_module = py::module::import("slicer");
  slicer_module.attr("xeusPythonDisplayHook") = m_displayhook;
}

nl::json xSlicerInterpreter::execute_request_impl(int execution_counter,
  const std::string& code,
  bool store_history,
  bool silent,
  nl::json user_expressions,
  bool allow_stdin)
{
  if (m_print_debug_output)
  {
    std::cout << "Received execute_request" << std::endl;
    std::cout << "execution_counter: " << execution_counter << std::endl;
    std::cout << "code: " << code << std::endl;
    std::cout << "store_history: " << store_history << std::endl;
    std::cout << "silent: " << silent << std::endl;
    std::cout << "allow_stdin: " << allow_stdin << std::endl;
    std::cout << std::endl;
  }

  qSlicerPythonManager* pythonManager = qSlicerApplication::application()->pythonManager();

  nl::json pub_data;
  nl::json result;
  QString qscode = QString::fromUtf8(code.c_str());
  if (qscode.endsWith(QString("__kernel_debug_enable()")))
  {
    m_print_debug_output = true;
    pub_data["text/plain"] = "Kernel debug info print enabled.";
    result["status"] = "ok";
  }
  else if (qscode.endsWith(QString("__kernel_debug_disable()")))
  {
    m_print_debug_output = false;
    pub_data["text/plain"] = "Kernel debug info print disabled.";
    result["status"] = "ok";
  }
  else
  {
    return xpyt::interpreter::execute_request_impl(execution_counter, code, store_history, silent, user_expressions, allow_stdin);
  }

  publish_execution_result(execution_counter, std::move(pub_data), nl::json::object());
  return result;
}

nl::json xSlicerInterpreter::complete_request_impl(const std::string& code,
  int cursor_pos)
{
  if (m_print_debug_output)
  {
    std::cout << "Received complete_request" << std::endl;
    std::cout << "code: " << code << std::endl;
    std::cout << "cursor_pos: " << cursor_pos << std::endl;
    std::cout << std::endl;
  }

  return xpyt::interpreter::complete_request_impl(code, cursor_pos);
}

nl::json xSlicerInterpreter::inspect_request_impl(const std::string& code,
  int cursor_pos,
  int detail_level)
{
  if (m_print_debug_output)
  {
    std::cout << "Received inspect_request" << std::endl;
    std::cout << "code: " << code << std::endl;
    std::cout << "cursor_pos: " << cursor_pos << std::endl;
    std::cout << "detail_level: " << detail_level << std::endl;
    std::cout << std::endl;
  }

  return xpyt::interpreter::inspect_request_impl(code, cursor_pos, detail_level);
}

nl::json xSlicerInterpreter::is_complete_request_impl(const std::string& code)
{
  if (m_print_debug_output)
  {
    std::cout << "Received is_complete_request" << std::endl;
    std::cout << "code: " << code << std::endl;
    std::cout << std::endl;
  }
  return xpyt::interpreter::is_complete_request_impl(code);

}

nl::json xSlicerInterpreter::kernel_info_request_impl()
{
  return xpyt::interpreter::kernel_info_request_impl();
}

void xSlicerInterpreter::shutdown_request_impl()
{
  if (m_print_debug_output)
  {
    std::cout << "Received shutdown_request" << std::endl;
    std::cout << std::endl;
  }
  return xpyt::interpreter::shutdown_request_impl();
}

void xSlicerInterpreter::set_jupyter_kernel_module(qSlicerJupyterKernelModule* module)
{
  m_jupyter_kernel_module = module;
}
