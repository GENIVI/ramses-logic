//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include <gmock/gmock.h>

#include "LogicEngineTest_Base.h"

#include "RamsesTestUtils.h"

#include "ramses-logic/RamsesAppearanceBinding.h"
#include "ramses-logic/RamsesNodeBinding.h"
#include "ramses-logic/RamsesCameraBinding.h"

#include "ramses-logic/Property.h"

#include "ramses-client-api/EffectDescription.h"
#include "ramses-client-api/Effect.h"
#include "ramses-client-api/UniformInput.h"
#include "ramses-client-api/Appearance.h"
#include "ramses-client-api/PerspectiveCamera.h"
#include "ramses-client-api/Node.h"

#include "impl/LogicNodeImpl.h"
#include "impl/LogicEngineImpl.h"

#include "fmt/format.h"

namespace rlogic
{
    class ALogicEngine_Update : public ALogicEngine
    {
    };

    TEST_F(ALogicEngine_Update, UpdatesRamsesNodeBindingValuesOnUpdate)
    {
        auto        luaScript = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
                IN.param = BOOL
                OUT.param = BOOL
            end
            function run()
                OUT.param = IN.param
            end
        )", "Script");

        auto        ramsesNodeBinding = m_logicEngine.createRamsesNodeBinding(*m_node, "NodeBinding");

        auto scriptInput  = luaScript->getInputs()->getChild("param");
        auto scriptOutput = luaScript->getOutputs()->getChild("param");
        auto nodeInput    = ramsesNodeBinding->getInputs()->getChild("visibility");
        scriptInput->set(true);
        nodeInput->set(false);

        m_logicEngine.link(*scriptOutput, *nodeInput);

        EXPECT_FALSE(*nodeInput->get<bool>());
        EXPECT_TRUE(m_logicEngine.update());
        EXPECT_TRUE(*nodeInput->get<bool>());
    }

    TEST_F(ALogicEngine_Update, UpdatesRamsesCameraBindingValuesOnUpdate)
    {
        RamsesTestSetup testSetup;
        ramses::Scene* scene = testSetup.createScene();

        auto        luaScript = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
                IN.param = INT
                OUT.param = INT
            end
            function run()
                OUT.param = IN.param
            end
        )", "Script");

        auto ramsesCameraBinding = m_logicEngine.createRamsesCameraBinding(*scene->createPerspectiveCamera(), "CameraBinding");

        auto scriptInput = luaScript->getInputs()->getChild("param");
        auto scriptOutput = luaScript->getOutputs()->getChild("param");
        auto cameraInput = ramsesCameraBinding->getInputs()->getChild("viewPortProperties")->getChild("viewPortOffsetX");
        scriptInput->set(34);
        cameraInput->set(21);

        m_logicEngine.link(*scriptOutput, *cameraInput);

        EXPECT_EQ(21, *cameraInput->get<int32_t>());
        EXPECT_TRUE(m_logicEngine.update());
        EXPECT_EQ(34, *cameraInput->get<int32_t>());
    }

    TEST_F(ALogicEngine_Update, UpdatesARamsesAppearanceBinding)
    {
        RamsesTestSetup testSetup;
        ramses::Scene* scene = testSetup.createScene();

        ramses::EffectDescription effectDesc;
        effectDesc.setFragmentShader(R"(
        #version 100

        void main(void)
        {
            gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
        })");

        effectDesc.setVertexShader(R"(
        #version 100

        uniform highp float floatUniform;
        attribute vec3 a_position;

        void main()
        {
            gl_Position = floatUniform * vec4(a_position, 1.0);
        })");

        const ramses::Effect* effect = scene->createEffect(effectDesc);
        ramses::Appearance* appearance = scene->createAppearance(*effect);

        auto appearanceBinding = m_logicEngine.createRamsesAppearanceBinding(*appearance, "appearancebinding");

        auto floatUniform = appearanceBinding->getInputs()->getChild("floatUniform");
        floatUniform->set(47.11f);

        m_logicEngine.update();

        ramses::UniformInput floatInput;
        effect->findUniformInput("floatUniform", floatInput);
        float result = 0.0f;
        appearance->getInputValueFloat(floatInput, result);
        EXPECT_FLOAT_EQ(47.11f, result);
    }

    TEST_F(ALogicEngine_Update, ProducesErrorIfLinkedScriptHasRuntimeError)
    {
        auto        scriptSource = R"(
            function interface()
                IN.param = BOOL
                OUT.param = BOOL
            end
            function run()
                error("This will die")
            end
        )";

        auto sourceScript = m_logicEngine.createLuaScriptFromSource(scriptSource, "Source");
        auto targetScript = m_logicEngine.createLuaScriptFromSource(scriptSource, "Target");

        auto output = sourceScript->getOutputs()->getChild("param");
        auto input  = targetScript->getInputs()->getChild("param");
        input->set(true);

        m_logicEngine.link(*output, *input);

        EXPECT_FALSE(m_logicEngine.update());
        auto errors = m_logicEngine.getErrors();
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr("This will die"));
        EXPECT_THAT(m_logicEngine.getErrors()[0].node, sourceScript);
    }

    TEST_F(ALogicEngine_Update, PropagatesValuesOnlyToConnectedLogicNodes)
    {
        auto        scriptSource = R"(
            function interface()
                IN.inFloat = FLOAT
                IN.inVec3  = VEC3F
                IN.inInt   = INT
                OUT.outFloat = FLOAT
                OUT.outVec3  = VEC3F
                OUT.outInt   = INT
            end
            function run()
                OUT.outFloat = IN.inFloat
                OUT.outVec3 = IN.inVec3
                OUT.outInt = IN.inInt
            end
        )";

        const std::string_view vertexShaderSource = R"(
            #version 300 es

            uniform highp float floatUniform;

            void main()
            {
                gl_Position = floatUniform * vec4(1.0);
            })";

        const std::string_view fragmentShaderSource = R"(
            #version 300 es

            out lowp vec4 color;
            void main(void)
            {
                color = vec4(1.0, 0.0, 0.0, 1.0);
            })";

        auto script            = m_logicEngine.createLuaScriptFromSource(scriptSource, "Script");
        auto nodeBinding       = m_logicEngine.createRamsesNodeBinding(*m_node, "NodeBinding");
        auto appearanceBinding = m_logicEngine.createRamsesAppearanceBinding(*m_appearance, "AppearanceBinding");
        auto cameraBinding = m_logicEngine.createRamsesCameraBinding(*m_camera, "CameraBinding");

        auto nodeBindingTranslation = nodeBinding->getInputs()->getChild("translation");
        nodeBindingTranslation->set(vec3f{1.f, 2.f, 3.f});
        auto appearanceBindingFloatUniform = appearanceBinding->getInputs()->getChild("floatUniform");
        appearanceBindingFloatUniform->set(42.f);
        auto cameraBindingViewportOffsetX = cameraBinding->getInputs()->getChild("viewPortProperties")->getChild("viewPortOffsetX");
        cameraBindingViewportOffsetX->set(43);

        m_logicEngine.update();

        ramses::UniformInput floatInput;
        m_appearance->getEffect().findUniformInput("floatUniform", floatInput);
        float floatUniformValue = 0.0f;
        m_appearance->getInputValueFloat(floatInput, floatUniformValue);

        EXPECT_FLOAT_EQ(42.f, floatUniformValue);
        EXPECT_EQ(43, m_camera->getViewportX());
        {
            std::array<float, 3> values = {0.0f, 0.0f, 0.0f};
            m_node->getTranslation(values[0], values[1], values[2]);
            EXPECT_THAT(values, ::testing::ElementsAre(1.f, 2.f, 3.f));
        }

        auto nodeBindingScaling = nodeBinding->getInputs()->getChild("scaling");
        auto cameraBindingVpY   = cameraBinding->getInputs()->getChild("viewPortProperties")->getChild("viewPortOffsetY");
        auto scriptOutputVec3   = script->getOutputs()->getChild("outVec3");
        auto scriptOutputFloat  = script->getOutputs()->getChild("outFloat");
        auto scriptOutputInt    = script->getOutputs()->getChild("outInt");
        auto scriptInputVec3    = script->getInputs()->getChild("inVec3");
        auto scriptInputFloat   = script->getInputs()->getChild("inFloat");
        auto scriptInputInt     = script->getInputs()->getChild("inInt");
        auto appearanceInput    = appearanceBinding->getInputs()->getChild("floatUniform");

        m_logicEngine.link(*scriptOutputVec3, *nodeBindingScaling);
        scriptInputVec3->set(vec3f{3.f, 2.f, 1.f});
        scriptInputFloat->set(42.f);
        scriptInputInt->set(43);

        m_logicEngine.update();
        EXPECT_FLOAT_EQ(42.f, floatUniformValue);
        EXPECT_EQ(43, m_camera->getViewportX());
        {
            std::array<float, 3> values = {0.0f, 0.0f, 0.0f};
            m_node->getTranslation(values[0], values[1], values[2]);
            EXPECT_THAT(values, ::testing::ElementsAre(1.f, 2.f, 3.f));
        }
        {
            std::array<float, 3> values = {0.0f, 0.0f, 0.0f};
            m_node->getScaling(values[0], values[1], values[2]);
            EXPECT_THAT(values, ::testing::ElementsAre(3.f, 2.f, 1.f));
        }
        {
            std::array<float, 3> values = { 0.0f, 0.0f, 0.0f };
            ramses::ERotationConvention unused;
            (void)unused;
            m_node->getRotation(values[0], values[1], values[2], unused);
            EXPECT_THAT(values, ::testing::ElementsAre(0.f, 0.f, 0.f));
        }

        ramses::UniformInput floatUniform;
        m_appearance->getEffect().findUniformInput("floatUniform", floatUniform);
        floatUniformValue = 0.0f;
        m_appearance->getInputValueFloat(floatUniform, floatUniformValue);

        EXPECT_FLOAT_EQ(42.f, floatUniformValue);

        m_logicEngine.link(*scriptOutputFloat, *appearanceInput);
        m_logicEngine.link(*scriptOutputInt, *cameraBindingVpY);

        m_logicEngine.update();

        m_appearance->getInputValueFloat(floatUniform, floatUniformValue);
        EXPECT_FLOAT_EQ(42.f, floatUniformValue);

        EXPECT_EQ(43, m_camera->getViewportY());

        m_logicEngine.unlink(*scriptOutputVec3, *nodeBindingScaling);
    }

    TEST_F(ALogicEngine_Update, OnlyUpdatesDirtyLogicNodes)
    {
        auto        scriptSource = R"(
            function interface()
                IN.inFloat = FLOAT
                OUT.outFloat = FLOAT
            end
            function run()
                OUT.outFloat = IN.inFloat
                print("executed")
            end
        )";

        auto sourceScript = m_logicEngine.createLuaScriptFromSource(scriptSource, "SourceScript");
        auto targetScript = m_logicEngine.createLuaScriptFromSource(scriptSource, "TargetScript");

        auto sourceInput  = sourceScript->getInputs()->getChild("inFloat");
        auto sourceOutput = sourceScript->getOutputs()->getChild("outFloat");
        auto targetInput  = targetScript->getInputs()->getChild("inFloat");

        std::vector<std::pair<std::string, std::string>> messages;
        sourceScript->overrideLuaPrint([&messages](std::string_view scriptName, std::string_view message) {
            messages.emplace_back(std::make_pair(scriptName, message));
        });
        targetScript->overrideLuaPrint([&messages](std::string_view scriptName, std::string_view message) {
            messages.emplace_back(std::make_pair(scriptName, message));
        });

        EXPECT_TRUE(sourceScript->m_impl.get().isDirty());
        EXPECT_TRUE(targetScript->m_impl.get().isDirty());

        m_logicEngine.link(*sourceOutput, *targetInput);

        EXPECT_TRUE(sourceScript->m_impl.get().isDirty());
        EXPECT_TRUE(targetScript->m_impl.get().isDirty());

        m_logicEngine.update();

        EXPECT_FALSE(sourceScript->m_impl.get().isDirty());
        EXPECT_FALSE(targetScript->m_impl.get().isDirty());

        // both scripts are updated, because its the first update
        ASSERT_EQ(2u, messages.size());
        EXPECT_EQ("SourceScript", messages[0].first);
        EXPECT_EQ("TargetScript", messages[1].first);

        messages.clear();
        m_logicEngine.update();

        EXPECT_TRUE(messages.empty());
        messages.clear();

        targetInput->set(42.f);

        // targetScript is linked input and cannot be set manually so it is not dirty
        EXPECT_FALSE(sourceScript->m_impl.get().isDirty());
        EXPECT_FALSE(targetScript->m_impl.get().isDirty());

        m_logicEngine.update();

        EXPECT_FALSE(sourceScript->m_impl.get().isDirty());
        EXPECT_FALSE(targetScript->m_impl.get().isDirty());

        // Nothing is updated, because targetScript is linked input and cannot be set manually
        ASSERT_EQ(0u, messages.size());

        sourceInput->set(24.f);
        messages.clear();
        m_logicEngine.update();

        // Both scripts are updated, because the input of the first script is changed and changes target through link.
        ASSERT_EQ(2u, messages.size());
        EXPECT_EQ("SourceScript", messages[0].first);
        EXPECT_EQ("TargetScript", messages[1].first);
    }

    TEST_F(ALogicEngine_Update, OnlyUpdatesDirtyLogicNodesInAComplexLogicGraph)
    {
        auto        scriptSource = R"(
            function interface()
                IN.in1 = INT
                IN.in2 = INT
                OUT.out = INT
            end
            function run()
                OUT.out = IN.in1 + IN.in2
                print("executed")
            end
        )";

        std::array<LuaScript*, 6> s = {};
        for (size_t i = 0; i < s.size(); ++i)
        {
            s[i] = m_logicEngine.createLuaScriptFromSource(scriptSource, fmt::format("Script{}", i));
        }

        auto in1S0  = s[0]->getInputs()->getChild("in1");
        auto out1S0 = s[0]->getOutputs()->getChild("out");
        auto in1S1  = s[1]->getInputs()->getChild("in1");
        auto in2S1  = s[1]->getInputs()->getChild("in2");
        auto out1S1 = s[1]->getOutputs()->getChild("out");
        auto in1S2  = s[2]->getInputs()->getChild("in1");
        auto out1S2 = s[2]->getOutputs()->getChild("out");
        auto in1S3  = s[3]->getInputs()->getChild("in1");
        auto in2S3  = s[3]->getInputs()->getChild("in2");
        auto out1S3 = s[3]->getOutputs()->getChild("out");
        auto in1S4  = s[4]->getInputs()->getChild("in1");
        auto in2S4  = s[4]->getInputs()->getChild("in2");
        auto out1S4 = s[4]->getOutputs()->getChild("out");
        auto in1S5  = s[5]->getInputs()->getChild("in1");
        auto in2S5  = s[5]->getInputs()->getChild("in2");

        /*
                 s2 -------
                    \      \
            s0 ----- s1 -- s3 - s5
                            \  /
                             s4
         */

        m_logicEngine.link(*out1S0, *in2S1);
        m_logicEngine.link(*out1S1, *in2S3);
        m_logicEngine.link(*out1S2, *in1S1);
        m_logicEngine.link(*out1S2, *in1S3);
        m_logicEngine.link(*out1S3, *in1S5);
        m_logicEngine.link(*out1S3, *in1S4);
        m_logicEngine.link(*out1S4, *in2S5);

        std::vector<std::string> messages;
        for (auto script : s)
        {
            script->overrideLuaPrint([&messages](std::string_view scriptName, std::string_view /*message*/) { messages.emplace_back(scriptName); });
        }

        m_logicEngine.update();

        // All scripts are executed
        ASSERT_THAT(messages, ::testing::UnorderedElementsAreArray({ "Script0", "Script1", "Script2", "Script3", "Script4", "Script5" }));
        messages.clear();

        m_logicEngine.update();
        EXPECT_TRUE(messages.empty());

        in2S4->set(1);
        m_logicEngine.update();
        ASSERT_THAT(messages, ::testing::UnorderedElementsAreArray({"Script4", "Script5"}));
        messages.clear();

        m_logicEngine.update();
        EXPECT_TRUE(messages.empty());

        in1S2->set(2);
        m_logicEngine.update();
        ASSERT_THAT(messages, ::testing::UnorderedElementsAreArray({"Script1", "Script2", "Script3", "Script4", "Script5"}));
        messages.clear();

        m_logicEngine.update();
        EXPECT_TRUE(messages.empty());

        in1S0->set(42);
        m_logicEngine.update();
        ASSERT_THAT(messages, ::testing::UnorderedElementsAreArray({"Script0", "Script1", "Script3", "Script4", "Script5"}));
        messages.clear();

        m_logicEngine.update();
        EXPECT_TRUE(messages.empty());

        in1S0->set(24);
        in1S2->set(23);
        m_logicEngine.update();
        ASSERT_THAT(messages, ::testing::UnorderedElementsAreArray({"Script0", "Script1", "Script2", "Script3", "Script4", "Script5"}));
    }

    TEST_F(ALogicEngine_Update, AlwaysUpdatesNodeIfDirtyHandlingIsDisabled)
    {
        auto        scriptSource = R"(
            function interface()
                IN.inFloat = FLOAT
                OUT.outFloat = FLOAT
            end
            function run()
                OUT.outFloat = IN.inFloat
                print("executed")
            end
        )";

        auto sourceScript = m_logicEngine.createLuaScriptFromSource(scriptSource, "SourceScript");
        auto targetScript = m_logicEngine.createLuaScriptFromSource(scriptSource, "TargetScript");

        auto sourceInput  = sourceScript->getInputs()->getChild("inFloat");
        auto sourceOutput = sourceScript->getOutputs()->getChild("outFloat");
        auto targetInput  = targetScript->getInputs()->getChild("inFloat");

        std::vector<std::pair<std::string, std::string>> messages;
        sourceScript->overrideLuaPrint([&messages](std::string_view scriptName, std::string_view message) { messages.emplace_back(std::make_pair(scriptName, message)); });
        targetScript->overrideLuaPrint([&messages](std::string_view scriptName, std::string_view message) { messages.emplace_back(std::make_pair(scriptName, message)); });

        m_logicEngine.link(*sourceOutput, *targetInput);
        m_logicEngine.m_impl->update(true);

        // both scripts are updated, because its the first update
        ASSERT_EQ(2u, messages.size());
        EXPECT_EQ("SourceScript", messages[0].first);
        EXPECT_EQ("TargetScript", messages[1].first);

        targetInput->set(42.f);
        messages.clear();
        m_logicEngine.m_impl->update(true);

        // Both scripts are updated, because dirty handling is disabled
        ASSERT_EQ(2u, messages.size());
        EXPECT_EQ("SourceScript", messages[0].first);
        EXPECT_EQ("TargetScript", messages[1].first);

        sourceInput->set(24.f);
        messages.clear();
        m_logicEngine.m_impl->update(true);

        // Both scripts are updated, because dirty handling is disabled
        ASSERT_EQ(2u, messages.size());
        EXPECT_EQ("SourceScript", messages[0].first);
        EXPECT_EQ("TargetScript", messages[1].first);
    }
}

