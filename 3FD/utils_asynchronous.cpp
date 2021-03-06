#include "stdafx.h"
#include "utils.h"
#include "exceptions.h"
#include "logger.h"

#include <sstream>
#include <thread>

namespace _3fd
{
namespace utils
{
    ////////////////////////////////
    // Asynchronous Class
    ////////////////////////////////

    /// <summary>
    /// Invokes a callback asynchronously and leaves without waiting for termination.
    /// </summary>
    /// <param name="callback">The callback.</param>
    void Asynchronous::InvokeAndLeave(const std::function<void()> &callback)
    {
        CALL_STACK_TRACE;

        try
        {
            // Launch asynchronous execution of callback and then a little housekeeping:
            std::thread thread([callback]()
            {
                try
                {
                    callback(); // invoke the callback
                }
                catch (core::IAppException &ex)
                {
                    core::Logger::Write(ex, core::Logger::PRIO_ERROR);
                }
                catch (std::exception &ex)
                {
                    std::ostringstream oss;
                    oss << "Generic failure when executing callback asynchronously: " << ex.what();
                    core::Logger::Write(oss.str(), core::Logger::PRIO_ERROR);
                }
                catch (...)
                {
                    std::ostringstream oss;
                    oss << "Unexpected exception during asynchronous execution of callback";
                    core::Logger::Write(oss.str(), core::Logger::PRIO_ERROR);
                }
            });

            thread.detach();
        }
        catch (std::system_error &ex)
        {
            std::ostringstream oss;
            oss << "System failure when starting new asynchronous execution: " << core::StdLibExt::GetDetailsFromSystemError(ex);
            throw core::AppException<std::runtime_error>(oss.str());
        }
        catch (std::exception &ex)
        {
            std::ostringstream oss;
            oss << "Generic failure when starting new asynchronous execution: " << ex.what();
            throw core::AppException<std::runtime_error>(oss.str());
        }
    }

} // end of namespace utils
} // end of namespace _3fd
