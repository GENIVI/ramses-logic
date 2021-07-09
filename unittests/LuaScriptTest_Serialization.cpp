//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "gmock/gmock.h"

#include "impl/LuaScriptImpl.h"
#include "impl/LogicEngineImpl.h"
#include "impl/PropertyImpl.h"
#include "internals/ErrorReporting.h"

#include "generated/LuaScriptGen.h"

#include "SerializationTestUtils.h"

namespace rlogic::internal
{
    // Serialization unit tests only. For higher-order tests, check ALuaScript_LifecycleWithFiles
    class ALuaScript_Serialization : public ::testing::Test
    {
    protected:
        std::unique_ptr<LuaScriptImpl> createTestScript(std::string_view source, std::string_view scriptName = "", std::string_view filename = "")
        {
            return LuaScriptImpl::Create(m_solState, source, scriptName, filename, m_errorReporting);
        }

        std::string_view m_minimalScript = R"(
            function interface()
            end

            function run()
            end
        )";

        SolState m_solState;
        ErrorReporting m_errorReporting;
        flatbuffers::FlatBufferBuilder m_flatBufferBuilder;
        SerializationTestUtils m_testUtils{ m_flatBufferBuilder };
        SerializationMap m_serializationMap;
        DeserializationMap m_deserializationMap;
    };

    // More unit tests with inputs/outputs declared in LogicNode (base class) serialization tests
    TEST_F(ALuaScript_Serialization, RemembersBaseClassData)
    {
        // Serialize
        {
            std::unique_ptr<LuaScriptImpl> script = createTestScript(m_minimalScript, "name", "filename");
            (void)LuaScriptImpl::Serialize(*script, m_flatBufferBuilder, m_serializationMap);
        }

        // Inspect flatbuffers data
        const auto& serializedScript = *flatbuffers::GetRoot<rlogic_serialization::LuaScript>(m_flatBufferBuilder.GetBufferPointer());

        ASSERT_TRUE(serializedScript.name());
        EXPECT_EQ(serializedScript.name()->string_view(), "name");

        ASSERT_TRUE(serializedScript.rootInput());
        EXPECT_EQ(serializedScript.rootInput()->rootType(), rlogic_serialization::EPropertyRootType::Struct);
        ASSERT_TRUE(serializedScript.rootInput()->children());
        EXPECT_EQ(serializedScript.rootInput()->children()->size(), 0u);

        ASSERT_TRUE(serializedScript.rootOutput());
        EXPECT_EQ(serializedScript.rootOutput()->rootType(), rlogic_serialization::EPropertyRootType::Struct);
        ASSERT_TRUE(serializedScript.rootOutput()->children());
        EXPECT_EQ(serializedScript.rootOutput()->children()->size(), 0u);

        // Deserialize
        {
            std::unique_ptr<LuaScriptImpl> deserializedScript = LuaScriptImpl::Deserialize(m_solState, serializedScript, m_errorReporting, m_deserializationMap);

            ASSERT_TRUE(deserializedScript);
            EXPECT_TRUE(m_errorReporting.getErrors().empty());

            EXPECT_EQ(deserializedScript->getName(), "name");
        }
    }

    TEST_F(ALuaScript_Serialization, RemembersFilename)
    {
        {
            std::unique_ptr<LuaScriptImpl> script = createTestScript(m_minimalScript, "", "filename");
            (void)LuaScriptImpl::Serialize(*script, m_flatBufferBuilder, m_serializationMap);
        }

        const auto& serializedScript = *flatbuffers::GetRoot<rlogic_serialization::LuaScript>(m_flatBufferBuilder.GetBufferPointer());
        ASSERT_TRUE(serializedScript.filename());
        EXPECT_EQ(serializedScript.filename()->string_view(), "filename");

        {
            std::unique_ptr<LuaScriptImpl> deserializedScript = LuaScriptImpl::Deserialize(m_solState, serializedScript, m_errorReporting, m_deserializationMap);
            EXPECT_EQ(deserializedScript->getFilename(), "filename");
        }
    }

    TEST_F(ALuaScript_Serialization, SerializesLuaSourceCode)
    {
        {
            std::unique_ptr<LuaScriptImpl> script = createTestScript(m_minimalScript, "", "");
            (void)LuaScriptImpl::Serialize(*script, m_flatBufferBuilder, m_serializationMap);
        }

        const auto& serializedScript = *flatbuffers::GetRoot<rlogic_serialization::LuaScript>(m_flatBufferBuilder.GetBufferPointer());
        ASSERT_TRUE(serializedScript.luaSourceCode());
        EXPECT_EQ(serializedScript.luaSourceCode()->string_view(), m_minimalScript);
    }

    TEST_F(ALuaScript_Serialization, ProducesErrorWhenNameMissing)
    {
        {
            auto script = rlogic_serialization::CreateLuaScript(
                m_flatBufferBuilder,
                0 // no name
            );
            m_flatBufferBuilder.Finish(script);
        }

        const auto& serialized = *flatbuffers::GetRoot<rlogic_serialization::LuaScript>(m_flatBufferBuilder.GetBufferPointer());
        std::unique_ptr<LuaScriptImpl> deserialized = LuaScriptImpl::Deserialize(m_solState, serialized, m_errorReporting, m_deserializationMap);

        EXPECT_FALSE(deserialized);
        ASSERT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Fatal error during loading of LuaScript from serialized data: missing name!");
    }

    TEST_F(ALuaScript_Serialization, ProducesErrorWhenFilenameMissing)
    {
        {
            auto script = rlogic_serialization::CreateLuaScript(
                m_flatBufferBuilder,
                m_flatBufferBuilder.CreateString("name"),
                0 // no filename
            );
            m_flatBufferBuilder.Finish(script);
        }

        const auto& serialized = *flatbuffers::GetRoot<rlogic_serialization::LuaScript>(m_flatBufferBuilder.GetBufferPointer());
        std::unique_ptr<LuaScriptImpl> deserialized = LuaScriptImpl::Deserialize(m_solState, serialized, m_errorReporting, m_deserializationMap);

        EXPECT_FALSE(deserialized);
        ASSERT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Fatal error during loading of LuaScript from serialized data: missing filename!");
    }

    TEST_F(ALuaScript_Serialization, ProducesErrorWhenLuaSourceCodeMissing)
    {
        {
            auto script = rlogic_serialization::CreateLuaScript(
                m_flatBufferBuilder,
                m_flatBufferBuilder.CreateString("name"),
                m_flatBufferBuilder.CreateString("some/file.lua"),
                0 // no source code
            );
            m_flatBufferBuilder.Finish(script);
        }

        const auto& serialized = *flatbuffers::GetRoot<rlogic_serialization::LuaScript>(m_flatBufferBuilder.GetBufferPointer());
        std::unique_ptr<LuaScriptImpl> deserialized = LuaScriptImpl::Deserialize(m_solState, serialized, m_errorReporting, m_deserializationMap);

        EXPECT_FALSE(deserialized);
        ASSERT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Fatal error during loading of LuaScript from serialized data: missing Lua source code!");
    }

    TEST_F(ALuaScript_Serialization, ProducesErrorWhenRootInputMissing)
    {
        {
            auto script = rlogic_serialization::CreateLuaScript(
                m_flatBufferBuilder,
                m_flatBufferBuilder.CreateString("name"),
                m_flatBufferBuilder.CreateString("some/file.lua"),
                m_flatBufferBuilder.CreateString("lua source code"),
                0 // no root input
            );
            m_flatBufferBuilder.Finish(script);
        }

        const auto& serialized = *flatbuffers::GetRoot<rlogic_serialization::LuaScript>(m_flatBufferBuilder.GetBufferPointer());
        std::unique_ptr<LuaScriptImpl> deserialized = LuaScriptImpl::Deserialize(m_solState, serialized, m_errorReporting, m_deserializationMap);

        EXPECT_FALSE(deserialized);
        ASSERT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Fatal error during loading of LuaScript from serialized data: missing root input!");
    }

    TEST_F(ALuaScript_Serialization, ProducesErrorWhenRootOutputMissing)
    {
        {
            auto script = rlogic_serialization::CreateLuaScript(
                m_flatBufferBuilder,
                m_flatBufferBuilder.CreateString("name"),
                m_flatBufferBuilder.CreateString("some/file.lua"),
                m_flatBufferBuilder.CreateString("lua source code"),
                m_testUtils.serializeTestProperty("IN"),
                0 // no root output
            );
            m_flatBufferBuilder.Finish(script);
        }

        const auto& serialized = *flatbuffers::GetRoot<rlogic_serialization::LuaScript>(m_flatBufferBuilder.GetBufferPointer());
        std::unique_ptr<LuaScriptImpl> deserialized = LuaScriptImpl::Deserialize(m_solState, serialized, m_errorReporting, m_deserializationMap);

        EXPECT_FALSE(deserialized);
        ASSERT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Fatal error during loading of LuaScript from serialized data: missing root output!");
    }

    TEST_F(ALuaScript_Serialization, ProducesErrorWhenRootInputHasErrors)
    {
        {
            auto script = rlogic_serialization::CreateLuaScript(
                m_flatBufferBuilder,
                m_flatBufferBuilder.CreateString("name"),
                m_flatBufferBuilder.CreateString("some/file.lua"),
                m_flatBufferBuilder.CreateString("lua source code"),
                m_testUtils.serializeTestProperty("IN", rlogic_serialization::EPropertyRootType::Struct, true, true), // create root input with errors
                m_testUtils.serializeTestProperty("OUT")
            );
            m_flatBufferBuilder.Finish(script);
        }

        const auto& serialized = *flatbuffers::GetRoot<rlogic_serialization::LuaScript>(m_flatBufferBuilder.GetBufferPointer());
        std::unique_ptr<LuaScriptImpl> deserialized = LuaScriptImpl::Deserialize(m_solState, serialized, m_errorReporting, m_deserializationMap);

        EXPECT_FALSE(deserialized);
        ASSERT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Fatal error during loading of Property from serialized data: missing name!");
    }

    TEST_F(ALuaScript_Serialization, ProducesErrorWhenRootOutputHasErrors)
    {
        {
            auto script = rlogic_serialization::CreateLuaScript(
                m_flatBufferBuilder,
                m_flatBufferBuilder.CreateString("name"),
                m_flatBufferBuilder.CreateString("some/file.lua"),
                m_flatBufferBuilder.CreateString("lua source code"),
                m_testUtils.serializeTestProperty("IN"),
                m_testUtils.serializeTestProperty("OUT", rlogic_serialization::EPropertyRootType::Struct, true, true) // create root output with errors
            );
            m_flatBufferBuilder.Finish(script);
        }

        const auto& serialized = *flatbuffers::GetRoot<rlogic_serialization::LuaScript>(m_flatBufferBuilder.GetBufferPointer());
        std::unique_ptr<LuaScriptImpl> deserialized = LuaScriptImpl::Deserialize(m_solState, serialized, m_errorReporting, m_deserializationMap);

        EXPECT_FALSE(deserialized);
        ASSERT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Fatal error during loading of Property from serialized data: missing name!");
    }

    TEST_F(ALuaScript_Serialization, ProducesErrorWhenLuaScriptSourceHasSyntaxErrors)
    {
        {
            auto script = rlogic_serialization::CreateLuaScript(
                m_flatBufferBuilder,
                m_flatBufferBuilder.CreateString("script"),
                m_flatBufferBuilder.CreateString("some/file.lua"),
                m_flatBufferBuilder.CreateString("this.is.bad.code"),
                m_testUtils.serializeTestProperty("IN"),
                m_testUtils.serializeTestProperty("OUT") // create root output with errors
            );
            m_flatBufferBuilder.Finish(script);
        }

        const auto& serialized = *flatbuffers::GetRoot<rlogic_serialization::LuaScript>(m_flatBufferBuilder.GetBufferPointer());
        std::unique_ptr<LuaScriptImpl> deserialized = LuaScriptImpl::Deserialize(m_solState, serialized, m_errorReporting, m_deserializationMap);

        EXPECT_FALSE(deserialized);
        ASSERT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_THAT(m_errorReporting.getErrors()[0].message, ::testing::HasSubstr("Fatal error during loading of LuaScript 'script' from serialized data: failed parsing Lua source code"));
    }

    TEST_F(ALuaScript_Serialization, ProducesErrorWhenLuaScriptSourceHasRuntimeErrors)
    {
        {
            auto script = rlogic_serialization::CreateLuaScript(
                m_flatBufferBuilder,
                m_flatBufferBuilder.CreateString("script"),
                m_flatBufferBuilder.CreateString("some/file.lua"),
                m_flatBufferBuilder.CreateString("error('This is not going to compile')"),
                m_testUtils.serializeTestProperty("IN"),
                m_testUtils.serializeTestProperty("OUT") // create root output with errors
            );
            m_flatBufferBuilder.Finish(script);
        }

        const auto& serialized = *flatbuffers::GetRoot<rlogic_serialization::LuaScript>(m_flatBufferBuilder.GetBufferPointer());
        std::unique_ptr<LuaScriptImpl> deserialized = LuaScriptImpl::Deserialize(m_solState, serialized, m_errorReporting, m_deserializationMap);

        EXPECT_FALSE(deserialized);
        ASSERT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_THAT(m_errorReporting.getErrors()[0].message, ::testing::HasSubstr("Fatal error during loading of LuaScript 'script' from serialized data: failed executing script"));
        EXPECT_THAT(m_errorReporting.getErrors()[0].message, ::testing::HasSubstr("This is not going to compile"));
    }
}
