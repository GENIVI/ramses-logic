//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "impl/RamsesNodeBindingImpl.h"
#include "impl/PropertyImpl.h"

#include "ramses-logic/Property.h"

#include "ramses-client-api/Node.h"

#include "generated/RamsesNodeBindingGen.h"
#include "internals/ErrorReporting.h"
#include "internals/IRamsesObjectResolver.h"

namespace rlogic::internal
{
    RamsesNodeBindingImpl::RamsesNodeBindingImpl(ramses::Node& ramsesNode, std::string_view name)
        : RamsesBindingImpl(name)
        , m_ramsesNode(ramsesNode)
    {
        auto inputs = std::make_unique<Property>(std::make_unique<PropertyImpl>("IN", EPropertyType::Struct, EPropertySemantics::BindingInput));

        // Attention! This order is important - it has to match the indices in ENodePropertyStaticIndex!
        // Set default values equivalent to those of Ramses
        inputs->m_impl->addChild(std::make_unique<PropertyImpl>("visibility", EPropertyType::Bool, EPropertySemantics::BindingInput, PropertyValue{ true }));
        inputs->m_impl->addChild(std::make_unique<PropertyImpl>("rotation", EPropertyType::Vec3f, EPropertySemantics::BindingInput));
        inputs->m_impl->addChild(std::make_unique<PropertyImpl>("translation", EPropertyType::Vec3f, EPropertySemantics::BindingInput));
        inputs->m_impl->addChild(std::make_unique<PropertyImpl>("scaling", EPropertyType::Vec3f, EPropertySemantics::BindingInput, PropertyValue{ vec3f{ 1.f, 1.f, 1.f } }));

        setRootProperties(std::move(inputs), {});
    }

    flatbuffers::Offset<rlogic_serialization::RamsesNodeBinding> RamsesNodeBindingImpl::Serialize(
        const RamsesNodeBindingImpl& nodeBinding,
        flatbuffers::FlatBufferBuilder& builder,
        SerializationMap& serializationMap)
    {
        auto ramsesReference = RamsesBindingImpl::SerializeRamsesReference(nodeBinding.m_ramsesNode, builder);

        auto ramsesBinding = rlogic_serialization::CreateRamsesBinding(builder,
            builder.CreateString(nodeBinding.getName()),
            ramsesReference,
            // TODO Violin don't serialize inputs - it's better to re-create them on the fly, they are uniquely defined and don't need serialization
            PropertyImpl::Serialize(*nodeBinding.getInputs()->m_impl, builder, serializationMap));
        builder.Finish(ramsesBinding);

        auto ramsesNodeBinding = rlogic_serialization::CreateRamsesNodeBinding(builder,
            ramsesBinding,
            static_cast<uint8_t>(nodeBinding.m_rotationConvention)
        );
        builder.Finish(ramsesNodeBinding);

        return ramsesNodeBinding;
    }

    std::unique_ptr<RamsesNodeBindingImpl> RamsesNodeBindingImpl::Deserialize(
        const rlogic_serialization::RamsesNodeBinding& nodeBinding,
        const IRamsesObjectResolver& ramsesResolver,
        ErrorReporting& errorReporting,
        DeserializationMap& deserializationMap)
    {
        if (!nodeBinding.base())
        {
            errorReporting.add("Fatal error during loading of RamsesNodeBinding from serialized data: missing base class info!");
            return nullptr;
        }

        // TODO Violin make optional - no need to always serialize string if not used
        if (!nodeBinding.base()->name())
        {
            errorReporting.add("Fatal error during loading of RamsesNodeBinding from serialized data: missing name!");
            return nullptr;
        }

        const std::string_view name = nodeBinding.base()->name()->string_view();

        if (!nodeBinding.base()->rootInput())
        {
            errorReporting.add("Fatal error during loading of RamsesNodeBinding from serialized data: missing root input!");
            return nullptr;
        }

        std::unique_ptr<PropertyImpl> deserializedRootInput = PropertyImpl::Deserialize(*nodeBinding.base()->rootInput(), EPropertySemantics::BindingInput, errorReporting, deserializationMap);

        if (!deserializedRootInput)
        {
            return nullptr;
        }

        // TODO Violin don't serialize these inputs -> get rid of the check
        if (deserializedRootInput->getName() != "IN" || deserializedRootInput->getType() != EPropertyType::Struct)
        {
            errorReporting.add("Fatal error during loading of RamsesNodeBinding from serialized data: root input has unexpected name or type!");
            return nullptr;
        }

        const auto* boundObject = nodeBinding.base()->boundRamsesObject();
        if (!boundObject)
        {
            errorReporting.add("Fatal error during loading of RamsesNodeBinding from serialized data: missing ramses object reference!");
            return nullptr;
        }

        const ramses::sceneObjectId_t objectId(boundObject->objectId());

        ramses::Node* ramsesNode = ramsesResolver.findRamsesNodeInScene(name, objectId);
        if (!ramsesNode)
        {
            // TODO Violin improve error reporting for this particular error (it's reported in ramsesResolver currently): provide better message and scene/node ids
            return nullptr;
        }

        if (ramsesNode->getType() != static_cast<int>(boundObject->objectType()))
        {
            errorReporting.add("Fatal error during loading of RamsesNodeBinding from serialized data: loaded node type does not match referenced node type!");
            return nullptr;
        }

        auto binding = std::make_unique<RamsesNodeBindingImpl>(*ramsesNode, name);
        binding->setRootProperties(std::make_unique<Property>(std::move(deserializedRootInput)), {});
        binding->m_rotationConvention = static_cast<ramses::ERotationConvention>(nodeBinding.rotationConvention());

        // TODO Violin should check compatibility of rotation convention, but no getter for that currently in ramses!
        // TODO initialize values of inputs with those of ramses node

        return binding;
    }

    std::optional<LogicNodeRuntimeError> RamsesNodeBindingImpl::update()
    {
        ramses::status_t status = ramses::StatusOK;
        PropertyImpl& visibility = *getInputs()->getChild(static_cast<size_t>(ENodePropertyStaticIndex::Visibility))->m_impl;
        if (visibility.checkForBindingInputNewValueAndReset())
        {
            // TODO Violin what about 'Off' state? Worth discussing!
            if (visibility.getValueAs<bool>())
            {
                status = m_ramsesNode.get().setVisibility(ramses::EVisibilityMode::Visible);
            }
            else
            {
                status = m_ramsesNode.get().setVisibility(ramses::EVisibilityMode::Invisible);
            }

            if (status != ramses::StatusOK)
            {
                return LogicNodeRuntimeError{m_ramsesNode.get().getStatusMessage(status)};
            }
        }

        PropertyImpl& rotation = *getInputs()->getChild(static_cast<size_t>(ENodePropertyStaticIndex::Rotation))->m_impl;
        if (rotation.checkForBindingInputNewValueAndReset())
        {
            const auto& value = rotation.getValueAs<vec3f>();
            status = m_ramsesNode.get().setRotation(value[0], value[1], value[2], m_rotationConvention);

            if (status != ramses::StatusOK)
            {
                return LogicNodeRuntimeError{m_ramsesNode.get().getStatusMessage(status)};
            }
        }

        PropertyImpl& translation = *getInputs()->getChild(static_cast<size_t>(ENodePropertyStaticIndex::Translation))->m_impl;
        if (translation.checkForBindingInputNewValueAndReset())
        {
            const auto& value = translation.getValueAs<vec3f>();
            status = m_ramsesNode.get().setTranslation(value[0], value[1], value[2]);

            if (status != ramses::StatusOK)
            {
                return LogicNodeRuntimeError{ m_ramsesNode.get().getStatusMessage(status) };
            }
        }

        PropertyImpl& scaling = *getInputs()->getChild(static_cast<size_t>(ENodePropertyStaticIndex::Scaling))->m_impl;
        if (scaling.checkForBindingInputNewValueAndReset())
        {
            const auto& value = scaling.getValueAs<vec3f>();
            status = m_ramsesNode.get().setScaling(value[0], value[1], value[2]);

            if (status != ramses::StatusOK)
            {
                return LogicNodeRuntimeError{ m_ramsesNode.get().getStatusMessage(status) };
            }
        }

        return std::nullopt;
    }

    ramses::Node& RamsesNodeBindingImpl::getRamsesNode() const
    {
        return m_ramsesNode;
    }

    bool RamsesNodeBindingImpl::setRotationConvention(ramses::ERotationConvention rotationConvention)
    {
        m_rotationConvention = rotationConvention;
        return true;
    }

    ramses::ERotationConvention RamsesNodeBindingImpl::getRotationConvention() const
    {
        return m_rotationConvention;
    }

}
