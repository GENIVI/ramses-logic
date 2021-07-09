//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "impl/LogicNodeImpl.h"

#include "internals/SolWrapper.h"
#include "internals/SerializationMap.h"
#include "internals/DeserializationMap.h"

#include "ramses-logic/Property.h"
#include "ramses-logic/LuaScript.h"

#include <memory>
#include <functional>
#include <string_view>

namespace flatbuffers
{
    class FlatBufferBuilder;

    class FlatBufferBuilder;
    template <typename T> struct Offset;
}

namespace rlogic_serialization
{
    struct LuaScript;
}

namespace rlogic::internal
{
    class SolState;
    class PropertyImpl;
    class ErrorReporting;

    class LuaScriptImpl : public LogicNodeImpl
    {
    public:
        // TODO Violin replace Create static method with constructor by handling errors outside and only passing in valid data
        [[nodiscard]] static std::unique_ptr<LuaScriptImpl> Create(SolState& solState, std::string_view source, std::string_view scriptName, std::string_view filename, ErrorReporting& errorReporting);

        [[nodiscard]] static flatbuffers::Offset<rlogic_serialization::LuaScript> Serialize(
            const LuaScriptImpl& luaScript,
            flatbuffers::FlatBufferBuilder& builder,
            SerializationMap& serializationMap);

        [[nodiscard]] static std::unique_ptr<LuaScriptImpl> Deserialize(
            SolState& solState,
            const rlogic_serialization::LuaScript& luaScript,
            ErrorReporting& errorReporting,
            DeserializationMap& deserializationMap);

        // Move-able (noexcept); Not copy-able
        ~LuaScriptImpl() noexcept override = default;
        LuaScriptImpl(LuaScriptImpl&& other) noexcept = default;
        LuaScriptImpl& operator=(LuaScriptImpl&& other) noexcept = default;
        LuaScriptImpl(const LuaScriptImpl& other) = delete;
        LuaScriptImpl& operator=(const LuaScriptImpl& other) = delete;

        [[nodiscard]] std::string_view getFilename() const;

        std::optional<LogicNodeRuntimeError> update() override;

        void luaPrint(sol::variadic_args args);
        void overrideLuaPrint(LuaPrintFunction luaPrintFunction);

    private:
        LuaScriptImpl(SolState&                     solState,
                      sol::protected_function       solFunction,
                      std::string_view              scriptName,
                      std::string_view              filename,
                      std::string_view              source,
                      std::unique_ptr<PropertyImpl> inputs,
                      std::unique_ptr<PropertyImpl> outputs);

        std::string                             m_filename;
        std::string                             m_source;
        std::reference_wrapper<SolState>        m_state;
        sol::protected_function                 m_solFunction;
        LuaPrintFunction                        m_luaPrintFunction;

        static std::string BuildChunkName(std::string_view scriptName, std::string_view fileName);

        static void DefaultLuaPrintFunction(std::string_view scriptName, std::string_view message);
    };
}
