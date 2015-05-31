#ifndef WEB_IMPL_H // header guard
#define WEB_IMPL_H

#include <WebServices.h>
#include "base.h"
#include "exceptions.h"
#include <string>
#include <codecvt>
#include <vector>
#include <thread>
#include <mutex>
#include <memory>

namespace _3fd
{
	using std::string;

	namespace web
	{
		/// <summary>
		/// A reusable object model capable to hold rich error information regarding a web service utilization.
		/// </summary>
		class WSError : notcopiable
		{
		private:

			WS_ERROR *m_wsErrorHandle;

			void Initialize();
			void Reset();

		public:

			WSError();
			~WSError();

			void RaiseExceptionWhenError(
				HRESULT hres,
				const char *funcName,
				const char *message,
				const char *svcName = nullptr);

			void RaiseExceptionWhenError(
				HRESULT hres,
				const char *funcName,
				const char *message,
				const string &svcName)
			{
				RaiseExceptionWhenError(hres, funcName, message, svcName.c_str());
			}

			WS_ERROR *GetHandle();
		};

		/// <summary>
		/// A heap provides precise control over memory allocation when producing or 
		/// consuming messages and when needing to allocate various other API structures.
		/// </summary>
		class WSHeap : notcopiable
		{
		private:

			WS_HEAP *m_wsHeapHandle;
			bool m_allowRelease;

		public:

			WSHeap(WS_HEAP *wsHeapHandle);
			WSHeap();
			~WSHeap();

			void Reset();

			void *Alloc(size_t qtBytes);

			template <typename Type> Type *Alloc()
			{
				return static_cast<Type *> (Alloc(sizeof(Type)));
			}

			template <typename Type> Type *Alloc(size_t qtObjects)
			{
				_ASSERTE(qtObjects > 0);
				return static_cast<Type *> (Alloc(sizeof(Type) * qtObjects));
			}

			/// <summary>
			/// Gets the heap handle.
			/// </summary>
			/// <returns>The handle for the opaque handle object.</returns>
			WS_HEAP *GetHandle() { return m_wsHeapHandle; }
		};

		/// <summary>
		/// Holds key information 
		/// </summary>
		struct SvcEndpointInfo
		{
			string
				portName,
				bindingName,
				bindingNs,
				address;
		};

		/// <summary>
		/// Contains several settings for a service endpoint.
		/// </summary>
		struct SvcEndpointsConfig
		{
			unsigned int
				maxAcceptingChannels, // specifies the maximum number of concurrent channels service host will have actively accepting new connections for a given endpoint
				maxConcurrency; // specifies the maximum number of concurrent calls that would be serviced on a session based channel

			unsigned long
				timeoutDnsResolve, // limits the amount of time (in milliseconds) that will be spent resolving the DNS name
				timeoutSend, // limits the amount of time (in milliseconds) that will be spent sending the HTTP headers and the bytes of the message
				timeoutReceive; // limits the amount of time (in milliseconds) that will be spent receiving the the bytes of the message

			SvcEndpointsConfig() :
				maxAcceptingChannels(1),
				maxConcurrency(1),
				timeoutDnsResolve(5000),
				timeoutSend(2500),
				timeoutReceive(2500)
			{}
		};

		// Callback type for code automatically generated by wsutil.exe
		template <typename FuncTableStructType>
		using CallbackCreateWSEndpointImpl = HRESULT(*)(
			_In_opt_ WS_HTTP_BINDING_TEMPLATE* templateValue,
			_In_opt_ CONST WS_STRING* address,
			_In_opt_ FuncTableStructType* functionTable,
			_In_opt_ WS_SERVICE_SECURITY_CALLBACK authorizationCallback,
			_In_reads_opt_(endpointPropertyCount) WS_SERVICE_ENDPOINT_PROPERTY* endpointProperties,
			_In_ const ULONG endpointPropertyCount,
			_In_ WS_HEAP* heap,
			_Outptr_ WS_SERVICE_ENDPOINT** serviceEndpoint,
			_In_opt_ WS_ERROR* error);

		// Callback type that invokes code automatically generated by wsutil.exe
		typedef HRESULT(*CallbackWrapperCreateWSEndpoint)(
			const string &, // address
			void *, // function table
			WS_SERVICE_SECURITY_CALLBACK,
			WS_SERVICE_ENDPOINT_PROPERTY *, // endpoint properties
			size_t, // endpoint properties count
			WSHeap &,
			WS_SERVICE_ENDPOINT **);

		/// <summary>
		/// A template that performs some parameter translations need to call automatically generated code.
		/// </summary>
		template <typename FuncTableStructType, CallbackCreateWSEndpointImpl<FuncTableStructType> callback>
		HRESULT CreateWSEndpoint(
			const string &address,
			void *functionTable,
			WS_SERVICE_SECURITY_CALLBACK callbackSecurity,
			WS_SERVICE_ENDPOINT_PROPERTY *endpointProps,
			size_t endpointPropsCount,
			web::WSHeap &heap,
			WS_SERVICE_ENDPOINT **endpoint)
		{
			std::wstring_convert<std::codecvt_utf8<wchar_t>> transcoder;
			auto ucs2address = transcoder.from_bytes(address);

			auto wsaddr = heap.Alloc<WS_STRING>();
			wsaddr->length = ucs2address.length();
			wsaddr->chars = heap.Alloc<wchar_t>(ucs2address.length());
			memcpy(wsaddr->chars, ucs2address.data(), ucs2address.length() * sizeof ucs2address[0]);

			WSError err;
			auto hr = callback(
				&WS_HTTP_BINDING_TEMPLATE{},
				wsaddr,
				static_cast<FuncTableStructType *> (functionTable),
				nullptr,
				endpointProps,
				endpointPropsCount,
				heap.GetHandle(),
				endpoint,
				err.GetHandle()
				);
			return hr;
		}

		/// <summary>
		/// Implements the web service host instance.
		/// </summary>
		class WebServiceHostImpl : notcopiable
		{
		private:

			WS_SERVICE_HOST *m_webSvcHostHandle;
			std::vector<char> m_wsdContentBuffer;
			string m_serviceName;
			string m_wsdTargetNs;
			std::vector<SvcEndpointInfo> m_endpointsInfo;
			std::thread m_svcThread;
			std::mutex m_svcStateMutex;
			WSHeap m_svcHeap;

		public:

			WebServiceHostImpl();
			~WebServiceHostImpl();

			void Setup(
				const string &wsdFilePath,
				const SvcEndpointsConfig &config,
				CallbackWrapperCreateWSEndpoint createWebSvcEndpointCallback,
				void *functionTable);

			void OpenAsync();
			bool CloseAsync();
			bool AbortAsync();
		};

		/////////////////////
		// Utilities
		/////////////////////

		void CreateSoapFault(core::IAppException &ex, WS_OPERATION_CONTEXT *wsOperContextHandle, WS_ERROR *wsErrorHandle);

	}// end of namespace web

}// end of namespace _3fd

#endif // end of header guard