//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "RamsesTestUtils.h"

#include "ramses-logic/LogicEngine.h"
#include "ramses-logic/LuaScript.h"
#include "ramses-logic/RamsesNodeBinding.h"
#include "ramses-logic/RamsesAppearanceBinding.h"
#include "ramses-logic/RamsesCameraBinding.h"
#include "ramses-client-api/OrthographicCamera.h"
#include "ramses-client-api/Appearance.h"
#include "ramses-utils.h"

namespace rlogic
{
    class ALogicEngine : public ::testing::Test
    {
    protected:
        LogicEngine m_logicEngine;
        RamsesTestSetup m_ramses;
        ramses::Scene* m_scene = { m_ramses.createScene() };
        ramses::Node* m_node = { m_scene->createNode() };
        ramses::OrthographicCamera* m_camera = { m_scene->createOrthographicCamera() };
        ramses::Appearance* m_appearance = { &RamsesTestSetup::CreateTrivialTestAppearance(*m_scene) };

        const std::string_view m_valid_empty_script = R"(
            function interface()
            end
            function run()
            end
        )";

        const std::string_view m_invalid_empty_script = R"(
        )";

        void recreate()
        {
            const ramses::sceneId_t sceneId = m_scene->getSceneId();

            m_ramses.destroyScene(*m_scene);
            m_scene = m_ramses.createScene(sceneId);
            m_node = m_scene->createNode();
            m_camera = m_scene->createOrthographicCamera();
            m_appearance = &RamsesTestSetup::CreateTrivialTestAppearance(*m_scene);
        }

    };
}
