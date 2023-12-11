#pragma once
#include <userver/formats/json.hpp>
#include <userver/formats/yaml.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <boost/pfr/core.hpp>
#include <boost/pfr/core_name.hpp>
#include <algorithm>
#include <map>
#include "string.hpp"

namespace UniversalSerializeLibrary {

  template <std::size_t N>
  userver::formats::json::Value Serialize(const String<N>& str,
      userver::formats::serialize::To<userver::formats::json::Value>) {
    return userver::formats::json::ValueBuilder(std::string_view(str)).ExtractValue();
  };
  template <std::size_t N>
  userver::formats::yaml::Value Serialize(const String<N>& str,
      userver::formats::serialize::To<userver::formats::yaml::Value>) {
    return userver::formats::yaml::ValueBuilder(std::string_view(str)).ExtractValue();
  };
  template <std::size_t n>
  String(char const (&)[n]) -> String<n>;

  namespace detail {
    struct Disabled {};
  };
  template <typename T>
  inline static constexpr auto kSerialization = detail::Disabled{};

  template <typename T>
  inline static constexpr auto kDeserialization = kSerialization<T>;

  template <typename T>
  inline static constexpr auto kValidation = kSerialization<T>;

  template <typename... T>
  struct Configurator {};

  struct Additional;
  template <auto>
  struct Default;

  namespace detail {

    template <typename T, auto I, typename... Params>
    struct FieldParametries {
      static constexpr auto kIndex = I;
      using Type = T;
      using FieldType = std::remove_cvref_t<decltype(boost::pfr::get<I>(std::declval<T>()))>;
    };

    template <typename... T>
    struct TypeList {};

    struct Caster {
      template <typename T>
      constexpr Caster(const T&){};
    };
    template <auto... I, typename T>
    consteval T get(decltype(Caster(I))..., T, ...);
    template <auto I, typename... Ts>
    consteval auto get(TypeList<Ts...>) {
      return []<auto... Index>(std::index_sequence<Index...>){
        return std::type_identity<decltype(get<Index...>(std::declval<Ts>()...))>();
      }(std::make_index_sequence<I>());
    };
    template <typename... Ts>
    consteval auto size(TypeList<Ts...>) {
      return sizeof...(Ts);
    };

    template <typename T, typename... Ts>
    consteval std::size_t find(TypeList<Ts...>) {
      bool a[] = {std::is_same_v<T, Ts>...};
      return std::find(a, a + sizeof...(Ts), true) - a;
    };
    template <typename T>
    consteval std::size_t find(TypeList<>) {
      return 0;
    };


    template <auto Needed, auto Value, typename T, typename F>
    consteval auto TransformIfEqual(const T& obj, const F& f) {
      if constexpr(Needed == Value) {
        return f(obj);
      } else {
        return obj;
      };
    };

    template <typename Field>
    constexpr inline auto Check(const Field&, Disabled) noexcept {
      return true;
    };
    struct NeverToBeUsed;
    template <typename... Ts>
    struct Error : public std::false_type {};
    template <> // https://eel.is/c++draft/temp#res.general-6.1
    struct Error<NeverToBeUsed, NeverToBeUsed> : public std::true_type {};



    namespace exam {
      template <typename T, typename Format>
      inline bool Is(Format&& json) {
        if constexpr(std::is_convertible_v<T, std::int64_t>) {
          return json.IsInt();
        } else if constexpr(std::is_convertible_v<T, std::string>) {
          return json.IsString();
        } else if constexpr(requires {std::begin(std::declval<T>());}) {
          return json.IsArray();
        } else if constexpr(std::is_convertible_v<bool, T>) {
          return json.IsBool();
        } else {
          return json.IsObject();
        };
      };
      template <typename Field, typename CheckT>
      constexpr inline bool Check(const Field&, CheckT) {
        static_assert(Error<Field, CheckT>::value, "Check Not Found");
        return false;
      };
      template <typename T, auto I, typename Field, typename CheckT>
      constexpr inline const char* ErrorMessage(Field&&, CheckT) {
        static_assert(Error<Field, CheckT>::value, "Error Message Not Found");
        return "";
      };
      template <typename T, auto I, typename Builder, typename Field, typename CheckT>
      constexpr inline auto RunCheckFor(const Builder&, Field&&, CheckT) {
        static_assert(Error<Field, CheckT>::value, "Check Not Found");
      };

      template <typename T, auto I, typename Value, typename Field, typename CheckT>
      constexpr inline auto RunParseCheckFor(const Value&, const Field& field, CheckT check) {
        if(!Check(field, check)) {
          throw std::runtime_error(ErrorMessage<T, I>(field, check));
        };
      };

      template <typename T, auto I, typename... Params, typename Builder, typename Field>
      constexpr inline auto RunWrite(Builder&& builder, const Field& field) {
        builder[std::string(boost::pfr::get_name<I, T>())] = field;
      };
      template <typename T, auto I, typename... Params, typename Builder, typename Field>
      constexpr inline auto RunWrite(Builder&& builder, const std::optional<Field>& field) {
        if(field) {
          builder[std::string(boost::pfr::get_name<I, T>())] = field;
        };
      };
      template <typename T, auto I, typename... Params, typename Format, typename Field>
      constexpr inline auto HasCheck(const Format& format, userver::formats::parse::To<Field>) {
        return format.HasMember(boost::pfr::get_name<I, T>()) && Is<Field>(format[std::string(boost::pfr::get_name<I, T>())]);
      };
      template <typename T, auto I, typename... Params, typename Format, typename Field>
      constexpr inline auto HasCheck(const Format&, userver::formats::parse::To<std::optional<Field>>) {
        return true;
      };

      template <typename T, auto I, typename... Params, typename Value, typename Field>
      constexpr inline Field RunRead(const Value& value, userver::formats::parse::To<Field>) {
        return value[boost::pfr::get_name<I, T>()].template As<Field>();
      };
      template <typename T, auto I, typename... Params, typename Value, typename Field>
      constexpr inline std::optional<Field> RunRead(const Value& value, userver::formats::parse::To<std::optional<Field>>) {
        if(HasCheck<T, I, Params...>(value, userver::formats::parse::To<Field>{})) {
          return RunRead<T, I, Params...>(value, userver::formats::parse::To<Field>{});
        };
        return std::nullopt;
      };
      template <typename T, auto I, typename... Params, typename From, typename Value>
      constexpr inline auto RunRead(const From& value, userver::formats::parse::To<std::unordered_map<std::string, Value>>) {
        constexpr auto names = boost::pfr::names_as_array<T>();
        std::unordered_map<std::string, Value> result;
        for(const auto& [name, value] : userver::formats::common::Items(value)) {
          if(std::find(names.begin(), names.end(), name) == names.end()) {
            result[name] = value.template As<Value>();
          };
        };
        return result;
      };
      template <typename T, auto I, typename Builder, typename Field, auto Value>
      constexpr inline auto RunCheckFor(Builder& builder, const std::optional<Field>& field, Default<Value>) {
        if(!field.has_value()) {
          builder[std::string(boost::pfr::get_name<I, T>())] = Value;
        };
      };

      template <typename T, auto I, typename Value, typename Field, auto DefaultValue>
      constexpr inline auto RunParseCheckFor(const Value&, std::optional<Field>& field, Default<DefaultValue>) {
        if(!field.has_value()) {
          field = DefaultValue;
        };
      };

      template <typename T, auto I, typename... Params, typename Builder, typename Value,
        std::enable_if_t<find<Additional>(TypeList<Params...>()) != sizeof...(Params), std::nullptr_t> = nullptr>
      constexpr inline auto RunWrite(Builder& builder, const std::unordered_map<std::string, Value>& field) {
        for(const auto& element : field) {
          builder[element.first] = element.second;
        };
      };

      template <typename T, auto I, typename Builder, typename Field>
      constexpr inline auto RunCheckFor(Builder&, const Field&, detail::Disabled) noexcept {};
      template <typename T, auto I, typename Builder, typename Field>
      constexpr inline auto RunCheckFor(Builder&, const std::optional<Field>&, detail::Disabled) noexcept {};
      template <typename T, auto I, typename Builder, typename Field, typename CheckT>
      constexpr inline auto RunCheckFor(Builder&, const Field& field, CheckT check) {
        if(!Check(field, check)) {
          throw std::runtime_error(ErrorMessage<T, I>(field, check));
        };
      };

      template <typename T, auto I, typename... Params, typename Format, typename Key, typename Value>
      constexpr inline
      std::enable_if_t<detail::find<Additional>(TypeList<Params...>()) != sizeof...(Params), bool>
      HasCheck(const Format&, userver::formats::parse::To<std::unordered_map<Key, Value>>) {
        return true;
      };
    };

    template <typename T, auto I, typename... Params>
    constexpr inline auto UniversalSerializeField(
         FieldParametries<T, I, Params...>
        ,userver::formats::json::ValueBuilder& builder
        ,const T& obj) {
      const auto& value = boost::pfr::get<I>(obj);
      using exam::RunCheckFor;
      using exam::RunWrite;
      (RunCheckFor<T, I>(builder, value, Params{}), ...);
      RunWrite<T, I, Params...>(builder, value);
    };

    template <typename T, auto I, typename Format, typename... Params>
    constexpr inline auto UniversalParseField(
         FieldParametries<T, I, Params...>
        ,Format&& from) {
      using exam::RunRead;
      using exam::RunParseCheckFor;
      using FieldType = std::remove_cvref_t<decltype(boost::pfr::get<I>(std::declval<T>()))>;
      auto value = RunRead<T, I, Params...>(from, userver::formats::parse::To<FieldType>{});
      (RunParseCheckFor<T, I>(from, value, Params{}), ...);
      return value;
    };

    template <typename T, auto I, typename Format, typename... Params>
    constexpr inline auto UniversalValidField(
         FieldParametries<T, I, Params...>
        ,Format&& from) noexcept {
      using FieldType = std::remove_cvref_t<decltype(boost::pfr::get<I>(std::declval<T>()))>;
      using exam::HasCheck;
      using exam::Check;
      using exam::RunRead;
      return HasCheck<T, I, Params...>(from, userver::formats::parse::To<FieldType>{})
        && (Check(RunRead<T, I, Params...>(from, userver::formats::parse::To<FieldType>{}), Params{}) && ...);
    };


    template <typename Tuple>
    consteval auto find(const Tuple& tuple, auto find) {
      return [&]<auto... I>(std::index_sequence<I...>){
        return ([](const std::string_view a, const std::string_view b, const std::size_t i) -> std::size_t {
          if (a == b) {
            return i;
          };
          return 0;
        }(boost::pfr::get<I>(tuple), find, I) + ...);
      }(std::make_index_sequence<boost::pfr::tuple_size_v<Tuple>>());
    };
  };

  template <typename T, typename... Params>
  class SerializationConfig {
    private:
      template <auto I, typename Param>
      static consteval auto AddParamTo() {
        return []<auto... Is>(std::index_sequence<Is...>){
          return SerializationConfig<T, decltype(
            detail::TransformIfEqual<I, Is>(Params{},
                []<typename... FieldParams>(detail::FieldParametries<T, I, FieldParams...>){
              return detail::FieldParametries<T, I, FieldParams..., Param>();
            })
          )...>();
        }(std::make_index_sequence<sizeof...(Params)>());
      };

    public:
      static consteval auto Create() {
        return []<auto... I>(std::index_sequence<I...>){
          return SerializationConfig<T, detail::FieldParametries<T, I>...>{};
        }(std::make_index_sequence<boost::pfr::tuple_size_v<T>>());
      };

      template <std::size_t I, typename... ConfigElements>
      consteval auto With(Configurator<ConfigElements...>) const {
        return SerializationConfig<T, Params...>();
      };

      template <std::size_t I, typename ConfigElement, typename... ConfigElements>
      consteval auto With(Configurator<ConfigElement, ConfigElements...>) const {
        return SerializationConfig<T, Params...>::AddParamTo<I, ConfigElement>().template With<I>(Configurator<ConfigElements...>());
      };

      template <String field, typename... ConfigElements>
      consteval auto With(Configurator<ConfigElements...> config) const {
        return With<detail::find(boost::pfr::names_as_array<T>(), field)>(config);
      };

      constexpr SerializationConfig() noexcept {
        static_assert(sizeof...(Params) == boost::pfr::tuple_size_v<T>, "Use Create");
      };
  };

  template <typename T, std::enable_if_t<!std::is_same_v<decltype(kSerialization<std::remove_cvref_t<T>>), const detail::Disabled>, std::nullptr_t> = nullptr>
  inline userver::formats::json::Value UniversalSerialize(T&& obj,
      userver::formats::serialize::To<userver::formats::json::Value>) {
    using Config = std::remove_const_t<decltype(kSerialization<std::remove_cvref_t<T>>)>;
    using Type = std::remove_cvref_t<T>;
    return [&]<typename... Params>
        (SerializationConfig<Type, Params...>){
      userver::formats::json::ValueBuilder builder;
      (detail::UniversalSerializeField(Params{}, builder, obj), ...);
      return builder.ExtractValue();
    }(Config{});
  };
  template <typename T, std::enable_if_t<!std::is_same_v<decltype(kSerialization<std::remove_cvref_t<T>>), const detail::Disabled>, std::nullptr_t> = nullptr>
  inline userver::formats::yaml::Value UniversalSerialize(T&& obj,
      userver::formats::serialize::To<userver::formats::yaml::Value>) {
    using Config = std::remove_const_t<decltype(kSerialization<std::remove_cvref_t<T>>)>;
    using Type = std::remove_cvref_t<T>;
    return [&]<typename... Params>
        (SerializationConfig<Type, Params...>){
      userver::formats::yaml::ValueBuilder builder;
      (detail::UniversalSerializeField(Params{}, builder, obj), ...);
      return builder.ExtractValue();
    }(Config{});
  };
  template <typename Format, typename T, std::enable_if_t<!std::is_same_v<decltype(kDeserialization<std::remove_cvref_t<T>>), const detail::Disabled>, std::nullptr_t> = nullptr>
  inline auto UniversalParse(Format&& from,
      userver::formats::parse::To<T>) {
    using Config = std::remove_const_t<decltype(kDeserialization<std::remove_cvref_t<T>>)>;
    using Type = std::remove_cvref_t<T>;
    return [&]<typename... Params>(SerializationConfig<Type, Params...>){
      return T{detail::UniversalParseField(Params{}, from)...};
    }(Config{});
  };
  template <typename Format, typename T, std::enable_if_t<!std::is_same_v<decltype(kValidation<std::remove_cvref_t<T>>), const detail::Disabled>, std::nullptr_t> = nullptr>
  inline bool UniversalValid(Format&& from,
      userver::formats::parse::To<T>) {
    using Config = std::remove_const_t<decltype(kValidation<std::remove_cvref_t<T>>)>;
    using Type = std::remove_cvref_t<T>;
    return [&]<typename... Params>(SerializationConfig<Type, Params...>){
      return (detail::UniversalValidField(Params{}, from) && ...);
    }(Config{});
  };
};
