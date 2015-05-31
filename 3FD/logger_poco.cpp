#include "stdafx.h"
#include "logger.h"
#include "configuration.h"
#include "callstacktracer.h"

#include <Poco/AutoPtr.h>
#include <Poco/Channel.h>
#include <Poco/AsyncChannel.h>
#include <Poco/FileChannel.h>
#include <Poco/ConsoleChannel.h>
#include <Poco/FormattingChannel.h>
#include <Poco/PatternFormatter.h>

#include <iostream>
#include <sstream>
#include <chrono>
#include <array>
#include <stack>

namespace _3fd
{
	namespace core
	{
		/// <summary>
		/// Attempts to output a message to the console.
		/// </summary>
		/// <param name="message">The message.</param>
		static void AttemptConsoleOutput(const string &message)
		{/* As standard procedure in this framework, exceptions thrown inside destructors are swallowed and logged.
		 So, when this functions is invoked, exceptions cannot be forwarded up in the stack, because the original
		 caller might be a destructor. The 'catch' clause below attempts to output the event in the console (if
		 available). */
#ifdef _3FD_CONSOLE_AVAILABLE
			using namespace std::chrono;
			std::array<char, 21> buffer;
			auto now = system_clock::to_time_t(system_clock::now());
			strftime(buffer.data(), buffer.size(), "%Y-%b-%d %H:%M:%S", localtime(&now));
            std::clog << "@(" << buffer.data() << ")\t" << message << std::endl;
#endif
		}

		/// <summary>
		/// Gets the unique instance of the singleton <see cref="Logger" /> class.
		/// </summary>
		/// <returns>A pointer to the singleton.</returns>
		Logger * Logger::GetInstance()
		{
			if (uniqueObjectPtr != nullptr)
				return uniqueObjectPtr;
			else
			{
				try
				{
					CreateInstance(AppConfig::GetApplicationId(),
						AppConfig::GetSettings().common.log.writeToConsole);
				}
				catch (IAppException &appEx)
				{
					std::ostringstream oss;
					oss << "The logging facility creation failed with an exception - " << appEx.GetErrorMessage();
					AttemptConsoleOutput(oss.str());
				}

				return uniqueObjectPtr;
			}
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="Logger"/> class.
		/// </summary>
		/// <param name="id">The id of the server, but also the name of the POCO logger.</param>
		/// <param name="logToConsole">Whether the console or a text file is to receive the log output.</param>
		Logger::Logger(const string &id, bool logToConsole)
			: m_logger(nullptr)
		{
			using Poco::AutoPtr;

			// Set up the POCO logger:
			try
			{
				m_logger = &Poco::Logger::get(id);
#	ifdef NDEBUG
				m_logger->setLevel(Poco::Message::PRIO_INFORMATION);
#	else
				m_logger->setLevel(Poco::Message::PRIO_DEBUG);
#	endif
				// Record the log to a file channel. (The name of the file is the same of the server ID.)
				AutoPtr<Poco::Channel> channel;

				if (logToConsole == false)
				{
					channel = new Poco::FileChannel(id + ".log");
					channel->setProperty("archive", "timestamp");
					channel->setProperty("rotation", "never");
					channel->setProperty("times", "local");

					std::ostringstream oss;
					oss << AppConfig::GetSettings().common.log.purgeAge << " days";
					channel->setProperty("purgeAge", oss.str());

					oss.str("");
					oss << AppConfig::GetSettings().common.log.purgeCount;
					channel->setProperty("purgeCount", oss.str());

					/* Create an asynchronous channel, so the log will be recorded in a parallel thread,
					preventing the other threads from waiting for disk flushes. */
					channel = new Poco::AsyncChannel(channel);
				}
				else
					channel = new Poco::ConsoleChannel();

				// Create a message formatter for the exceptions' informations:
				AutoPtr<Poco::PatternFormatter> formatter(new Poco::PatternFormatter());
				formatter->setProperty("times", "local");
				formatter->setProperty("pattern", "%Y-%b-%d %H:%M:%S [process %P] - %t");

				// Perform formatting before passing the messages to the asynchronous channel
				AutoPtr<Poco::FormattingChannel> formattingChannel(new Poco::FormattingChannel(formatter, channel));

				// The logger channel is now formatted, asynchronous and recorded to a file
				m_logger->setChannel(formattingChannel);
			}
			catch (Poco::Exception &ex)
			{
				if (m_logger != nullptr)
				{
					Poco::Logger::destroy(m_logger->name());
					m_logger = nullptr;
				}

				std::ostringstream oss;
				oss << "There was a failure when trying to set up the logger. POCO C++ library reported: " << ex.what();
				AttemptConsoleOutput(oss.str());
			}
			catch (std::bad_alloc &ex)
			{
				if (m_logger != nullptr)
				{
					Poco::Logger::destroy(m_logger->name());
					m_logger = nullptr;
				}

				AttemptConsoleOutput("Failed to allocate memory when trying to set up the logger");
			}
			catch (...)
			{
				if (m_logger != nullptr)
				{
					Poco::Logger::destroy(m_logger->name());
					m_logger = nullptr;
				}

				AttemptConsoleOutput("Unexpected exception was caught when trying to set up the logger");
			}
			/* Even when the set-up of the logger fails, the application must continue to execute,
			because the logger is merely an auxiliary service. */
		}

		/// <summary>
		/// Finalizes an instance of the <see cref="Logger"/> class.
		/// </summary>
		Logger::~Logger()
		{
			if (m_logger != nullptr)
				Poco::Logger::destroy(m_logger->name());
		}

		/// <summary>
		/// Finishes writing a log event string.
		/// </summary>
		/// <param name="oss">The output string stream.</param>
		/// <param name="details">The event details.</param>
		/// <param name="cst">When set to <c>true</c>, append the call stack trace.</param>
		static void FinishEventString(std::ostringstream &oss, const string &details, bool cst)
		{
#	ifdef ENABLE_3FD_ERR_IMPL_DETAILS
			if (details.empty() == false)
				oss << " - " << details;
#	endif
#	ifdef ENABLE_3FD_CST
			if (cst && CallStackTracer::IsReady())
				oss << " - Call Stack: { " << CallStackTracer::GetInstance().GetStackReport() << " }";
#	endif
		}

		/// <summary>
		/// Writes a message and its details to the log output.
		/// </summary>
		/// <param name="what">The reason for the message.</param>
		/// <param name="details">The message details.</param>
		/// <param name="prio">The priority for the message.</param>
		/// <param name="cst">When set to <c>true</c>, append the call stack trace.</param>
		void Logger::WriteImpl(string &&what, string &&details, Priority prio, bool cst)
		{
			try
			{
				if (m_logger == nullptr)
					return;

				std::ostringstream oss;

				switch (prio)
				{
				case PRIO_FATAL:
					oss << "FATAL - " << what;
					FinishEventString(oss, details, cst);
					m_logger->fatal(oss.str());
					break;

				case PRIO_CRITICAL:
					oss << "CRITICAL - " << what;
					FinishEventString(oss, details, cst);
					m_logger->critical(oss.str());
					break;

				case PRIO_ERROR:
					oss << "ERROR - " << what;
					FinishEventString(oss, details, cst);
					m_logger->error(oss.str());
					break;

				case PRIO_WARNING:
					oss << "WARNING - " << what;
					FinishEventString(oss, details, cst);
					m_logger->warning(oss.str());
					break;

				case PRIO_NOTICE:
					oss << "NOTICE - " << what;
					FinishEventString(oss, details, cst);
					m_logger->notice(oss.str());
					break;

				case PRIO_INFORMATION:
					oss << "INFORMATION - " << what;
					FinishEventString(oss, details, cst);
					m_logger->information(oss.str());
					break;

				case PRIO_DEBUG:
					oss << "DEBUG - " << what;
					FinishEventString(oss, details, cst);
					m_logger->debug(oss.str());
					break;

				case PRIO_TRACE:
					oss << "TRACE - " << what;
					FinishEventString(oss, details, cst);
					m_logger->trace(oss.str());
					break;

				default:
					break;
				}
			}
			catch (Poco::Exception &ex)
			{
				std::ostringstream oss;
				oss << "Failed to write in log output. POCO C++ library reported an exception that had to be swallowed: " << ex.message();
				AttemptConsoleOutput(oss.str());
			}
			catch (std::exception &ex)
			{
				std::ostringstream oss;
				oss << "Failed to write in log output. An exception had to be swallowed: " << ex.what();
				AttemptConsoleOutput(oss.str());
			}
		}

	}// end of namespace core
}// end of namespace _3fd
