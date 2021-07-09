//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "ramses-logic/EPropertyType.h"
#include "internals/EPropertySemantics.h"
#include "internals/SerializationMap.h"
#include "internals/DeserializationMap.h"

#include <cassert>
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include <variant>
#include <memory>

namespace rlogic
{
    class Property;
}
namespace rlogic_serialization
{
    struct Property;
}

namespace flatbuffers
{
    template<typename T> struct Offset;
    class FlatBufferBuilder;
}

namespace rlogic::internal
{
    class LogicNodeImpl;
    class ErrorReporting;

    using PropertyValue = std::variant<int32_t, float, bool, std::string, vec2f, vec3f, vec4f, vec2i, vec3i, vec4i>;

    class PropertyImpl
    {
    public:
        PropertyImpl(std::string_view name, EPropertyType type, EPropertySemantics semantics);
        PropertyImpl(std::string_view name, EPropertyType type, EPropertySemantics semantics, PropertyValue initialValue);

        [[nodiscard]] static flatbuffers::Offset<rlogic_serialization::Property> Serialize(
            const PropertyImpl& prop,
            flatbuffers::FlatBufferBuilder& builder,
            SerializationMap& serializationMap);

        [[nodiscard]] static std::unique_ptr<PropertyImpl> Deserialize(
            const rlogic_serialization::Property& prop,
            EPropertySemantics semantics,
            ErrorReporting& errorReporting,
            DeserializationMap& deserializationMap);


        // Move-able (noexcept); Not copy-able
        ~PropertyImpl() noexcept = default;
        PropertyImpl& operator=(PropertyImpl&& other) noexcept = default;
        PropertyImpl(PropertyImpl&& other) noexcept = default;
        PropertyImpl& operator=(const PropertyImpl& other) = delete;
        PropertyImpl(const PropertyImpl& other) = delete;

        // Creates a data copy of itself and its children on new memory
        [[nodiscard]] std::unique_ptr<PropertyImpl> deepCopy() const;

        [[nodiscard]] size_t getChildCount() const;
        [[nodiscard]] EPropertyType getType() const;
        [[nodiscard]] std::string_view getName() const;

        [[nodiscard]] bool bindingInputHasNewValue() const;
        [[nodiscard]] bool checkForBindingInputNewValueAndReset();

        [[nodiscard]] const Property* getChild(size_t index) const;

        // TODO Violin these 3 methods have redundancy, refactor
        [[nodiscard]] bool isInput() const;
        [[nodiscard]] bool isOutput() const;
        [[nodiscard]] EPropertySemantics getPropertySemantics() const;

        [[nodiscard]] Property* getChild(size_t index);
        [[nodiscard]] Property* getChild(std::string_view name);
        [[nodiscard]] bool hasChild(std::string_view name) const;

        void addChild(std::unique_ptr<PropertyImpl> child, bool sortChildrenLexicographically = false);
        void clearChildren();

        // Public API access - only ever called by user, full error check and logs
        template <typename T>
        [[nodiscard]] std::optional<T> getValue_PublicApi() const;
        [[nodiscard]] bool setValue_PublicApi(PropertyValue value);

        // Access from inside Lua scripts
        void setOutputValue_FromScript(PropertyValue value);

        // Generic setter. Can optionally skip dirty-check
        void setValue(PropertyValue value, bool checkDirty = true);

        // Generic getter for use in other non-template code
        [[nodiscard]] const PropertyValue& getValue() const;
        // std::get wrapper for use in template code
        template <typename T>
        [[nodiscard]] const T& getValueAs() const
        {
            return std::get<T>(m_value);
        }

        // Design smells (can fix by changing class design and topology)
        void setIsLinkedInput(bool isLinkedInput);
        void setLogicNode(LogicNodeImpl& logicNode);
        [[nodiscard]] LogicNodeImpl& getLogicNode();

    private:
        std::string                                     m_name;
        EPropertyType                                   m_type;
        std::vector<std::unique_ptr<Property>>          m_children;
        PropertyValue                                   m_value;

        // TODO Violin/Sven consider solving this more elegantly
        LogicNodeImpl*                                  m_logicNode = nullptr;
        bool m_bindingInputHasNewValue = false;
        bool m_isLinkedInput = false;
        EPropertySemantics                              m_semantics;

        [[nodiscard]] static flatbuffers::Offset<rlogic_serialization::Property> SerializeRecursive(
            const PropertyImpl& prop,
            flatbuffers::FlatBufferBuilder& builder,
            SerializationMap& serializationMap);
    };
}
