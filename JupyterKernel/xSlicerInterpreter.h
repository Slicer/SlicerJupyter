#ifndef xSlicerInterpreter_h
#define xSlicerInterpreter_h

#include <xeus/xinterpreter.hpp>

#include <QStringList>

using xeus::xinterpreter;
using xeus::xjson;
using xeus::xjson_node;
using xeus::xhistory_arguments;

class qSlicerJupyterKernelModule;

class xSlicerInterpreter : public xinterpreter
{

public:

    xSlicerInterpreter() = default;
    virtual ~xSlicerInterpreter() = default;

    void set_jupyter_kernel_module(qSlicerJupyterKernelModule* module);

private:

    void configure_impl() override;

    xjson execute_request_impl(int execution_counter,
                               const std::string& code,
                               bool silent,
                               bool store_history,
                               const xjson_node* user_expressions,
                               bool allow_stdin) override;

    xjson complete_request_impl(const std::string& code,
                                int cursor_pos) override;

    xjson inspect_request_impl(const std::string& code,
                               int cursor_pos,
                               int detail_level) override;

    xjson history_request_impl(const xhistory_arguments& args) override;

    xjson is_complete_request_impl(const std::string& code) override;

    xjson kernel_info_request_impl() override;

    void input_reply_impl(const std::string& value) override;

    QStringList m_captured_stdout;
    QStringList m_captured_stderr;

    bool m_print_debug_output = false;
    qSlicerJupyterKernelModule* m_jupyter_kernel_module = nullptr;
};

#endif
