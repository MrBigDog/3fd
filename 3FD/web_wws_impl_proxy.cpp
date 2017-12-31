#include "stdafx.h"
#include "web_wws_impl_proxy.h"
#include "configuration.h"
#include "callstacktracer.h"
#include "logger.h"
#include "utils_io.h"

#define format utils::FormatArg

#include <cstdlib>
#include <codecvt>
#include <sstream>
#include <thread>
#include <ctime>

namespace _3fd
{
namespace web
{
namespace wws
{
    using namespace _3fd::core;


    /// <summary>
    /// Enumerates the possible recommended measures when a service operation fails.
    /// </summary>
    enum class OpErrRecommendedAction { RetryBackoff, Reconnect, Quit };


    /// <summary>
    /// Gets the recommended action for a given error code returned by WWS API.
    /// </summary>
    /// <param name="hr">The error code.</param>
    /// <returns>The recommended action.</returns>
    static OpErrRecommendedAction GetRecommendation(HRESULT hr)
    {
        switch (hr)
        {
        case E_OUTOFMEMORY:
        case WS_E_ENDPOINT_NOT_AVAILABLE:
        case WS_E_ENDPOINT_TOO_BUSY:
        case WS_E_OPERATION_TIMED_OUT:
        case WS_E_QUOTA_EXCEEDED:
            return OpErrRecommendedAction::RetryBackoff; // retry with exponential back-off

        case WS_E_ENDPOINT_DISCONNECTED:
        case WS_E_ENDPOINT_NOT_FOUND:
        case WS_E_ENDPOINT_UNREACHABLE:
            return OpErrRecommendedAction::Reconnect; // reconnect and try again

        default:
            return OpErrRecommendedAction::Quit; // not fit for another attempt, just quit
        }
    }


    // Prepare the service proxy properties from the provided configuration
    static WS_PROXY_PROPERTY *PrepareSvcProxyProperties(
        const SvcProxyConfig &config,
        WSHeap &heap,
        ULONG &propCount)
    {
        propCount = 2;
        auto properties = heap.Alloc<WS_PROXY_PROPERTY>(propCount);

        size_t idxProp(0);
        properties[idxProp++] = WS_PROXY_PROPERTY{
            WS_PROXY_PROPERTY_CALL_TIMEOUT,
            WS_HEAP_NEW(heap, decltype(config.timeoutCall), (config.timeoutCall)),
            sizeof config.timeoutCall
        };

        properties[idxProp++] = WS_PROXY_PROPERTY{
            WS_PROXY_PROPERTY_MAX_CLOSE_TIMEOUT,
            WS_HEAP_NEW(heap, decltype(config.timeoutClose), (config.timeoutClose)),
            sizeof config.timeoutClose
        };

        // Check the counting of properties
        _ASSERTE(idxProp == propCount);

        return properties;
    }


    // Prepare the channel properties for the service proxy from the provided configuration
    static WS_CHANNEL_PROPERTIES PrepareChannelProperties(const SvcProxyConfig &config, WSHeap &heap)
    {
        WS_CHANNEL_PROPERTIES channel;
        channel.propertyCount = 4;
        channel.properties = heap.Alloc<WS_CHANNEL_PROPERTY>(channel.propertyCount);

        size_t idxProp = 0;
        channel.properties[idxProp++] =
            WS_CHANNEL_PROPERTY {
                WS_CHANNEL_PROPERTY_RESOLVE_TIMEOUT,
                WS_HEAP_NEW(heap, decltype(config.timeoutDnsResolve), (config.timeoutDnsResolve)),
                sizeof config.timeoutDnsResolve
            };

        channel.properties[idxProp++] =
            WS_CHANNEL_PROPERTY {
                WS_CHANNEL_PROPERTY_CONNECT_TIMEOUT,
                WS_HEAP_NEW(heap, decltype(config.timeoutConnect), (config.timeoutConnect)),
                sizeof config.timeoutConnect
            };

        channel.properties[idxProp++] =
            WS_CHANNEL_PROPERTY {
                WS_CHANNEL_PROPERTY_SEND_TIMEOUT,
                WS_HEAP_NEW(heap, decltype(config.timeoutSend), (config.timeoutSend)),
                sizeof config.timeoutSend
            };

        channel.properties[idxProp++] =
            WS_CHANNEL_PROPERTY {
                WS_CHANNEL_PROPERTY_RECEIVE_TIMEOUT,
                WS_HEAP_NEW(heap, decltype(config.timeoutReceive), (config.timeoutReceive)),
                sizeof config.timeoutReceive
            };

        // Check the count of channel properties:
        _ASSERTE(idxProp == channel.propertyCount);

        return channel;
    }


    /// <summary>
    /// Initializes a new instance of the <see cref="WebServiceProxyImpl"/> class.
    /// </summary>
    /// <param name="svcEndpointAddress">The address for the service endpoint.</param>
    /// <param name="callback">
    /// The callback for creating the proxy, which wraps implementation generated by wsutil.exe.
    /// </param>
    WebServiceProxyImpl::WebServiceProxyImpl(
        const string &svcEndpointAddress,
        const SvcProxyConfig &config,
        CallbackWrapperCreateServiceProxy callback)
    try : 
        m_wsSvcProxyHandle(nullptr),
        m_isOnHold(false),
        m_heap(config.reservedMemory),
        m_proxyStateMutex()
    {
        CALL_STACK_TRACE;

        std::wstring_convert<std::codecvt_utf8<wchar_t>> transcoder;
        m_svcEndptAddr = transcoder.from_bytes(svcEndpointAddress);

        // From the provided configuration, fill the channel properties...
        auto channelProperties = PrepareChannelProperties(config, m_heap);

        // ...and the proxy properties
        ULONG proxyPropCount;
        auto proxyProperties = PrepareSvcProxyProperties(config, m_heap, proxyPropCount);

        WSError err;
        auto hr = callback(
            channelProperties,
            proxyProperties,
            proxyPropCount,
            &m_wsSvcProxyHandle,
            m_heap,
            err
        );
        err.RaiseExceptionApiError(hr, "WsCreateServiceProxyFromTemplate",
            "Failed to create proxy for web service");
    }
    catch (IAppException &)
    {
        throw; // just forward exceptions regarding errors known to have been previously handled
    }
    catch (std::system_error &ex)
    {
        CALL_STACK_TRACE;
        std::ostringstream oss;
        oss << "System failure when creating web service proxy: "
            << StdLibExt::GetDetailsFromSystemError(ex);

        throw AppException<std::runtime_error>(oss.str());
    }
    catch (std::exception &ex)
    {
        CALL_STACK_TRACE;
        std::ostringstream oss;
        oss << "Generic failure when creating web service proxy: " << ex.what();
        throw AppException<std::runtime_error>(oss.str());
    }


    /// <summary>
    /// Initializes a new instance of the <see cref="WebServiceProxy"/> class.
    /// </summary>
    /// <param name="svcEndpointAddress">The address for the service endpoint.</param>
    /// <param name="callback">
    /// The callback for creating the proxy, which wraps implementation generated by wsutil.exe.
    /// </param>
    WebServiceProxy::WebServiceProxy(
        const string &svcEndpointAddress,
        const SvcProxyConfig &config,
        CallbackWrapperCreateServiceProxy callback
    )
    :   m_pimpl(nullptr)
    {
        CALL_STACK_TRACE;

        try
        {
            m_pimpl = new WebServiceProxyImpl(svcEndpointAddress, config, callback);
        }
        catch (std::bad_alloc &)
        {
            throw AppException<std::runtime_error>(
                "Failed to allocate memory for instantiation of web service proxy"
            );
        }
    }


    /// <summary>
    /// Initializes a new instance of the <see cref="WebServiceProxyImpl" /> class.
    /// This constructor applies to the specific case of HTTP binding with SSL
    /// and this proxy using a client certificate.
    /// </summary>
    /// <param name="svcEndpointAddress">The address for the service endpoint.</param>
    /// <param name="config">The configuration values for the proxy.</param>
    /// <param name="certInfo">The information about the certificate to use in the client side.</param>
    /// <param name="callback">The callback for creating the proxy, which wraps implementation
    /// generated by wsutil.exe.</param>
    WebServiceProxyImpl::WebServiceProxyImpl(
        const string &svcEndpointAddress,
        const SvcProxyConfig &config,
        const SvcProxyCertInfo &certInfo,
        CallbackCreateServiceProxyImpl<WS_HTTP_SSL_BINDING_TEMPLATE> callback)
    try :
        m_wsSvcProxyHandle(nullptr),
        m_heap(config.reservedMemory),
        m_proxyStateMutex()
    {
        CALL_STACK_TRACE;

        std::wstring_convert<std::codecvt_utf8<wchar_t>> transcoder;
        m_svcEndptAddr = transcoder.from_bytes(svcEndpointAddress);

        // Create the certificate credential from the thumbprint:
        WS_THUMBPRINT_CERT_CREDENTIAL certCredential{};
        certCredential.credential = WS_CERT_CREDENTIAL{ WS_THUMBPRINT_CERT_CREDENTIAL_TYPE };
        certCredential.storeLocation = certInfo.storeLocation;
        certCredential.storeName = ToWsString(certInfo.storeName, m_heap);
        certCredential.thumbprint = ToWsString(certInfo.thumbprint, m_heap);

        WS_HTTP_SSL_BINDING_TEMPLATE bindingTemplate{};
        bindingTemplate.channelProperties = PrepareChannelProperties(config, m_heap);
        bindingTemplate.sslTransportSecurityBinding.localCertCredential = &certCredential.credential;

        // Set some properties for the service proxy:
        ULONG proxyPropCount;
        auto proxyProperties = PrepareSvcProxyProperties(config, m_heap, proxyPropCount);

        // Finally create the proxy using the implementation generated by wsutil.exe:
        WSError err;
        auto hr = callback(
            &bindingTemplate,
            proxyProperties,
            proxyPropCount,
            &m_wsSvcProxyHandle,
            err.GetHandle()
        );
        err.RaiseExceptionApiError(hr, "WsCreateServiceProxyFromTemplate",
            "Failed to create proxy for web service with client side SSL certificate");
    }
    catch (IAppException &)
    {
        throw; // just forward exceptions regarding errors known to have been previously handled
    }
    catch (std::system_error &ex)
    {
        CALL_STACK_TRACE;
        std::ostringstream oss;
        oss << "System failure when creating web service proxy: "
            << StdLibExt::GetDetailsFromSystemError(ex);

        throw AppException<std::runtime_error>(oss.str());
    }
    catch (std::exception &ex)
    {
        CALL_STACK_TRACE;
        std::ostringstream oss;
        oss << "Generic failure when creating web service proxy: " << ex.what();
        throw AppException<std::runtime_error>(oss.str());
    }


    /// <summary>
    /// Initializes a new instance of the <see cref="WebServiceProxyImpl" /> class.
    /// This constructor applies to the specific case of HTTP binding with SSL
    /// and this proxy using a client certificate.
    /// </summary>
    /// <param name="svcEndpointAddress">The address for the service endpoint.</param>
    /// <param name="config">The configuration values for the proxy.</param>
    /// <param name="certInfo">The information about the certificate to use in the client side.</param>
    /// <param name="callback">The callback for creating the proxy, which wraps implementation
    /// generated by wsutil.exe.</param>
    WebServiceProxy::WebServiceProxy(
        const string &svcEndpointAddress,
        const SvcProxyConfig &config,
        const SvcProxyCertInfo &certInfo,
        CallbackCreateServiceProxyImpl<WS_HTTP_SSL_BINDING_TEMPLATE> callback
    )
    :   m_pimpl(nullptr)
    {
        CALL_STACK_TRACE;

        try
        {
            m_pimpl = new WebServiceProxyImpl(svcEndpointAddress, config, certInfo, callback);
        }
        catch (std::bad_alloc &)
        {
            throw AppException<std::runtime_error>(
                "Failed to allocate memory for instantiation of web service proxy"
            );
        }
    }


    /// <summary>
    /// Initializes a new instance of the <see cref="WebServiceProxyImpl" /> class.
    /// This constructor applies to the specific case of HTTP binding with header
    /// authentication, SSL and this proxy using a client certificate.
    /// </summary>
    /// <param name="svcEndpointAddress">The address for the service endpoint.</param>
    /// <param name="config">The configuration values for the proxy.</param>
    /// <param name="certInfo">The information about the certificate to use in the client side.</param>
    /// <param name="callback">The callback for creating the proxy, which wraps implementation
    /// generated by wsutil.exe.</param>
    /// <remarks>
    /// This proxy will use Windows Integrated Authentication credential based on the current Windows
    /// identity, and the security package specified in the implementation generated by wsutil.exe.
    /// </remarks>
    WebServiceProxyImpl::WebServiceProxyImpl(
        const string &svcEndpointAddress,
        const SvcProxyConfig &config,
        const SvcProxyCertInfo &certInfo,
        CallbackCreateServiceProxyImpl<WS_HTTP_SSL_HEADER_AUTH_BINDING_TEMPLATE> callback)
    try :
        m_wsSvcProxyHandle(nullptr),
        m_heap(config.reservedMemory),
        m_proxyStateMutex()
    {
        CALL_STACK_TRACE;

        std::wstring_convert<std::codecvt_utf8<wchar_t>> transcoder;
        m_svcEndptAddr = transcoder.from_bytes(svcEndpointAddress);

        // Create the certificate credential from the thumbprint:
        WS_THUMBPRINT_CERT_CREDENTIAL certCredential{};
        certCredential.credential = WS_CERT_CREDENTIAL{ WS_THUMBPRINT_CERT_CREDENTIAL_TYPE };
        certCredential.storeLocation = certInfo.storeLocation;
        certCredential.storeName = ToWsString(certInfo.storeName, m_heap);
        certCredential.thumbprint = ToWsString(certInfo.thumbprint, m_heap);

        WS_DEFAULT_WINDOWS_INTEGRATED_AUTH_CREDENTIAL authCredential {
            WS_WINDOWS_INTEGRATED_AUTH_CREDENTIAL{ WS_DEFAULT_WINDOWS_INTEGRATED_AUTH_CREDENTIAL_TYPE }
        };

        WS_HTTP_SSL_HEADER_AUTH_BINDING_TEMPLATE bindingTemplate{};
        bindingTemplate.channelProperties = PrepareChannelProperties(config, m_heap);
        bindingTemplate.sslTransportSecurityBinding.localCertCredential = &certCredential.credential;
        bindingTemplate.httpHeaderAuthSecurityBinding.clientCredential = &authCredential.credential;

        // Set some properties for the service proxy:
        ULONG proxyPropCount;
        auto proxyProperties = PrepareSvcProxyProperties(config, m_heap, proxyPropCount);

        // Finally create the proxy using the implementation generated by wsutil.exe:
        WSError err;
        auto hr = callback(
            &bindingTemplate,
            proxyProperties,
            proxyPropCount,
            &m_wsSvcProxyHandle,
            err.GetHandle()
        );
        err.RaiseExceptionApiError(hr, "WsCreateServiceProxyFromTemplate",
            "Failed to create proxy for web service with HTTP "
            "header authentication and client side SSL certificate");
    }
    catch (IAppException &ex)
    {
        CALL_STACK_TRACE;
        throw AppException<std::runtime_error>(
            "Failed to instantiate object wrapper for web service proxy", ex
        );
    }
    catch (std::system_error &ex)
    {
        CALL_STACK_TRACE;
        std::ostringstream oss;
        oss << "System failure when creating web service proxy: "
            << StdLibExt::GetDetailsFromSystemError(ex);

        throw AppException<std::runtime_error>(oss.str());
    }
    catch (std::exception &ex)
    {
        CALL_STACK_TRACE;
        std::ostringstream oss;
        oss << "Generic failure when creating web service proxy: " << ex.what();
        throw AppException<std::runtime_error>(oss.str());
    }


    /// <summary>
    /// Initializes a new instance of the <see cref="WebServiceProxy" /> class.
    /// This constructor applies to the specific case of HTTP binding with header
    /// authentication, SSL and this proxy using a client certificate.
    /// </summary>
    /// <param name="svcEndpointAddress">The address for the service endpoint.</param>
    /// <param name="config">The configuration values for the proxy.</param>
    /// <param name="certInfo">The information about the certificate to use in the client side.</param>
    /// <param name="callback">The callback for creating the proxy, which wraps implementation
    /// generated by wsutil.exe.</param>
    /// <remarks>
    /// This proxy will use Windows Integrated Authentication credential based on the current Windows
    /// identity, and the security package specified in the implementation generated by wsutil.exe.
    /// </remarks>
    WebServiceProxy::WebServiceProxy(
        const string &svcEndpointAddress,
        const SvcProxyConfig &config,
        const SvcProxyCertInfo &certInfo,
        CallbackCreateServiceProxyImpl<WS_HTTP_SSL_HEADER_AUTH_BINDING_TEMPLATE> callback
    )
    :   m_pimpl(nullptr)
    {
        CALL_STACK_TRACE;

        try
        {
            m_pimpl = new WebServiceProxyImpl(svcEndpointAddress, config, certInfo, callback);
        }
        catch (std::bad_alloc &)
        {
            throw AppException<std::runtime_error>(
                "Failed to allocate memory for instantiation of web service proxy"
            );
        }
    }


    /// <summary>
    /// Finalizes an instance of the <see cref="WebServiceProxyImpl"/> class.
    /// </summary>
    WebServiceProxyImpl::~WebServiceProxyImpl()
    {
        if (m_wsSvcProxyHandle == nullptr)
            return;

        CALL_STACK_TRACE;

        try
        {
            Abort();
            WsFreeServiceProxy(m_wsSvcProxyHandle);

            /* Only after the service proxy is done, which means no more asynchronous
            callbacks will be called, the promises can be deleted: */
            for (auto entry : m_promises)
                delete entry;
        }
        catch (IAppException &ex)
        {
            Logger::Write(ex, Logger::Priority::PRIO_CRITICAL);
        }
        catch (std::exception &ex)
        {
            std::ostringstream oss;
            oss << "Generic failure when terminating proxy for web service: " << ex.what();
            Logger::Write(oss.str(), Logger::Priority::PRIO_CRITICAL);
        }
    }


    /// <summary>
    /// Finalizes an instance of the <see cref="WebServiceProxy"/> class.
    /// </summary>
    WebServiceProxy::~WebServiceProxy()
    {
        delete m_pimpl;
    }


    /// <summary>
    /// Gets the handle for this web service proxy.
    /// </summary>
    /// <returns>A handle for this web service proxy.</returns>
    WS_SERVICE_PROXY * WebServiceProxy::GetHandle() const
    {
        return m_pimpl->GetHandle();
    }


    /// <summary>
    /// Opens the service proxy before it can start sending requests.
    /// If the proxy was already opened, nothing is done. In case of
    /// failure, retry logic is applied before throwing an exception.
    /// </summary>
    /// <returns>Whether the service proxy was closed or at fault, and the call had to open it.</returns>
    bool WebServiceProxyImpl::Open()
    {
        CALL_STACK_TRACE;

        try
        {
            _ASSERTE(m_wsSvcProxyHandle != nullptr); // attempt to use uninitialized proxy

            /* First acquire lock before changing state of service proxy. This lock guarantees
            that after a failure event, the proxy will not be left in a transitive state such
            as WS_SERVICE_PROXY_STATE_OPENING or WS_SERVICE_PROXY_STATE_CLOSING. */
            std::lock_guard<std::mutex> lock(m_proxyStateMutex);

            WSError err;
            HRESULT hr;

            // Ask for the proxy current state:
            WS_SERVICE_PROXY_STATE state;
            hr = WsGetServiceProxyProperty(
                m_wsSvcProxyHandle,
                WS_PROXY_PROPERTY_STATE,
                &state,
                sizeof state,
                err.GetHandle()
            );
            err.RaiseExceptionApiError(hr, "WsGetServiceProxyProperty",
                "Failed to get state of proxy for web service");

            if (state == WS_SERVICE_PROXY_STATE_OPEN)
                return false;

            WS_ENDPOINT_ADDRESS endpointAddress = { 0 };
            endpointAddress.url.chars = const_cast<wchar_t *> (m_svcEndptAddr.data());
            endpointAddress.url.length = m_svcEndptAddr.length();

            static const auto maxRetries = AppConfig::GetSettings().framework.wws.proxyConnMaxRetries;

            int count(0);

            clock_t startTime = clock();

            while (true)
            {
                hr = WsOpenServiceProxy(
                    m_wsSvcProxyHandle,
                    &endpointAddress,
                    nullptr,
                    err.GetHandle()
                );

                if (hr == S_OK)
                    return true;

                OpErrRecommendedAction ra = GetRecommendation(hr);

                if (count == maxRetries || ra == OpErrRecommendedAction::Quit)
                    break;

                if (count == 0)
                {
                    std::array<char, 512> message;
                    utils::SerializeTo(message,
                        "Failed to open proxy for web service at '", m_svcEndptAddr,
                        "': will re-attempt up to ", maxRetries, " time(s)");

                    Logger::Write(message.data(), Logger::PRIO_WARNING);
                }

                static const auto retryTimeSlotMs = AppConfig::GetSettings().framework.wws.proxyRetryTimeSlotMs;
                static const auto retrySleepTimeSecs = AppConfig::GetSettings().framework.wws.proxyRetrySleepSecs;

                uint64_t intervalMs;

                if (ra == OpErrRecommendedAction::Reconnect)
                {
                    // connectivity related problems cause attempts with constant interval
                    intervalMs = 1000UL * retrySleepTimeSecs;
                }
                else
                {
                    // resource related problems cause attempts with intervals in exponential back-off
                    intervalMs = static_cast<uint32_t> (rand() * (pow(2, count) - 1) / RAND_MAX) * retryTimeSlotMs;
                }

                std::this_thread::sleep_for(
                    std::chrono::milliseconds(intervalMs)
                );

                ++count;

            }// end of loop
            
            err.RaiseExceptionApiError(hr, "WsOpenServiceProxy",
                "Failed to open proxy for web service");

            if (count > 0)
            {
                auto elapsed = static_cast<double> (clock() - startTime) / CLOCKS_PER_SEC;

                std::array<char, 512> message;
                utils::SerializeTo(message,
                    "Proxy for web service at '", m_svcEndptAddr,
                    "' successfully opened after ", count, " attempt(s) within ",
                    format(elapsed).precision(3), " second(s)");

                Logger::Write(message.data(), Logger::PRIO_NOTICE);
            }

            return true;
        }
        catch (IAppException &)
        {
            throw; // just forward exceptions regarding errors known to have been previously handled
        }
        catch (std::system_error &ex)
        {
            std::ostringstream oss;
            oss << "System failure when opening proxy for web service: "
                << StdLibExt::GetDetailsFromSystemError(ex);

            throw AppException<std::runtime_error>(oss.str());
        }
        catch (std::exception &ex)
        {
            std::ostringstream oss;
            oss << "Generic failure when opening proxy web service: " << ex.what();
            throw AppException<std::runtime_error>(oss.str());
        }
    }


    /// <summary>
    /// Opens the service proxy before it can start sending requests.
    /// If the proxy was already opened, nothing is done. In case of
    /// failure, retry logic is applied before throwing an exception.
    /// </summary>
    /// <returns>Whether the service proxy was closed or at fault, and the call had to open it.</returns>
    bool WebServiceProxy::Open()
    {
        return m_pimpl->Open();
    }


    // Helps closing the proxy
    static void HelpCloseServiceProxy(WS_SERVICE_PROXY *wsSvcProxyHandle, WSError &err)
    {
        auto hr = WsCloseServiceProxy(wsSvcProxyHandle, nullptr, err.GetHandle());

        /* As an operation, to close a service proxy can only fail when it is invalid,
        all the other error do not prevent the operation from succeeding, hence such
        types of failure should be logged instead of having an exception thrown. */
        if (hr == WS_E_INVALID_OPERATION)
            err.RaiseExceptionApiError(hr, "WsCloseServiceProxy", "Failed to close proxy for web service");
        else
            err.LogApiError(hr, "WsCloseServiceProxy", "Proxy for web service has been closed, but with an error");
    }


    /// <summary>
    /// Closes down communications in the service proxy (but wait sessions to disconnect)
    /// and make it ready for possible restart.
    /// </summary>
    /// <returns>Whether the service proxy was actively running and the call managed to stop it.</returns>
    bool WebServiceProxyImpl::Close()
    {
        CALL_STACK_TRACE;

        try
        {
            _ASSERTE(m_wsSvcProxyHandle != nullptr); // attempt to use uninitialized proxy

            /* First acquire lock before changing state of service proxy. This lock guarantees
            that after a failure event, the proxy will not be left in a transitive state such
            as WS_SERVICE_PROXY_STATE_OPENING or WS_SERVICE_PROXY_STATE_CLOSING. */
            std::lock_guard<std::mutex> lock(m_proxyStateMutex);

            bool wasRunning(false);
            WSError err;

            // Ask for the proxy current state:
            WS_SERVICE_PROXY_STATE state;
            auto hr = WsGetServiceProxyProperty(
                m_wsSvcProxyHandle,
                WS_PROXY_PROPERTY_STATE,
                &state,
                sizeof state,
                err.GetHandle()
            );
            err.RaiseExceptionApiError(hr, "WsGetServiceProxyProperty", 
                "Failed to get state of proxy for web service");

            // only in open or faulted state can a proxy be closed
            if (state == WS_SERVICE_PROXY_STATE_OPEN
                || state == WS_SERVICE_PROXY_STATE_FAULTED)
            {
                HelpCloseServiceProxy(m_wsSvcProxyHandle, err);
                wasRunning = true;
            }

            // Reset the service proxy:
            hr = WsResetServiceProxy(m_wsSvcProxyHandle, err.GetHandle());
            err.RaiseExceptionApiError(hr, "WsResetServiceProxy",
                "Failed to reset proxy for web service");

            return wasRunning;
        }
        catch (IAppException &)
        {
            throw; // just forward exceptions regarding errors known to have been previously handled
        }
        catch (std::system_error &ex)
        {
            std::ostringstream oss;
            oss << "System failure when opening proxy for web service: "
                << StdLibExt::GetDetailsFromSystemError(ex);

            throw AppException<std::runtime_error>(oss.str());
        }
        catch (std::exception &ex)
        {
            std::ostringstream oss;
            oss << "Generic failure when opening proxy for web service: " << ex.what();
            throw AppException<std::runtime_error>(oss.str());
        }
    }


    /// <summary>
    /// Closes down communications in the service proxy (but wait sessions to disconnect)
    /// and make it ready for possible restart.
    /// </summary>
    /// <returns>Whether the service proxy was actively running and the call managed to stop it.</returns>
    bool WebServiceProxy::Close()
    {
        return m_pimpl->Close();
    }


    /// <summary>
    /// Closes down communication in the service proxy (immediately, drops connections)
    /// and make it ready for possible restart.
    /// </summary>
    /// <returns>Whether the service proxy was actively running and the call managed to abort it.</returns>
    bool WebServiceProxyImpl::Abort()
    {
        CALL_STACK_TRACE;

        try
        {
            _ASSERTE(m_wsSvcProxyHandle != nullptr); // attempt to use uninitialized proxy

            /* First acquire lock before changing state of service proxy. This lock guarantees
            that after a failure event, the proxy will not be left in a transitive state such
            as WS_SERVICE_PROXY_STATE_OPENING or WS_SERVICE_PROXY_STATE_CLOSING. */
            std::lock_guard<std::mutex> lock(m_proxyStateMutex);

            bool wasRunning(false);
            WSError err;

            // Ask for the proxy current state:
            WS_SERVICE_PROXY_STATE state;
            auto hr = WsGetServiceProxyProperty(
                m_wsSvcProxyHandle,
                WS_PROXY_PROPERTY_STATE,
                &state,
                sizeof state,
                err.GetHandle()
            );
            err.RaiseExceptionApiError(hr, "WsGetServiceProxyProperty",
                "Failed to get state of proxy for web service");

            switch (state)
            {
            // only an open proxy should be aborted
            case WS_SERVICE_PROXY_STATE_OPEN:

                // Abort service proxy. It will lead it to falted state...
                hr = WsAbortServiceProxy(m_wsSvcProxyHandle, err.GetHandle());
                err.RaiseExceptionApiError(hr, "WsAbortServiceProxy",
                    "Failed to abort proxy for web service");

            /* FALLTHROUGH: The proxy is left in faulted state due to the current
            abort operation, but it can also happen due to an unknown previous failure.
            In this case, close the proxy here in this separate case: */
            case WS_SERVICE_PROXY_STATE_FAULTED:

                HelpCloseServiceProxy(m_wsSvcProxyHandle, err);
                wasRunning = true;
                break;

            default:
                break;
            }

            // Reset the service host:
            hr = WsResetServiceProxy(m_wsSvcProxyHandle, err.GetHandle());
            err.RaiseExceptionApiError(hr, "WsResetServiceProxy",
                "Failed to reset proxy for web service");

            return wasRunning;
        }
        catch (IAppException &)
        {
            throw; // just forward exceptions regarding errors known to have been previously handled
        }
        catch (std::system_error &ex)
        {
            std::ostringstream oss;
            oss << "System failure when aborting proxy for web service: "
                << StdLibExt::GetDetailsFromSystemError(ex);

            throw AppException<std::runtime_error>(oss.str());
        }
        catch (std::exception &ex)
        {
            std::ostringstream oss;
            oss << "Generic failure when aborting proxy for web service: " << ex.what();
            throw AppException<std::runtime_error>(oss.str());
        }
    }


    /// <summary>
    /// Closes down communication in the service proxy (immediately, drops connections)
    /// and make it ready for possible restart.
    /// </summary>
    /// <returns>Whether the service proxy was actively running and the call managed to abort it.</returns>
    bool WebServiceProxy::Abort()
    {
        return m_pimpl->Abort();
    }


    /// <summary>
    /// Wraps a call to the proxy implementation (generated by wsutil.exe) of a service
    /// operation, taking responsibility for memory allocation and error handling.
    /// </summary>
    /// <param name="operLabel">A label for the service operation.</param>
    /// <param name="operHeapSize">Size of the heap dedicated for the operation and its error handling.</param>
    /// <param name="operWrap">A wrap around the proxy implementation for the service operation.</param>
    void WebServiceProxyImpl::Call(const char *operLabel,
                                   size_t operHeapSize,
                                   const WsCallWrap &operWrap)
    {
        CALL_STACK_TRACE;

        std::array<char, 256> errMsgBuffer;

        static const auto maxRetries = AppConfig::GetSettings().framework.wws.proxyCallMaxRetries;

        WSHeap heap(operHeapSize);
        WSError err;
        HRESULT hr;
        int count;

        clock_t startTime = clock();

        for (count = 0; true; ++count)
        {
            hr = operWrap(GetHandle(), heap.GetHandle(), err.GetHandle());

            if (hr == S_OK)
            {
                m_isOnHold.store(false);
                return;
            }

            OpErrRecommendedAction ra = GetRecommendation(hr);

            if (count == maxRetries || ra == OpErrRecommendedAction::Quit)
                break;

            if (!m_isOnHold.exchange(true))
            {
                utils::SerializeTo(errMsgBuffer,
                    "Failed call to operation in web service at '", m_svcEndptAddr,
                    "', but the proxy will retry up to ", maxRetries, " time(s)");

                Logger::Write(errMsgBuffer.data(), Logger::PRIO_WARNING);
            }

            static const auto retryTimeSlotMs = AppConfig::GetSettings().framework.wws.proxyRetryTimeSlotMs;
            static const auto retrySleepTimeSecs = AppConfig::GetSettings().framework.wws.proxyRetrySleepSecs;

            uint64_t intervalMs;

            if (ra == OpErrRecommendedAction::RetryBackoff)
            {
                // resource related problems cause attempts with intervals in exponential back-off
                intervalMs = static_cast<uint32_t> (rand() * (pow(2, count) - 1) / RAND_MAX) * retryTimeSlotMs;
                std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
            }
            else if (Open()/* managed to re-open proxy? */)
            {
                utils::SerializeTo(errMsgBuffer, "Proxy for web service at '", m_svcEndptAddr, "' was re-opened after loss of connection");
                Logger::Write(errMsgBuffer.data(), Logger::PRIO_NOTICE);
            }
            else // proxy was already opened?
            {
                // connectivity related problems cause attempts with constant interval:
                intervalMs = 1000UL * retrySleepTimeSecs;
                std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
            }
        }

        if (count > 0)
        {
            auto elapsed = static_cast<double> (clock() - startTime) / CLOCKS_PER_SEC;

            utils::SerializeTo(errMsgBuffer,
                operLabel, " failed after ", count, " attempt(s) in ",
                format(elapsed).precision(3), " second(s)");
        }
        else
            utils::SerializeTo(errMsgBuffer, operLabel, " returned an error");

        err.RaiseExClientNotOK(hr, errMsgBuffer.data(), heap);
    }


    /// <summary>
    /// Wraps a call to the proxy implementation (generated by wsutil.exe) of a service
    /// operation, taking responsibility for memory allocation and error handling.
    /// In cases of error due to exhaustion of resources or loss of connection, a
    /// retry logic is used before throwing an exception.
    /// </summary>
    /// <param name="operLabel">A label for the service operation.</param>
    /// <param name="operHeapSize">Size of the heap dedicated for the operation and its error handling.</param>
    /// <param name="operWrap">A wrap around the proxy implementation for the service operation.</param>
    void WebServiceProxy::Call(const char *operLabel,
                               size_t operHeapSize,
                               const WsCallWrap &operWrap)
    {
        m_pimpl->Call(operLabel, operHeapSize, operWrap);
    }


    /// <summary>
    /// Wraps a call to the proxy implementation (generated by wsutil.exe) of
    /// a service operation, starting it asynchronously and taking responsibility
    /// for memory allocation and error handling.
    /// In cases of error due to exhaustion of resources or loss of connection, a
    /// retry logic is used before throwing an exception.
    /// </summary>
    /// <param name="operLabel">A label for the service operation.</param>
    /// <param name="operHeapSize">Size of the heap dedicated for the operation and its error handling.</param>
    /// <param name="operWrap">A wrap around the proxy implementation for the service operation.</param>
    /// <returns>A <see cref="std::future&lt;void&gt;"/> object to await for operation completion.</returns>
    std::future<void> WebServiceProxy::CallAsync(const char *operLabel,
                                                 size_t operHeapSize,
                                                 const WsCallWrap &operWrap)
    {
        CALL_STACK_TRACE;

        try
        {
            return std::async(std::launch::async,
                [this](const char *operLabel, size_t operHeapSize, const WsCallWrap &operWrap)
                {
                    m_pimpl->Call(operLabel, operHeapSize, operWrap);
                },
                operLabel,
                operHeapSize,
                operWrap
            );
        }
        catch (std::system_error &ex)
        {
            std::ostringstream oss;
            oss << operLabel << " failed to be launched asynchronously"
                << StdLibExt::GetDetailsFromSystemError(ex);

            throw AppException<std::runtime_error>(oss.str());
        }
    }


}// namespace wws
}// namespace web
}// namespace _3fd
