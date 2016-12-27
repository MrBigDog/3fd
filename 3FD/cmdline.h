#ifndef CMDLINE_H // header guard
#define CMDLINE_H

#include "exceptions.h"
#include "callstacktracer.h"
#include <cinttypes>
#include <initializer_list>
#include <stdexcept>
#include <sstream>
#include <string>
#include <memory>
#include <vector>
#include <map>

namespace _3fd
{
namespace core
{
    using std::string;

    /// <summary>
    /// Flexible parser of command lines arguments.
    /// </summary>
    class CommandLineArguments
    {
    public:

        static const uint8_t argIsOptionFlag = 0x80;
        static const uint8_t argIsValueFlag = 0x40;

        /// <summary>
        /// Enumerates the types of command line arguments an application can receive.
        /// </summary>
        enum class ArgType : uint8_t
        {
            OptionSwitch = 0 | argIsOptionFlag,            // arg is switch-type option (no accompanying value)
            OptionWithRequiredValue = 1 | argIsOptionFlag, // arg is option that requires an adjacent value
            OptionWithNonReqValue = 2 | argIsOptionFlag,   // arg is option that can have an optional adjacent value
            SingleValue = 3 | argIsValueFlag,              // arg is single value
            ValuesList = 4 | argIsValueFlag                // arg is list of values
        };

        /// <summary>
        /// Enumerates the expected placements for arguments.
        /// </summary>
        enum class ArgPlacement : int8_t
        {
            Anywhere = 0,  // argument has no required position
            MustComeLast   // argument has to be the last
        };

        static const uint8_t argValIsRangedTypeFlag = 0x80;
        static const uint8_t argValIsEnumTypeFlag = 0x40;

        /// <summary>
        /// Enumerates possible types for argument values.
        /// </summary>
        enum class ArgValType : uint8_t
        {
            None = 0, // option has no accompanying value
            String = 1,   // string value (UTF-8)
            Integer = 2,  // long signed integer value
            Float = 3,    // double precision floating point value
            EnumString = 1 | argValIsEnumTypeFlag,     // string limited to set of values
            EnumInteger = 2 | argValIsEnumTypeFlag,    // integer limited to a set of values
            RangeInteger = 2 | argValIsRangedTypeFlag, // range limited integer value
            RangeFloat = 3 | argValIsRangedTypeFlag    // range limited double precision floating point value
        };

        /// <summary>
        /// Holds the characteristics of an expected argument.
        /// </summary>
        struct ArgDeclaration
        {
            int id;                  // argument ID
            ArgType type;            // type of argument
            ArgPlacement placement;  // placement for argument
            ArgValType valueType;    // type of argument value
            char optChar;            // single character representing the option
            const char *optName;     // long name that represents the option
            const char *description; // description of argument purpose
        };

        /// <summary>
        /// Enumerates possible characters for separation of option label and its value.
        /// </summary>
        enum class ArgValSeparator : char
        {
            Space = ' ',    // expected format: --option value
            Colon = ':',    // expected format: --option:value
            EqualSign = '=' // expected format: --option=value
        };

    private:

        /// <summary>
        /// Holds an argument declaration plus extended info for configuration of values.
        /// </summary>
        struct ArgDeclExtended
        {
            ArgDeclaration common;
            void *typedExtInfo;
        };

        std::map<int, ArgDeclExtended> m_expectedArgs;
        std::map<char, int> m_argsByCharLabel;
        std::map<string, int> m_argsByNameLabel;

        void ValidateArgDescAndLabels(const ArgDeclaration &argDecl, const char *stdExMsg);

        /// <summary>
        /// Adds a previously consistency-verified argument specification into the map.
        /// </summary>
        /// <param name="argDecl">The argument declaration.</param>
        /// <param name="argValCfg">The configuration for argument values.</param>
        /// <param name="stdExMsg">The standard message for thrown exceptions.</param>
        template <typename ValType>
        void AddVerifiedArgSpecIntoMap(const ArgDeclaration &argDecl,
                                       const std::initializer_list<ValType> &argValCfg,
                                       const char *stdExMsg)
        {
            CALL_STACK_TRACE;

            try
            {
                ValidateArgDescAndLabels(argDecl, stdExMsg);

                if (m_expectedArgs.find(argDecl.code) == m_expectedArgs.end())
                {
                    std::unique_ptr<std::initializer_list<ValType>> temp;
                    temp.reset(new std::initializer_list<ValType>(std::move(argValCfg)));
                    m_expectedArgs[argDecl.code] = ArgDeclExtended{ argDecl, temp.get() };
                    temp.release();
                }
                else
                {// Collision of argument codes (ID's):
                    std::ostringstream oss;
                    oss << "Argument ID " << argDecl.id << ": collision of ID";
                    throw AppException<std::invalid_argument>(stdExMsg, oss.str());
                }
            }
            catch (std::exception &ex)
            {
                std::ostringstream oss;
                oss << "Could not add specification of command line argument: " << ex.what();
                throw AppException<std::runtime_error>(oss.str());
            }
        }

        ArgValSeparator m_argValSeparator;
        uint8_t m_largestNameLabel;
        bool m_useOptSignSlash;
        bool m_isOptCaseSensitive;
        bool m_lastPositionIsTaken;

    public:

        /// <summary>
        /// Initializes a new instance of the <see cref="CommandLineArguments"/> class.
        /// </summary>
        /// <param name="argValSeparator">The character separator to use between option label and value.</param>
        /// <param name="useOptSignSlash">if set to <c>true</c>, use option sign slash (Windows prompt style) instead of dash.</param>
        /// <param name="optCaseSensitive">if set to <c>true</c>, make case sensitive the parsing of option labels.</param>
        CommandLineArguments(ArgValSeparator argValSeparator,
                             bool useOptSignSlash,
                             bool optCaseSensitive) :
            m_argValSeparator(argValSeparator),
            m_largestNameLabel(0),
            m_useOptSignSlash(useOptSignSlash),
            m_isOptCaseSensitive(optCaseSensitive),
            m_lastPositionIsTaken(false)
        {}

        ~CommandLineArguments();

        void AddExpectedArgument(const ArgDeclaration &argDecl);

        void AddExpectedArgument(const ArgDeclaration &argDecl, std::initializer_list<long long> &&argValCfg);

        void AddExpectedArgument(const ArgDeclaration &argDecl, std::initializer_list<double> &&argValCfg);

        void AddExpectedArgument(const ArgDeclaration &argDecl, std::initializer_list<const char *> &&argValCfg);

        void PrintUsage() const;

        bool Parse(int argCount, const char *arguments[]);
    };

}// end of namespace core
}// end of namespace _3fd

#endif // end of header guard