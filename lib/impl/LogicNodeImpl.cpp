//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "impl/LogicNodeImpl.h"
#include "impl/PropertyImpl.h"

#include "ramses-logic/Property.h"

namespace rlogic::internal
{
    LogicNodeImpl::LogicNodeImpl(std::string_view name) noexcept
        : m_name(name)
    {
    }

    LogicNodeImpl::~LogicNodeImpl() noexcept = default;

    Property* LogicNodeImpl::getInputs()
    {
        return m_inputs.get();
    }

    const Property* LogicNodeImpl::getInputs() const
    {
        return m_inputs.get();
    }

    const Property* LogicNodeImpl::getOutputs() const
    {
        return m_outputs.get();
    }

    Property* LogicNodeImpl::getOutputs()
    {
        return m_outputs.get();
    }

    std::string_view LogicNodeImpl::getName() const
    {
        return m_name;
    }

    void LogicNodeImpl::setName(std::string_view name)
    {
        m_name = name;
    }

    void LogicNodeImpl::setDirty(bool dirty)
    {
        m_dirty = dirty;
    }

    bool LogicNodeImpl::isDirty() const
    {
        return m_dirty;
    }

    void LogicNodeImpl::setRootProperties(std::unique_ptr<Property> rootInput, std::unique_ptr<Property> rootOutput)
    {
        m_inputs = std::move(rootInput);
        if (m_inputs)
        {
            m_inputs->m_impl->setLogicNode(*this);
        }

        m_outputs = std::move(rootOutput);
        if (m_outputs)
        {
            m_outputs->m_impl->setLogicNode(*this);
        }
    }
}
