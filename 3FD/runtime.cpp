#include "stdafx.h"
#include "runtime.h"
#include "callstacktracer.h"
#include "logger.h"
#include "gc.h"

#ifdef _3FD_PLATFORM_WINRT
#	include "sqlite3.h"
#	include <codecvt>
#endif

namespace _3fd
{
	namespace core
	{
		////////////////////////////////////
		//  FrameworkInstance Class
		////////////////////////////////////

		/// <summary>
		/// Initializes a new instance of the <see cref="FrameworkInstance" /> class.
		/// </summary>
		FrameworkInstance::FrameworkInstance()
		{
			Logger::Write("3FD has been initialized", core::Logger::PRIO_DEBUG);

#ifdef _3FD_PLATFORM_WINRT
			using namespace Windows::Storage;

			std::wstring_convert<std::codecvt_utf8<wchar_t>> transcoder;

			string tempFolderPath = transcoder.to_bytes(
				ApplicationData::Current->TemporaryFolder->Path->Data()
			);
			
			auto tempDirStrSize = tempFolderPath.length() + 1;
			sqlite3_temp_directory = (char *)sqlite3_malloc(tempDirStrSize);

			if (sqlite3_temp_directory == nullptr)
				throw std::bad_alloc();

			strncpy(sqlite3_temp_directory, tempFolderPath.data(), tempDirStrSize);
#endif
		}

		/// <summary>
		/// Finalizes an instance of the <see cref="FrameworkInstance"/> class.
		/// </summary>
		FrameworkInstance::~FrameworkInstance()
		{
			memory::GarbageCollector::Shutdown();
			CallStackTracer::Shutdown();

			Logger::Write("3FD was shutdown", core::Logger::PRIO_DEBUG);
			Logger::Shutdown();

#ifdef _3FD_PLATFORM_WINRT
			sqlite3_free(sqlite3_temp_directory);
#endif
		}
	}
}