#ifndef RPC_IMPL_H // header guard
#define RPC_IMPL_H

#include "rpc.h"

namespace _3fd
{
    namespace rpc
    {
        /// <summary>
        /// Private implementation of <see cref="RpcServer"/> class.
        /// </summary>
        class RpcServerImpl
        {
        private:

            RPC_BINDING_VECTOR *m_bindings;

            const wchar_t *m_protSeqName;

            std::wstring m_serviceName;

            /// <summary>
            /// Enumerates the possible states for the RPC server.
            /// </summary>
            enum class State
            {
                Instantiated,
                Listening,
                InterfacesRegistered,
                BindingsAcquired
            };

            State m_state;

        public:

            RpcServerImpl(
                ProtocolSequence protSeq,
                const string &serviceName,
                bool useActDirSec
            );

            ~RpcServerImpl();

            bool Start(const std::vector<RPC_IF_HANDLE> &interfaces);

            bool Stop();

            bool Resume();

            bool Wait();
        };

    }// end of namespace rpc
}// end of namespace _3fd

#endif // end of header guard
