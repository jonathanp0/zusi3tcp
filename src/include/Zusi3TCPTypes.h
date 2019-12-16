#pragma once

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace zusi {

enum class Command : uint16_t;

/**
 * @brief Generic Zusi message attribute.
 * Data is owned by the class
 */
class Attribute {
 public:
  //! Construct an empty attribute
  Attribute() : data{}, m_id(0) {}

  /** @brief Constructs an attribute
   * @param id Attribute ID
   */
  explicit Attribute(uint16_t id) : data{}, m_id(id) {}

  template <typename T>
  Attribute(uint16_t id, T value) : data{}, m_id(id) {
    setValue<T>(value);
  }

  Attribute(const Attribute&) = default;
  Attribute(Attribute&&) = default;
  Attribute& operator=(const Attribute& o) = default;

  //! Get Attribute ID
  uint16_t getId() const { return m_id; }

  //! Utility function to set the value
  template <typename T>
  void setValue(T value) {
    data.assign(reinterpret_cast<uint8_t*>(&value), sizeof(T));
  }

  void setValueRaw(const uint8_t* data, size_t length) {
    this->data.assign(data, length);
  }

  bool hasValue() const { return data.length() != 0; }

  template <typename T>
  bool hasValueType() const {
    return sizeof(T) == data.length();
  }

  size_t getValueLen() const { return data.length(); }

  template <typename T>
  T getValue() const {
    return *reinterpret_cast<const T*>(data.c_str());
  }

  const uint8_t* getValueRaw(size_t* len = nullptr) const {
    if (len) {
      *len = data.length();
    }
    return data.c_str();
  }

 private:
  std::basic_string<uint8_t> data;
  uint16_t m_id;
};

template <uint16_t id_, typename NetworkType, typename CPPType = NetworkType>
struct AttribTag {
  using networktype = NetworkType;
  using type = CPPType;
  enum key { id = id_ };
  // constexpr static auto id = id_;

  constexpr AttribTag() {}

  constexpr AttribTag(type val) : value{val} {}

  AttribTag(std::unique_ptr<uint8_t[]>&& data, uint32_t size)
      : value{static_cast<type>(*data.get())} {
    if (sizeof(type) != size) {
      throw std::runtime_error{
          "Invalid conversion of attribute - size does no match"};
    }
  }

  AttribTag(const Attribute& attrib)
      : value{static_cast<type>(attrib.getValue<networktype>())} {
    if (id != attrib.getId()) {
      throw std::runtime_error{"Invalid conversion of attribute"};
    }
  }

  type& operator*() { return value; }
  const type& operator*() const { return value; }

  constexpr operator type() const { return value; }

  constexpr bool operator==(
      const AttribTag<id_, NetworkType, CPPType>& o) const {
    return o.value == value;
  }

  Attribute att() const {
    return Attribute{id, static_cast<NetworkType>(value)};
  }

  operator Attribute() const { return att(); }

 protected:
  type value;
};

template <uint16_t id_>
struct AttribTag<id_, std::string, std::string>
    : public AttribTag<id_, void*, std::string> {
  using parentT = AttribTag<id_, void*, std::string>;

  AttribTag(const Attribute& attrib)
      : parentT{std::string{reinterpret_cast<const char*>(attrib.getValueRaw()),
                            attrib.getValueLen()}} {
    if (id_ != attrib.getId()) {
      throw std::runtime_error{"Invalid conversion of attribute"};
    }
  }

  AttribTag(std::unique_ptr<uint8_t[]>&& data, uint32_t size)
      : AttribTag{std::string{reinterpret_cast<char*>(data.get()), size}} {}

  AttribTag() : parentT{} {}

  AttribTag(const std::string& str) : parentT{str} {}

  Attribute att() const {
    Attribute att{id_};
    att.setValueRaw(reinterpret_cast<const uint8_t*>(parentT::value.c_str()),
                    parentT::value.size());
    return att;
  }

  operator Attribute() const { return att(); }
};

//! Generic Zusi message node
class Node {
 public:
  //! Constructs an empty node
  explicit Node() : m_id(0) {}

  /** @brief Constructs an Node
   * @param id Attribute ID
   */
  explicit Node(uint16_t id) : m_id(id) {}
  explicit Node(Command cmd) : Node(static_cast<uint16_t>(cmd)) {}

  Node(const Node& o) = default;
  Node(Node&&) = default;
  Node& operator=(const Node& o) = default;

  //! Get Attribute ID
  uint16_t getId() const { return m_id; }

  template <typename T>
  typename std::enable_if<std::is_convertible<T, Attribute>::value,
                          std::optional<T>>::type
  get() const noexcept {
    return getImpl<T>(attributes);
  }

  template <typename T>
  typename std::enable_if<!std::is_convertible<T, Attribute>::value,
                          std::optional<T>>::type
  get() const noexcept {
    /* TODO: We should do a positive check here, but currently ComplexNode has
     * nothing trivially checkable without C++17 */
    return getImpl<T>(nodes);
  }

  //!  Attributes of this node
  std::vector<Attribute> attributes;
  //!  Sub-nodes of this node
  std::vector<Node> nodes;

 private:
  uint16_t m_id;

  template <typename T, typename L>
  std::optional<T> getImpl(const L& list) const noexcept {
    const auto& element =
        std::find_if(list.cbegin(), list.cend(),
                     [](const auto& e) { return e.getId() == T::id; });
    if (element == list.cend()) {
      return {};
    }
    return *element;
  }
};

template <uint16_t id_, typename... Atts>
class ComplexNode {
 public:
  // constexpr static auto id = id_;
  enum vals { id = id_ };

 private:
  using AttTupleT = std::tuple<Atts...>;
  AttTupleT atts;

  template <int N>
  void matchAttribute(const Node& /*node*/, std::false_type) {}

  template <int N>
  void matchAttribute(const Node& node, std::true_type) {
    constexpr int id = std::tuple_element<N - 1, AttTupleT>::type::id;
    auto att = std::find_if(
        node.attributes.cbegin(), node.attributes.cend(),
        [=](const Attribute& att) -> bool { return att.getId() == id; });
    if (att != node.attributes.cend()) {
      std::get<N - 1>(atts) = *att;
    } else {
      throw std::runtime_error("Attribute not found");
    }
    matchAttribute<N - 1>(node, std::integral_constant<bool, (N - 1 > 0)>{});
  }

 public:
  ComplexNode() = default;

  ComplexNode(const Node& node) : atts{} {
    if (id_ != node.getId()) {
      throw std::runtime_error{"Invalid conversion of attribute"};
    }

    matchAttribute<std::tuple_size<AttTupleT>::value>(node, std::true_type{});
  }

  template <typename search>
  const search& get() const noexcept {
    return std::get<search>(atts);
  }

  template <typename search>
  search& get() noexcept {
    return std::get<search>(atts);
  }
};
}  // namespace zusi
