#include <iostream>

#include "xSlicerInterpreter.h"

#include <xeus/xguid.hpp>

#include <qMRMLSliceWidget.h>
#include <qMRMLSliceView.h>
#include <qMRMLThreeDWidget.h>
#include <qMRMLThreeDView.h>

#include <qSlicerApplication.h>
#include <qSlicerLayoutManager.h>
#include <qSlicerPythonManager.h>

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

xjson xSlicerInterpreter::execute_request_impl(int execution_counter,
  const std::string& code,
  bool silent,
  bool store_history,
  const xjson_node* /* user_expressions */,
  bool allow_stdin)
{
  if (m_print_debug_output)
  {
    std::cout << "Received execute_request" << std::endl;
    std::cout << "execution_counter: " << execution_counter << std::endl;
    std::cout << "code: " << code << std::endl;
    std::cout << "silent: " << silent << std::endl;
    std::cout << "store_history: " << store_history << std::endl;
    std::cout << "allow_stdin: " << allow_stdin << std::endl;
    std::cout << std::endl;
  }

  m_captured_stdout.clear();
  m_captured_stderr.clear();

  qSlicerPythonManager* pythonManager = qSlicerApplication::application()->pythonManager();

  xjson pub_data;
  QString qscode = QString::fromUtf8(code.c_str());
  QString displayCommand = "display()";
  if (qscode.endsWith(displayCommand))
  {
    QVariant executeResult = pythonManager->executeString(qscode.left(qscode.length() - displayCommand.length()));
    pub_data["image/png"] = execute_display_command();
  }
  else if (qscode.endsWith("__kernel_debug_enable()"))
  {
    m_print_debug_output = true;
  }
  else if (qscode.endsWith("__kernel_debug_disable()"))
  {
    m_print_debug_output = false;
  }
  else
  {
    QVariant executeResult = pythonManager->executeString(QString::fromStdString(code));
    pub_data["text/plain"] = m_captured_stdout.join("").toStdString();
  }

  xjson result;
  result["status"] = "ok";
  if (pythonManager->pythonErrorOccured())
  {
    result["status"] = "error";
    pub_data["text/plain"] = m_captured_stderr.join("").toStdString();
  }

  publish_execution_result(execution_counter, std::move(pub_data), xjson::object());

  return result;
}

std::string xSlicerInterpreter::execute_display_command()
{
  // Make sure display updates are completed
  qSlicerApplication::application()->processEvents();
  qSlicerLayoutManager* layoutManager = qSlicerApplication::application()->layoutManager();
  for (int viewIndex = 0; viewIndex < layoutManager->threeDViewCount(); viewIndex++)
  {
    layoutManager->threeDWidget(viewIndex)->threeDView()->forceRender();
  }
  foreach(QString sliceViewName, layoutManager->sliceViewNames())
  {
    layoutManager->sliceWidget(sliceViewName)->sliceView()->forceRender();
  }

  // base64 encoding
  QPixmap screenshot = layoutManager->viewport()->grab();
  QByteArray bArray;
  QBuffer buffer(&bArray);
  buffer.open(QIODevice::WriteOnly);
  screenshot.save(&buffer, "PNG");
  QString base64 = QString::fromLatin1(bArray.toBase64().data());
  return base64.toStdString();
}

xjson xSlicerInterpreter::complete_request_impl(const std::string& code,
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
  xjson result = xjson::parse(executeResult.toString().toStdString());
  return result;
}

xjson xSlicerInterpreter::inspect_request_impl(const std::string& code,
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

  QVariantList args;
  args.push_back(QString::fromStdString(code));
  args.push_back(cursor_pos);
  args.push_back(detail_level);
  QVariant executeResult = context.call("slicer.util.py_inspect_request", args);

  if (m_print_debug_output)
  {
    std::cout << "result: " << std::endl << executeResult.toString().toStdString() << std::endl;
  }

  // TODO error check?
  xjson result = xjson::parse(executeResult.toString().toStdString());
  return result;
}

xjson xSlicerInterpreter::history_request_impl(const xhistory_arguments& args)
{
  if (m_print_debug_output)
  {
    std::cout << "Received history_request" << std::endl;
    std::cout << "output: " << args.m_output << std::endl;
    std::cout << "raw: " << args.m_raw << std::endl;
    std::cout << "hist_access_type: " << args.m_hist_access_type << std::endl;
    std::cout << "session: " << args.m_session << std::endl;
    std::cout << "start: " << args.m_start << std::endl;
    std::cout << "stop: " << args.m_stop << std::endl;
    std::cout << "n: " << args.m_n << std::endl;
    std::cout << "pattern: " << args.m_pattern << std::endl;
    std::cout << "unique: " << args.m_unique << std::endl;
    std::cout << std::endl;
  }

  xjson result;
  result["history"] = { {args.m_session, 0, ""} };
  return result;
}

xjson xSlicerInterpreter::is_complete_request_impl(const std::string& code)
{
  if (m_print_debug_output)
  {
    std::cout << "Received is_complete_request" << std::endl;
    std::cout << "code: " << code << std::endl;
    std::cout << std::endl;
  }
  xjson result;
  result["status"] = "complete";
  return result;
}

xjson xSlicerInterpreter::kernel_info_request_impl()
{
  xjson result;
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

void xSlicerInterpreter::input_reply_impl(const std::string& value)
{
  if (m_print_debug_output)
  {
    std::cout << "Received input_reply" << std::endl;
    std::cout << "value: " << value << std::endl;
  }
}
