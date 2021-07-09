//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "impl/LogicNodeImpl.h"
#include "impl/PropertyImpl.h"

#include "ramses-logic/LogicNode.h"
#include "ramses-logic/Property.h"

namespace rlogic::internal
{
    class LogicNodeDummyImpl : public LogicNodeImpl
    {
    public:
        explicit LogicNodeDummyImpl(std::string_view name, bool createNestedProperties = false)
            : LogicNodeImpl(name)
        {
            auto inputs = std::make_unique<Property>(std::make_unique<PropertyImpl>("IN", EPropertyType::Struct, EPropertySemantics::ScriptInput));
            auto outputs = std::make_unique<Property>(std::make_unique<PropertyImpl>("OUT", EPropertyType::Struct, EPropertySemantics::ScriptOutput));

            inputs->m_impl->addChild(std::make_unique<PropertyImpl>("input1", EPropertyType::Int32, EPropertySemantics::ScriptInput));
            inputs->m_impl->addChild(std::make_unique<PropertyImpl>("input2", EPropertyType::Int32, EPropertySemantics::ScriptInput));

            outputs->m_impl->addChild(std::make_unique<PropertyImpl>("output1", EPropertyType::Int32, EPropertySemantics::ScriptOutput));
            outputs->m_impl->addChild(std::make_unique<PropertyImpl>("output2", EPropertyType::Int32, EPropertySemantics::ScriptOutput));

            if (createNestedProperties)
            {
                inputs->m_impl->addChild(std::make_unique<PropertyImpl>("inputStruct", EPropertyType::Struct, EPropertySemantics::ScriptInput));
                inputs->getChild("inputStruct")->m_impl->addChild(std::make_unique<PropertyImpl>("nested", EPropertyType::Int32, EPropertySemantics::ScriptInput));

                outputs->m_impl->addChild(std::make_unique<PropertyImpl>("outputStruct", EPropertyType::Struct, EPropertySemantics::ScriptOutput));
                outputs->getChild("outputStruct")->m_impl->addChild(std::make_unique<PropertyImpl>("nested", EPropertyType::Int32, EPropertySemantics::ScriptOutput));

                inputs->m_impl->addChild(std::make_unique<PropertyImpl>("inputArray", EPropertyType::Array, EPropertySemantics::ScriptInput));
                inputs->getChild("inputArray")->m_impl->addChild(std::make_unique<PropertyImpl>("", EPropertyType::Int32, EPropertySemantics::ScriptInput));

                outputs->m_impl->addChild(std::make_unique<PropertyImpl>("outputArray", EPropertyType::Array, EPropertySemantics::ScriptOutput));
                outputs->getChild("outputArray")->m_impl->addChild(std::make_unique<PropertyImpl>("", EPropertyType::Int32, EPropertySemantics::ScriptOutput));
            }

            setRootProperties(std::move(inputs), std::move(outputs));
        }

        std::optional<LogicNodeRuntimeError> update() override
        {
            return std::nullopt;
        }

    private:
    };

    class LogicNodeDummy : public LogicNode
    {
    public:
        explicit LogicNodeDummy(std::unique_ptr<LogicNodeDummyImpl> impl)
            : LogicNode(std::ref(static_cast<LogicNodeImpl&>(*impl)))
            , m_node(std::move(impl))
        {
        }
        static std::unique_ptr<LogicNodeDummy> Create(std::string_view name)
        {
            return std::make_unique<LogicNodeDummy>(std::make_unique<LogicNodeDummyImpl>(name));
        }

        std::unique_ptr<LogicNodeDummyImpl> m_node;
    };
}
