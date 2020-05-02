#ifndef xSlicerInterpreter_h
#define xSlicerInterpreter_h

//#include <xeus/xinterpreter.hpp>

#include <xeus-python/xinterpreter.hpp>

#include <QStringList>

//using xpyt::interpreter;

class qSlicerJupyterKernelModule;

class xSlicerInterpreter : public xpyt::interpreter
{

public:

    xSlicerInterpreter();
    virtual ~xSlicerInterpreter() = default;

    void set_jupyter_kernel_module(qSlicerJupyterKernelModule* module);

private:

    void configure_impl() override;

    nl::json execute_request_impl(int execution_counter,
                               const std::string& code,
                               bool store_history,
                               bool silent,
                               nl::json user_expressions,
                               bool allow_stdin) override;

    nl::json complete_request_impl(const std::string& code,
                                int cursor_pos) override;

    nl::json inspect_request_impl(const std::string& code,
                               int cursor_pos,
                               int detail_level) override;

    nl::json is_complete_request_impl(const std::string& code) override;

    nl::json kernel_info_request_impl() override;

    void shutdown_request_impl() override;

    bool m_print_debug_output = false;
    qSlicerJupyterKernelModule* m_jupyter_kernel_module = nullptr;
};

#endif
