//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "impl/RamsesCameraBindingImpl.h"
#include "impl/PropertyImpl.h"
#include "impl/LoggerImpl.h"

#include "internals/RamsesHelper.h"
#include "internals/ErrorReporting.h"
#include "internals/IRamsesObjectResolver.h"

#include "ramses-logic/EPropertyType.h"
#include "ramses-logic/Property.h"

#include "generated/RamsesCameraBindingGen.h"

#include "ramses-utils.h"
#include "ramses-client-api/Camera.h"
#include "ramses-client-api/PerspectiveCamera.h"

namespace rlogic::internal
{
    RamsesCameraBindingImpl::RamsesCameraBindingImpl(ramses::Camera& ramsesCamera, std::string_view name)
        : RamsesBindingImpl(name)
        , m_ramsesCamera(ramsesCamera)
    {
        auto inputs = std::make_unique<Property>(std::make_unique<PropertyImpl>("IN", EPropertyType::Struct, EPropertySemantics::BindingInput));

        // Attention! This order is important - it has to match the indices in ECameraViewportPropertyStaticIndex
        std::unique_ptr<PropertyImpl> viewPortProperty = std::make_unique<PropertyImpl>("viewPortProperties", EPropertyType::Struct, EPropertySemantics::BindingInput);
        viewPortProperty->addChild(std::make_unique<PropertyImpl>("viewPortOffsetX", EPropertyType::Int32, EPropertySemantics::BindingInput));
        viewPortProperty->addChild(std::make_unique<PropertyImpl>("viewPortOffsetY", EPropertyType::Int32, EPropertySemantics::BindingInput));
        viewPortProperty->addChild(std::make_unique<PropertyImpl>("viewPortWidth", EPropertyType::Int32, EPropertySemantics::BindingInput, PropertyValue{ int32_t{16} }));
        viewPortProperty->addChild(std::make_unique<PropertyImpl>("viewPortHeight", EPropertyType::Int32, EPropertySemantics::BindingInput, PropertyValue{ int32_t{16} }));

        // Add frustum struct
        std::unique_ptr<PropertyImpl> frustumProperty = std::make_unique<PropertyImpl>("frustumProperties", EPropertyType::Struct, EPropertySemantics::BindingInput);
        frustumProperty->addChild(std::make_unique<PropertyImpl>("nearPlane", EPropertyType::Float, EPropertySemantics::BindingInput, PropertyValue{ 0.1f }));
        frustumProperty->addChild(std::make_unique<PropertyImpl>("farPlane", EPropertyType::Float, EPropertySemantics::BindingInput, PropertyValue{ 1.0f }));
        switch (m_ramsesCamera.get().getType())
        {
        case ramses::ERamsesObjectType::ERamsesObjectType_PerspectiveCamera:
            // Attention! This order is important - it has to match the indices in EPerspectiveCameraFrustumPropertyStaticIndex
            frustumProperty->addChild(std::make_unique<PropertyImpl>("fieldOfView", EPropertyType::Float, EPropertySemantics::BindingInput, PropertyValue{ 168.579f }));
            frustumProperty->addChild(std::make_unique<PropertyImpl>("aspectRatio", EPropertyType::Float, EPropertySemantics::BindingInput, PropertyValue{ 1.0f }));
            break;
        case ramses::ERamsesObjectType::ERamsesObjectType_OrthographicCamera:
            // Attention! This order is important - it has to match the indices in EOrthographicCameraFrustumPropertyStaticIndex
            frustumProperty->addChild(std::make_unique<PropertyImpl>("leftPlane", EPropertyType::Float, EPropertySemantics::BindingInput, PropertyValue{ -1.0f }));
            frustumProperty->addChild(std::make_unique<PropertyImpl>("rightPlane", EPropertyType::Float, EPropertySemantics::BindingInput, PropertyValue{ 1.0f }));
            frustumProperty->addChild(std::make_unique<PropertyImpl>("bottomPlane", EPropertyType::Float, EPropertySemantics::BindingInput, PropertyValue{ -1.0f }));
            frustumProperty->addChild(std::make_unique<PropertyImpl>("topPlane", EPropertyType::Float, EPropertySemantics::BindingInput, PropertyValue{ 1.0f }));
            break;
        default:
            assert(false && "This should never happen");
            break;
        }

        // Attention! This order is important - it has to match the indices in ECameraPropertyStructStaticIndex
        inputs->m_impl->addChild(std::move(viewPortProperty));
        inputs->m_impl->addChild(std::move(frustumProperty));

        setRootProperties(std::move(inputs), {});
    }

    flatbuffers::Offset<rlogic_serialization::RamsesCameraBinding> RamsesCameraBindingImpl::Serialize(
        const RamsesCameraBindingImpl& cameraBinding,
        flatbuffers::FlatBufferBuilder& builder,
        SerializationMap& serializationMap)
    {
        auto ramsesReference = RamsesBindingImpl::SerializeRamsesReference(cameraBinding.m_ramsesCamera, builder);

        auto ramsesBinding = rlogic_serialization::CreateRamsesBinding(builder,
            builder.CreateString(cameraBinding.getName()),
            ramsesReference,
            // TODO Violin don't serialize these - they carry no useful information and are redundant
            PropertyImpl::Serialize(*cameraBinding.getInputs()->m_impl, builder, serializationMap));
        builder.Finish(ramsesBinding);

        auto ramsesCameraBinding = rlogic_serialization::CreateRamsesCameraBinding(builder, ramsesBinding);
        builder.Finish(ramsesCameraBinding);

        return ramsesCameraBinding;
    }

    std::unique_ptr<RamsesCameraBindingImpl> RamsesCameraBindingImpl::Deserialize(
        const rlogic_serialization::RamsesCameraBinding& cameraBinding,
        const IRamsesObjectResolver& ramsesResolver,
        ErrorReporting& errorReporting,
        DeserializationMap& deserializationMap)
    {
        if (!cameraBinding.base())
        {
            errorReporting.add("Fatal error during loading of RamsesCameraBinding from serialized data: missing base class info!");
            return nullptr;
        }

        // TODO Violin make optional - no need to always serialize string if not used
        if (!cameraBinding.base()->name())
        {
            errorReporting.add("Fatal error during loading of RamsesCameraBinding from serialized data: missing name!");
            return nullptr;
        }

        const std::string_view name = cameraBinding.base()->name()->string_view();

        if (!cameraBinding.base()->rootInput())
        {
            errorReporting.add("Fatal error during loading of RamsesCameraBinding from serialized data: missing root input!");
            return nullptr;
        }

        std::unique_ptr<PropertyImpl> deserializedRootInput = PropertyImpl::Deserialize(*cameraBinding.base()->rootInput(), EPropertySemantics::BindingInput, errorReporting, deserializationMap);

        if (!deserializedRootInput)
        {
            return nullptr;
        }

        // TODO Violin don't serialize these inputs -> get rid of the check
        if (deserializedRootInput->getName() != "IN" || deserializedRootInput->getType() != EPropertyType::Struct)
        {
            errorReporting.add("Fatal error during loading of RamsesCameraBinding from serialized data: root input has unexpected name or type!");
            return nullptr;
        }

        const auto* boundObject = cameraBinding.base()->boundRamsesObject();
        if (!boundObject)
        {
            errorReporting.add("Fatal error during loading of RamsesCameraBinding from serialized data: no reference to ramses camera!");
            return nullptr;
        }

        const ramses::sceneObjectId_t objectId(boundObject->objectId());

        ramses::Camera* resolvedCamera = ramsesResolver.findRamsesCameraInScene(name, objectId);
        if (!resolvedCamera)
        {
            // TODO Violin improve error reporting for this particular error (it's reported in ramsesResolver currently): provide better message and scene/camera ids
            return nullptr;
        }

        if (resolvedCamera->getType() != static_cast<int>(boundObject->objectType()))
        {
            errorReporting.add("Fatal error during loading of RamsesCameraBinding from serialized data: loaded type does not match referenced camera type!");
            return nullptr;
        }

        auto binding = std::make_unique<RamsesCameraBindingImpl>(*resolvedCamera, name);
        binding->setRootProperties(std::make_unique<Property>(std::move(deserializedRootInput)), {});

        return binding;
    }

    std::optional<LogicNodeRuntimeError> RamsesCameraBindingImpl::update()
    {
        ramses::status_t status = ramses::StatusOK;
        PropertyImpl& vpProperties = *getInputs()->getChild(static_cast<size_t>(ECameraPropertyStructStaticIndex::Viewport))->m_impl;

        PropertyImpl& vpOffsetX = *vpProperties.getChild(static_cast<size_t>(ECameraViewportPropertyStaticIndex::ViewPortOffsetX))->m_impl;
        PropertyImpl& vpOffsetY = *vpProperties.getChild(static_cast<size_t>(ECameraViewportPropertyStaticIndex::ViewPortOffsetY))->m_impl;
        PropertyImpl& vpWidth = *vpProperties.getChild(static_cast<size_t>(ECameraViewportPropertyStaticIndex::ViewPortWidth))->m_impl;
        PropertyImpl& vpHeight = *vpProperties.getChild(static_cast<size_t>(ECameraViewportPropertyStaticIndex::ViewPortHeight))->m_impl;
        if (vpOffsetX.checkForBindingInputNewValueAndReset()
            || vpOffsetY.checkForBindingInputNewValueAndReset()
            || vpWidth.checkForBindingInputNewValueAndReset()
            || vpHeight.checkForBindingInputNewValueAndReset())
        {
            const int32_t vpX = vpOffsetX.getValueAs<int32_t>();
            const int32_t vpY = vpOffsetY.getValueAs<int32_t>();
            const int32_t vpW = vpWidth.getValueAs<int32_t>();
            const int32_t vpH = vpHeight.getValueAs<int32_t>();

            if (vpW <= 0 || vpH <= 0)
            {
                return LogicNodeRuntimeError{ fmt::format("Camera viewport size must be positive! (width: {}; height: {})", vpW, vpH) };
            }

            status = m_ramsesCamera.get().setViewport(vpX, vpY, vpW, vpH);

            if (status != ramses::StatusOK)
            {
                return LogicNodeRuntimeError{m_ramsesCamera.get().getStatusMessage(status)};
            }
        }

        PropertyImpl& frustumProperties = *getInputs()->getChild(static_cast<size_t>(ECameraPropertyStructStaticIndex::Frustum))->m_impl;

        // Index of Perspective Frustum Properties is used, but wouldn't matter as Ortho Camera indeces are the same for these two properties
        PropertyImpl& nearPlane = *frustumProperties.getChild(static_cast<size_t>(EPerspectiveCameraFrustumPropertyStaticIndex::NearPlane))->m_impl;
        PropertyImpl& farPlane = *frustumProperties.getChild(static_cast<size_t>(EPerspectiveCameraFrustumPropertyStaticIndex::FarPlane))->m_impl;

        if (m_ramsesCamera.get().getType() == ramses::ERamsesObjectType_PerspectiveCamera)
        {
            PropertyImpl& fov = *frustumProperties.getChild(static_cast<size_t>(EPerspectiveCameraFrustumPropertyStaticIndex::FieldOfView))->m_impl;
            PropertyImpl& aR = *frustumProperties.getChild(static_cast<size_t>(EPerspectiveCameraFrustumPropertyStaticIndex::AspectRatio))->m_impl;

            if (nearPlane.checkForBindingInputNewValueAndReset()
                || farPlane.checkForBindingInputNewValueAndReset()
                || fov.checkForBindingInputNewValueAndReset()
                || aR.checkForBindingInputNewValueAndReset())
            {
                ramses::PerspectiveCamera* perspectiveCam = nullptr;
                perspectiveCam = ramses::RamsesUtils::TryConvert<ramses::PerspectiveCamera>(m_ramsesCamera.get());
                status = perspectiveCam->setFrustum(fov.getValueAs<float>(), aR.getValueAs<float>(), nearPlane.getValueAs<float>(), farPlane.getValueAs<float>());

                if (status != ramses::StatusOK)
                {
                    return LogicNodeRuntimeError{ m_ramsesCamera.get().getStatusMessage(status) };
                }
            }
        }
        else if (m_ramsesCamera.get().getType() == ramses::ERamsesObjectType_OrthographicCamera)
        {
            PropertyImpl& leftPlane = *frustumProperties.getChild(static_cast<size_t>(EOrthographicCameraFrustumPropertyStaticIndex::LeftPlane))->m_impl;
            PropertyImpl& rightPlane = *frustumProperties.getChild(static_cast<size_t>(EOrthographicCameraFrustumPropertyStaticIndex::RightPlane))->m_impl;
            PropertyImpl& bottomPlane = *frustumProperties.getChild(static_cast<size_t>(EOrthographicCameraFrustumPropertyStaticIndex::BottomPlane))->m_impl;
            PropertyImpl& topPlane = *frustumProperties.getChild(static_cast<size_t>(EOrthographicCameraFrustumPropertyStaticIndex::TopPlane))->m_impl;

            if (nearPlane.checkForBindingInputNewValueAndReset()
                || farPlane.checkForBindingInputNewValueAndReset()
                || leftPlane.checkForBindingInputNewValueAndReset()
                || rightPlane.checkForBindingInputNewValueAndReset()
                || bottomPlane.checkForBindingInputNewValueAndReset()
                || topPlane.checkForBindingInputNewValueAndReset())
            {
                status = m_ramsesCamera.get().setFrustum(
                    leftPlane.getValueAs<float>(),
                    rightPlane.getValueAs<float>(),
                    bottomPlane.getValueAs<float>(),
                    topPlane.getValueAs<float>(),
                    nearPlane.getValueAs<float>(),
                    farPlane.getValueAs<float>());

                if (status != ramses::StatusOK)
                {
                    return LogicNodeRuntimeError{ m_ramsesCamera.get().getStatusMessage(status) };
                }
            }
        }
        else
        {
            assert(false && "this should never happen");
        }

        return std::nullopt;
    }

    ramses::ERamsesObjectType RamsesCameraBindingImpl::getCameraType() const
    {
        return m_ramsesCamera.get().getType();
    }

    ramses::Camera& RamsesCameraBindingImpl::getRamsesCamera() const
    {
        return m_ramsesCamera;
    }
}
