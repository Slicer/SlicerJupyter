#include <iostream>

#include "xSlicerInterpreter.h"

#include <xeus/xguid.hpp>

#include <qMRMLPlotView.h>
#include <qMRMLPlotViewControllerWidget.h>
#include <qMRMLPlotWidget.h>
#include <qMRMLSliceWidget.h>
#include <qMRMLSliceControllerWidget.h>
#include <qMRMLSliceView.h>
#include <qMRMLTableView.h>
#include <qMRMLTableViewControllerWidget.h>
#include <qMRMLTableWidget.h>
#include <qMRMLThreeDViewControllerWidget.h>
#include <qMRMLThreeDWidget.h>
#include <qMRMLThreeDView.h>

#include <qSlicerApplication.h>
#include <qSlicerLayoutManager.h>
#include <qSlicerPythonManager.h>

#include "qSlicerJupyterKernelModule.h"

#include <QBuffer>
#include <QObject>

#include <PythonQt.h>

void xSlicerInterpreter::configure_impl()
{
  auto handle_comm_opened = [](xeus::xcomm&& comm, const xeus::xmessage&) {
    std::cout << "Comm opened for target: " << comm.target().name() << std::endl;
  };
  comm_manager().register_comm_target("echo_target", handle_comm_opened);
  //using function_type = std::function<void(xeus::xcomm&&, const xeus::xmessage&)>;

  QObject::connect(PythonQt::self(), &PythonQt::pythonStdOut,
    [=](const QString& text) {
    m_captured_stdout << text;
  });

  QObject::connect(PythonQt::self(), &PythonQt::pythonStdErr,
    [=](const QString& text) {
    m_captured_stderr << text;
  });
}

nl::json xSlicerInterpreter::execute_request_impl(int execution_counter,
  const std::string& code,
  bool store_history,
  bool silent,
  nl::json /* user_expressions */,
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

  m_captured_stdout.clear();
  m_captured_stderr.clear();

  qSlicerPythonManager* pythonManager = qSlicerApplication::application()->pythonManager();

  nl::json pub_data;
  QString qscode = QString::fromUtf8(code.c_str());
  QString displayCommand = "display()";
  if (qscode.endsWith(QString("__kernel_debug_enable()")))
  {
    m_print_debug_output = true;
    pub_data["text/plain"] = "Kernel debug info print enabled.";
  }
  else if (qscode.endsWith(QString("__kernel_debug_disable()")))
  {
    m_print_debug_output = false;
    pub_data["text/plain"] = "Kernel debug info print disabled.";
  }
  else
  {
    QVariant executeResult = pythonManager->executeString(QString::fromStdString(code));
    if (m_jupyter_kernel_module && !m_jupyter_kernel_module->executeResultDataType().isEmpty())
    {
      pub_data[m_jupyter_kernel_module->executeResultDataType().toStdString()] =
        m_jupyter_kernel_module->executeResultDataValue().toStdString();
      m_jupyter_kernel_module->setExecuteResultDataType("");
      m_jupyter_kernel_module->setExecuteResultDataValue("");
    }
    else
    {
      pub_data["text/plain"] = m_captured_stdout.join("").toStdString();
    }
  }

  nl::json result;
  result["status"] = "ok";
  if (pythonManager->pythonErrorOccured())
  {
    result["status"] = "error";
    pub_data["text/plain"] = m_captured_stderr.join("").toStdString();
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

  qSlicerPythonManager* pythonManager = qSlicerApplication::application()->pythonManager();
  PythonQtObjectPtr context = pythonManager->mainContext();

  QVariantList args;
  args.push_back(QString::fromStdString(code));
  args.push_back(cursor_pos);
  QVariant executeResult = context.call("slicer.util.py_complete_request", args);

  if (m_print_debug_output)
  {
    std::cout << "result: " << std::endl << executeResult.toString().toStdString() << std::endl;
  }

  // TODO error check?
  std::string resultStr = executeResult.toString().toStdString();
  nl::json result = nl::json::parse(resultStr.c_str());
  return result;
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

  qSlicerPythonManager* pythonManager = qSlicerApplication::application()->pythonManager();
  PythonQtObjectPtr context = pythonManager->mainContext();

  QVariant executeResult;

  // Get token at cursor (if inside parentheses after method name, it returns the method name)
  std::string token;
  {
    QVariantList args;
    args.push_back(QString::fromStdString(code));
    args.push_back(cursor_pos);
    executeResult = context.call("slicer.util.py_token_at_cursor", args);
    token = executeResult.toString().toStdString();
    if (m_print_debug_output)
    {
      std::cout << "slicer.util.py_token_at_cursor result: " << std::endl << executeResult.toString().toStdString() << std::endl;
    }
    // TODO error check?
  }

  // Get documentation
  std::string documentation;
  {
    QVariantList args;
    args.push_back(QString::fromStdString(token));
    args.push_back(static_cast<unsigned int>(token.size()));
    args.push_back(detail_level);
    executeResult = context.call("slicer.util.py_inspect_request", args);
    documentation = executeResult.toString().toStdString();
    if (m_print_debug_output)
    {
      std::cout << "slicer.util.py_inspect_request result: " << std::endl << executeResult.toString().toStdString() << std::endl;
    }
    // TODO error check?
  }

  nl::json result = nl::json::parse(documentation.c_str());

  return result;
}

nl::json xSlicerInterpreter::is_complete_request_impl(const std::string& code)
{
  if (m_print_debug_output)
  {
    std::cout << "Received is_complete_request" << std::endl;
    std::cout << "code: " << code << std::endl;
    std::cout << std::endl;
  }
  nl::json result;
  result["status"] = "complete";
  return result;
}

nl::json xSlicerInterpreter::kernel_info_request_impl()
{
  nl::json result;
  result["language_info"]["mimetype"] = "text/x-python";
  result["language_info"]["name"] = "python";
  result["language_info"]["nbconvert_exporter"] = "python";
  result["language_info"]["version"] = "2.7.13+";
  result["language_info"]["file_extension"] = ".py";
  result["language_info"]["pygments_lexer"] = "ipython3";
  result["language_info"]["codemirror_mode"]["version"] = 3;
  result["language_info"]["codemirror_mode"]["name"] = "ipython";
  return result;
}

void xSlicerInterpreter::shutdown_request_impl()
{
  if (m_print_debug_output)
  {
    std::cout << "Received shutdown_request" << std::endl;
    std::cout << std::endl;
  }
}

void xSlicerInterpreter::set_jupyter_kernel_module(qSlicerJupyterKernelModule* module)
{
  m_jupyter_kernel_module = module;
}
