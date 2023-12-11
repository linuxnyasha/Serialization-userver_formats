#include <userver/utest/utest.hpp>
#include "universal_serializing.hpp"
#include "basic_checks.hpp"
#include <userver/utils/regex.hpp>
struct SomeStruct {
  int field1;
  int field2;
  constexpr auto operator==(const SomeStruct& other) const noexcept {
    return (this->field1 == other.field1) && (this->field2 == other.field2);
  };
};
template <>
inline constexpr auto UniversalSerializeLibrary::kSerialization<SomeStruct> =
    UniversalSerializeLibrary::SerializationConfig<SomeStruct>::Create();

UTEST(Serialize, Basic) {
  using UniversalSerializeLibrary::UniversalSerialize;
  SomeStruct a{10, 100};
  const auto json = UniversalSerialize(a, userver::formats::serialize::To<userver::formats::json::Value>{});
  EXPECT_EQ(userver::formats::json::ToString(json), "{\"field1\":10,\"field2\":100}");
};

UTEST(Parse, Basic) {
  using UniversalSerializeLibrary::UniversalParse;
  const auto json = userver::formats::json::FromString("{\"field1\":10,\"field2\":100}");
  const auto fromJson = UniversalParse(json, userver::formats::parse::To<SomeStruct>{});
  constexpr SomeStruct valid{10, 100};
  EXPECT_EQ(fromJson, valid);
};
UTEST(Valid, Basic) {
  using UniversalSerializeLibrary::UniversalValid;
  const auto json = userver::formats::json::FromString("{\"field1\":10,\"field2\":100}");
  const auto json2 = userver::formats::json::FromString("{\"field1\":10,\"field3\":100}");
  const auto json3 = userver::formats::json::FromString("{\"field1\":10,\"field2\":\"100\"}");
  EXPECT_EQ(UniversalValid(json, userver::formats::parse::To<SomeStruct>{}), true);
  EXPECT_EQ(UniversalValid(json2, userver::formats::parse::To<SomeStruct>{}), false);
  EXPECT_EQ(UniversalValid(json3, userver::formats::parse::To<SomeStruct>{}), false);
};

struct SomeStruct2 {
  std::optional<int> field1;
  std::optional<int> field2;
  std::optional<int> field3;
  constexpr bool operator==(const SomeStruct2& other) const noexcept {
    return this->field1 == other.field1 && this->field2 == other.field2 && this->field3 == other.field3;
  };
};

template <>
inline constexpr auto UniversalSerializeLibrary::kSerialization<SomeStruct2> =
    UniversalSerializeLibrary::SerializationConfig<SomeStruct2>::Create()
    .With<"field1">(Configurator<Default<114>>{});


UTEST(Serialize, Optional) {
  using UniversalSerializeLibrary::UniversalSerialize;
  SomeStruct2 a{{}, 100, {}};
  const auto json = UniversalSerialize(a, userver::formats::serialize::To<userver::formats::json::Value>{});
  EXPECT_EQ(json, userver::formats::json::FromString("{\"field1\":114,\"field2\":100}"));
};
UTEST(Parse, Optional) {
  using UniversalSerializeLibrary::UniversalParse;
  constexpr SomeStruct2 valid{{114}, {}, {}};
  const auto json = userver::formats::json::FromString("{}");
  EXPECT_EQ(UniversalParse(json, userver::formats::parse::To<SomeStruct2>{}), valid);
};
UTEST(Valid, Optional) {
  using UniversalSerializeLibrary::UniversalValid;
  const auto json = userver::formats::json::FromString("{}");
  EXPECT_EQ(UniversalValid(json, userver::formats::parse::To<SomeStruct2>{}), true);
};

struct SomeStruct3 {
  std::unordered_map<std::string, int> field;
  auto operator==(const SomeStruct3& other) const noexcept {
    return this->field == other.field;
  };
};
template <>
inline constexpr auto UniversalSerializeLibrary::kSerialization<SomeStruct3> =
    UniversalSerializeLibrary::SerializationConfig<SomeStruct3>::Create()
    .With<"field">(Configurator<Additional>{});

UTEST(Serialize, Additional) {
  using UniversalSerializeLibrary::UniversalSerialize;
  std::unordered_map<std::string, int> value;
  value["data1"] = 1;
  value["data2"] = 2;
  SomeStruct3 a{value};
  const auto json = UniversalSerialize(a, userver::formats::serialize::To<userver::formats::json::Value>{});
  EXPECT_EQ(json, userver::formats::json::FromString("{\"data1\":1,\"data2\":2}"));
};

UTEST(Parse, Additional) {
  using UniversalSerializeLibrary::UniversalParse;
  std::unordered_map<std::string, int> value;
  value["data1"] = 1;
  value["data2"] = 2;
  SomeStruct3 valid{value};
  const auto json = userver::formats::json::FromString("{\"data1\":1,\"data2\":2}");
  const auto fromJson = UniversalParse(json, userver::formats::parse::To<SomeStruct3>{});
  EXPECT_EQ(valid, fromJson);
};

UTEST(Valid, Additional) {
  using UniversalSerializeLibrary::UniversalValid;
  const auto json = userver::formats::json::FromString("{\"data1\":1,\"data2\":2}");
  EXPECT_EQ(UniversalValid(json, userver::formats::parse::To<SomeStruct3>{}), true);
};

struct SomeStruct4 {
  int field;
};

template <>
inline constexpr auto UniversalSerializeLibrary::kSerialization<SomeStruct4> =
    UniversalSerializeLibrary::SerializationConfig<SomeStruct4>::Create()
    .With<"field">(Configurator<Max<120>, Min<10>>{});




UTEST(Valid, MinMax) {
  using UniversalSerializeLibrary::UniversalValid;
  const auto json = userver::formats::json::FromString("{\"field\":1}");
  const auto json2 = userver::formats::json::FromString("{\"field\":11}");
  const auto json3 = userver::formats::json::FromString("{\"field\":121}");

  EXPECT_EQ(UniversalValid(json, userver::formats::parse::To<SomeStruct4>{}), false);
  EXPECT_EQ(UniversalValid(json2, userver::formats::parse::To<SomeStruct4>{}), true);
  EXPECT_EQ(UniversalValid(json3, userver::formats::parse::To<SomeStruct4>{}), false);
};


struct SomeStruct5 {
  std::string field;
};

template <>
inline constexpr auto UniversalSerializeLibrary::kSerialization<SomeStruct5> =
    UniversalSerializeLibrary::SerializationConfig<SomeStruct5>::Create()
    .With<"field">(Configurator<Pattern<"^[0-9]+$">>());

UTEST(Valid, Pattern) {
  using UniversalSerializeLibrary::UniversalValid;
  const auto json = userver::formats::json::FromString(R"({"field":"1234412"})");
  const auto json2 = userver::formats::json::FromString(R"({"field":"abcdefgh"})");
  EXPECT_EQ(UniversalValid(json, userver::formats::parse::To<SomeStruct5>{}), true);
  EXPECT_EQ(UniversalValid(json2, userver::formats::parse::To<SomeStruct5>{}), false);
};



