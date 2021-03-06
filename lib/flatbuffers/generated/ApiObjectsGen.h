// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_APIOBJECTS_RLOGIC_SERIALIZATION_H_
#define FLATBUFFERS_GENERATED_APIOBJECTS_RLOGIC_SERIALIZATION_H_

#include "flatbuffers/flatbuffers.h"

#include "LinkGen.h"
#include "LuaScriptGen.h"
#include "PropertyGen.h"
#include "RamsesAppearanceBindingGen.h"
#include "RamsesBindingGen.h"
#include "RamsesCameraBindingGen.h"
#include "RamsesNodeBindingGen.h"
#include "RamsesReferenceGen.h"

namespace rlogic_serialization {

struct ApiObjects;
struct ApiObjectsBuilder;

struct ApiObjects FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef ApiObjectsBuilder Builder;
  struct Traits;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_LUASCRIPTS = 4,
    VT_NODEBINDINGS = 6,
    VT_APPEARANCEBINDINGS = 8,
    VT_CAMERABINDINGS = 10,
    VT_LINKS = 12
  };
  const flatbuffers::Vector<flatbuffers::Offset<rlogic_serialization::LuaScript>> *luaScripts() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<rlogic_serialization::LuaScript>> *>(VT_LUASCRIPTS);
  }
  const flatbuffers::Vector<flatbuffers::Offset<rlogic_serialization::RamsesNodeBinding>> *nodeBindings() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<rlogic_serialization::RamsesNodeBinding>> *>(VT_NODEBINDINGS);
  }
  const flatbuffers::Vector<flatbuffers::Offset<rlogic_serialization::RamsesAppearanceBinding>> *appearanceBindings() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<rlogic_serialization::RamsesAppearanceBinding>> *>(VT_APPEARANCEBINDINGS);
  }
  const flatbuffers::Vector<flatbuffers::Offset<rlogic_serialization::RamsesCameraBinding>> *cameraBindings() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<rlogic_serialization::RamsesCameraBinding>> *>(VT_CAMERABINDINGS);
  }
  const flatbuffers::Vector<flatbuffers::Offset<rlogic_serialization::Link>> *links() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<rlogic_serialization::Link>> *>(VT_LINKS);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_LUASCRIPTS) &&
           verifier.VerifyVector(luaScripts()) &&
           verifier.VerifyVectorOfTables(luaScripts()) &&
           VerifyOffset(verifier, VT_NODEBINDINGS) &&
           verifier.VerifyVector(nodeBindings()) &&
           verifier.VerifyVectorOfTables(nodeBindings()) &&
           VerifyOffset(verifier, VT_APPEARANCEBINDINGS) &&
           verifier.VerifyVector(appearanceBindings()) &&
           verifier.VerifyVectorOfTables(appearanceBindings()) &&
           VerifyOffset(verifier, VT_CAMERABINDINGS) &&
           verifier.VerifyVector(cameraBindings()) &&
           verifier.VerifyVectorOfTables(cameraBindings()) &&
           VerifyOffset(verifier, VT_LINKS) &&
           verifier.VerifyVector(links()) &&
           verifier.VerifyVectorOfTables(links()) &&
           verifier.EndTable();
  }
};

struct ApiObjectsBuilder {
  typedef ApiObjects Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_luaScripts(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<rlogic_serialization::LuaScript>>> luaScripts) {
    fbb_.AddOffset(ApiObjects::VT_LUASCRIPTS, luaScripts);
  }
  void add_nodeBindings(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<rlogic_serialization::RamsesNodeBinding>>> nodeBindings) {
    fbb_.AddOffset(ApiObjects::VT_NODEBINDINGS, nodeBindings);
  }
  void add_appearanceBindings(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<rlogic_serialization::RamsesAppearanceBinding>>> appearanceBindings) {
    fbb_.AddOffset(ApiObjects::VT_APPEARANCEBINDINGS, appearanceBindings);
  }
  void add_cameraBindings(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<rlogic_serialization::RamsesCameraBinding>>> cameraBindings) {
    fbb_.AddOffset(ApiObjects::VT_CAMERABINDINGS, cameraBindings);
  }
  void add_links(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<rlogic_serialization::Link>>> links) {
    fbb_.AddOffset(ApiObjects::VT_LINKS, links);
  }
  explicit ApiObjectsBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ApiObjectsBuilder &operator=(const ApiObjectsBuilder &);
  flatbuffers::Offset<ApiObjects> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<ApiObjects>(end);
    return o;
  }
};

inline flatbuffers::Offset<ApiObjects> CreateApiObjects(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<rlogic_serialization::LuaScript>>> luaScripts = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<rlogic_serialization::RamsesNodeBinding>>> nodeBindings = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<rlogic_serialization::RamsesAppearanceBinding>>> appearanceBindings = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<rlogic_serialization::RamsesCameraBinding>>> cameraBindings = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<rlogic_serialization::Link>>> links = 0) {
  ApiObjectsBuilder builder_(_fbb);
  builder_.add_links(links);
  builder_.add_cameraBindings(cameraBindings);
  builder_.add_appearanceBindings(appearanceBindings);
  builder_.add_nodeBindings(nodeBindings);
  builder_.add_luaScripts(luaScripts);
  return builder_.Finish();
}

struct ApiObjects::Traits {
  using type = ApiObjects;
  static auto constexpr Create = CreateApiObjects;
};

inline flatbuffers::Offset<ApiObjects> CreateApiObjectsDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    const std::vector<flatbuffers::Offset<rlogic_serialization::LuaScript>> *luaScripts = nullptr,
    const std::vector<flatbuffers::Offset<rlogic_serialization::RamsesNodeBinding>> *nodeBindings = nullptr,
    const std::vector<flatbuffers::Offset<rlogic_serialization::RamsesAppearanceBinding>> *appearanceBindings = nullptr,
    const std::vector<flatbuffers::Offset<rlogic_serialization::RamsesCameraBinding>> *cameraBindings = nullptr,
    const std::vector<flatbuffers::Offset<rlogic_serialization::Link>> *links = nullptr) {
  auto luaScripts__ = luaScripts ? _fbb.CreateVector<flatbuffers::Offset<rlogic_serialization::LuaScript>>(*luaScripts) : 0;
  auto nodeBindings__ = nodeBindings ? _fbb.CreateVector<flatbuffers::Offset<rlogic_serialization::RamsesNodeBinding>>(*nodeBindings) : 0;
  auto appearanceBindings__ = appearanceBindings ? _fbb.CreateVector<flatbuffers::Offset<rlogic_serialization::RamsesAppearanceBinding>>(*appearanceBindings) : 0;
  auto cameraBindings__ = cameraBindings ? _fbb.CreateVector<flatbuffers::Offset<rlogic_serialization::RamsesCameraBinding>>(*cameraBindings) : 0;
  auto links__ = links ? _fbb.CreateVector<flatbuffers::Offset<rlogic_serialization::Link>>(*links) : 0;
  return rlogic_serialization::CreateApiObjects(
      _fbb,
      luaScripts__,
      nodeBindings__,
      appearanceBindings__,
      cameraBindings__,
      links__);
}

}  // namespace rlogic_serialization

#endif  // FLATBUFFERS_GENERATED_APIOBJECTS_RLOGIC_SERIALIZATION_H_
