#pragma once
#include <algorithm>
#include <string>

namespace UniversalSerializeLibrary {

  template <std::size_t Size = 1>
  struct String {
    static constexpr auto kSize = Size;
    std::array<char, Size> contents;
    constexpr operator std::string_view() const {
      return std::string_view{this->contents.data(), Size - 1};
    };
    constexpr auto c_str() const -> const char* {
      return this->contents.data();
    };
    constexpr const char* data() const {
      return this->contents.begin();
    };
    constexpr const char& operator[](size_t index) const {
      return this->contents[index];
    };
    constexpr operator char&() const {
      return this->contents.data();
    };
    constexpr char& operator[](size_t index) {
      return this->contents[index];
    };
    friend constexpr bool operator==(const String& string, const char* other) {
      return std::string_view(string) == std::string_view(other);
    };
    template <typename Stream>
    friend constexpr auto operator<<(Stream& stream, const String& string) {
      stream << std::string_view(string);
      return stream;
    };

    constexpr String(const char (&str)[Size]) noexcept {
      std::copy_n(str, Size, begin(contents));
    };
    constexpr String(std::array<char, Size> data) noexcept : contents(data) {};
    template <std::size_t OtherSize>
    constexpr String<Size + OtherSize> operator+(const String<OtherSize>& other) const noexcept {
      return
          [&]<auto... I, auto... I2>(std::index_sequence<I...>, std::index_sequence<I2...>){
        return String<Size + OtherSize>({this->contents[I]..., other[I2]...});
      }(std::make_index_sequence<Size>(), std::make_index_sequence<OtherSize>());
    };
    template <std::size_t OtherSize>
    constexpr String<Size + OtherSize> operator+(const char(&str)[OtherSize]) const noexcept {
      return *this + String<OtherSize>{str};
    };
  };
};
