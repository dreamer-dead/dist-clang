//===-------------------------- optional ----------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef BASE_OPTIONAL_H_
#define BASE_OPTIONAL_H_

// This is an implementation of std::experimental::optional taken from
// libc++ rev257874 and adjusted to the Chromium codebase. Notable differences:
// 1. For storage we use base::AlignedMemory instead of union class
//    because it's forbidden for now.
// 2. Constructor and emplace lack std::initializer_list because it's
//    forbidden for now.
// 3. value_or is missing && ref-qualifier overload because MSVC 2013 does not
//    support it.
// 4. Instead of std::hash we use hash defined in base/containers/hash_tables.h.
// 5. Instead of using explicit operator bool we use safe bool idiom because
//    explicit operator overloading is forbidden for now.
// 6. Nothing related to exceptions is present because we don't have them.
//    value() methods that ought to throw when optional is unset use CHECKs
//    instead.
// 7. constexpr specifier is omitted because it's forbidden for now.
// 8. Methods are not marked to force inlining and add __visibility__("hidden").
// 9. Explicitly delete operator = in optional_storage.
// Also inheriting the following non-compliances:
// 1. Missing methods
//    constexpr T&& operator *() &&;
//    constexpr const T&& operator *() const &&;
//    constexpr T&& value() &&;
//    constexpr const T&& value() const &&;

#include <base/aliases.h>
#include <base/assert.h>

#include STL(algorithm)
#include STL(functional)
#include STL(utility)
#include STL(type_traits)

#if !defined(BASE_ALIGNOF)
#if defined(_MSC_VER)
#define BASE_ALIGNOF(type) __alignof(type)
#elif
#define BASE_ALIGNOF(type) __alignof__(type)
#endif
#endif  // !defined(BASE_ALIGNOF)

namespace base {

struct in_place_t {};
static in_place_t in_place;

struct nullopt_t {
  explicit nullopt_t(int) {}  // NOLINT
};

static nullopt_t nullopt(0);

namespace internal {

template <class T, bool = std::is_trivially_destructible<T>::value>
class optional_storage {
 protected:
  using value_type = T;

  ~optional_storage() {
    if (is_set_)
      val().~value_type();
  }

  optional_storage() = default;

  optional_storage(const optional_storage& rhs) : is_set_(rhs.is_set_) {
    if (is_set_)
      new (addressof_val()) value_type(rhs.val());
  }

  optional_storage(optional_storage&& rhs) : is_set_(rhs.is_set_) {  // NOLINT
    if (is_set_)
      new (addressof_val()) value_type(std::move(rhs.val()));
  }

  explicit optional_storage(const value_type& rhs) : is_set_(true) {
    new (addressof_val()) value_type(rhs);
  }

  explicit optional_storage(value_type&& rhs) : is_set_(true) {  // NOLINT
    new (addressof_val()) value_type(std::move(rhs));
  }

  template <class... Args>
  explicit optional_storage(in_place_t, Args&&... args)  // NOLINT
      : is_set_(true) {
    new (addressof_val()) value_type(std::forward<Args>(args)...);  // NOLINT
  }

  optional_storage& operator=(const optional_storage&) = delete;
  optional_storage& operator=(optional_storage&&) = delete;

  const value_type& val() const { return *addressof_val(); }

  value_type& val() { return *addressof_val(); }

  const value_type* addressof_val() const {
    return data_.template data_as<value_type>();
  }

  value_type* addressof_val() {
    return data_.template data_as<value_type>();
  }

  bool is_set_ = false;
  std::aligned_storage<sizeof(value_type), BASE_ALIGNOF(value_type)> data_;
};

template <class T>
class optional_storage<T, true> {
 protected:
  using value_type = T;

  optional_storage() = default;

  optional_storage(const optional_storage& rhs) : is_set_(rhs.is_set_) {
    if (is_set_)
      new (addressof_val()) value_type(rhs.val());
  }

  optional_storage(optional_storage&& rhs) : is_set_(rhs.is_set_) {  // NOLINT
    if (is_set_)
      new (addressof_val()) value_type(std::move(rhs.val()));
  }

  explicit optional_storage(const value_type& rhs) : is_set_(true) {
    new (addressof_val()) value_type(rhs);
  }

  explicit optional_storage(value_type&& rhs) : is_set_(true) {  // NOLINT
    new (addressof_val()) value_type(std::move(rhs));
  }

  template <class... Args>
  explicit optional_storage(in_place_t, Args&&... args)  // NOLINT
      : is_set_(true) {
    new (addressof_val()) value_type(std::forward<Args>(args)...);  // NOLINT
  }

  optional_storage& operator=(const optional_storage&) = delete;
  optional_storage& operator=(optional_storage&&) = delete;

  const value_type& val() const { return *addressof_val(); }

  value_type& val() { return *addressof_val(); }

  const value_type* addressof_val() const {
    return data_.template data_as<value_type>();
  }

  value_type* addressof_val() {
    return data_.template data_as<value_type>();
  }

  bool is_set_ = false;
  std::aligned_storage<sizeof(value_type), BASE_ALIGNOF(value_type)> data_;
};

}  // namespace internal

template <class T>
class optional : private internal::optional_storage<T> {
  using __base = internal::optional_storage<T>;

  typedef void (optional::*bool_type)() const;
  void safe_bool() const {}

 public:
  using value_type = T;

  static_assert(
      !std::is_reference<value_type>::value,
      "Instantiation of optional with a reference type is ill-formed.");
  static_assert(
      !std::is_same<typename std::remove_cv<value_type>::type,
                    in_place_t>::value,
      "Instantiation of optional with a in_place_t type is ill-formed.");
  static_assert(
      !std::is_same<typename std::remove_cv<value_type>::type,
                    nullopt_t>::value,
      "Instantiation of optional with a nullopt_t type is ill-formed.");
  static_assert(std::is_object<value_type>::value,
                "Instantiation of optional with a non-object type is undefined "
                "behavior.");

  optional() = default;
  // In MSVC 2013 making this = default generates errors when instantiating
  // from move-only types.
  optional(const optional& rhs) : __base(rhs) {}
  // MSVC 2013 cannot generate move constructors.
  optional(optional&& rhs) : __base(std::move(rhs)) {}  // NOLINT
  ~optional() = default;
  optional(nullopt_t) {}                                      // NOLINT
  optional(const value_type& value) : __base(value) {}        // NOLINT
  optional(value_type&& value) : __base(std::move(value)) {}  // NOLINT

  template <class... Args,
            class = typename std::enable_if<
                std::is_constructible<value_type, Args...>::value>::type>
  explicit optional(in_place_t, Args&&... args)           // NOLINT
      : __base(in_place, std::forward<Args>(args)...) {}  // NOLINT

  // TODO: Initializing via std::initializer_list is missing.

  optional& operator=(nullopt_t) {
    if (this->is_set_) {
      this->val().~value_type();
      this->is_set_ = false;
    }
    return *this;
  }

  optional& operator=(const optional& rhs) {
    if (this->is_set_ == rhs.is_set_) {
      if (this->is_set_)
        this->val() = rhs.val();
    } else {
      if (this->is_set_)
        this->val().~value_type();
      else
        new (this->addressof_val()) value_type(rhs.val());
      this->is_set_ = rhs.is_set_;
    }
    return *this;
  }

  optional& operator=(optional&& rhs) {  // NOLINT
    if (this->is_set_ == rhs.is_set_) {
      if (this->is_set_)
        this->val() = std::move(rhs.val());
    } else {
      if (this->is_set_)
        this->val().~value_type();
      else
        new (this->addressof_val()) value_type(std::move(rhs.val()));
      this->is_set_ = rhs.is_set_;
    }
    return *this;
  }

  template <class U,
            class = typename std::enable_if<
                std::is_same<typename std::remove_reference<U>::type,
                             value_type>::value &&
                std::is_constructible<value_type, U>::value &&
                std::is_assignable<value_type&, U>::value>::type>
  optional& operator=(U&& rhs) {  // NOLINT
    if (this->is_set_) {
      this->val() = std::forward<U>(rhs);  // NOLINT
    } else {
      new (this->addressof_val()) value_type(std::forward<U>(rhs));  // NOLINT
      this->is_set_ = true;
    }
    return *this;
  }

  template <class... Args,
            class = typename std::enable_if<
                std::is_constructible<value_type, Args...>::value>::type>
  void emplace(Args&&... args) {  // NOLINT
    *this = nullopt;
    new (this->addressof_val())
        value_type(std::forward<Args>(args)...);  // NOLINT
    this->is_set_ = true;
  }

  // TODO: emplace with initializer_list is missing.

  void swap(optional& rhs) {
    using std::swap;
    if (this->is_set_ == rhs.is_set_) {
      if (this->is_set_)
        swap(this->val(), rhs.val());
    } else {
      if (this->is_set_) {
        new (rhs.addressof_val()) value_type(std::move(this->val()));
        this->val().~value_type();
      } else {
        new (this->addressof_val()) value_type(std::move(rhs.val()));
        rhs.val().~value_type();
      }
      swap(this->is_set_, rhs.is_set_);
    }
  }

  const value_type* operator->() const {
    DCHECK(this->is_set_);
    return this->addressof_val();
  }

  value_type* operator->() {
    DCHECK(this->is_set_);
    return this->addressof_val();
  }

  const value_type& operator*() const {
    DCHECK(this->is_set_);
    return this->val();
  }

  value_type& operator*() {
    DCHECK(this->is_set_);
    return this->val();
  }

  // TODO: Rewrite to explicit bool when it's available.
  operator bool_type() const {
    return this->is_set_ ? &optional::safe_bool : nullptr;
  }

  const value_type& value() const {
    CHECK(this->is_set_);
    return this->val();
  }

  value_type& value() {
    CHECK(this->is_set_);
    return this->val();
  }

  template <class U>
  value_type value_or(U&& default_value) const & {  // NOLINT
    static_assert(base::is_copy_constructible<value_type>::value,
                  "optional<T>::value_or: T must be copy constructible");
    static_assert(std::is_convertible<U, value_type>::value,
                  "optional<T>::value_or: U must be convertible to T");
    return this->is_set_ ? this->val()
                         : static_cast<value_type>(
                               std::forward<U>(default_value));  // NOLINT
  }

  template <class U>
  value_type value_or(U&& default_value) && {  // NOLINT
    static_assert(base::is_move_constructible<value_type>::value,
                  "optional<T>::value_or: T must be move constructible");
    static_assert(std::is_convertible<U, value_type>::value,
                  "optional<T>::value_or: U must be convertible to T");
    return this->is_set_ ? std::move(this->val())
                         : static_cast<value_type>(
                               std::forward<U>(default_value));  // NOLINT
  }
};

template <class T>
inline bool operator==(const optional<T>& lhs, const optional<T>& rhs) {
  if (static_cast<bool>(lhs) != static_cast<bool>(rhs))
    return false;
  if (!static_cast<bool>(lhs))
    return true;
  return *lhs == *rhs;
}

template <class T>
inline bool operator!=(const optional<T>& lhs, const optional<T>& rhs) {
  return !(lhs == rhs);
}

template <class T>
inline bool operator<(const optional<T>& lhs, const optional<T>& rhs) {
  if (!static_cast<bool>(rhs))
    return false;
  if (!static_cast<bool>(lhs))
    return true;
  return *lhs < *rhs;
}

template <class T>
inline bool operator>(const optional<T>& lhs, const optional<T>& rhs) {
  return rhs < lhs;
}

template <class T>
inline bool operator<=(const optional<T>& lhs, const optional<T>& rhs) {
  return !(rhs < lhs);
}

template <class T>
inline bool operator>=(const optional<T>& lhs, const optional<T>& rhs) {
  return !(lhs < rhs);
}

template <class T>
inline bool operator==(const optional<T>& lhs, nullopt_t rhs) {
  return !static_cast<bool>(lhs);
}

template <class T>
inline bool operator==(nullopt_t lhs, const optional<T>& rhs) {
  return !static_cast<bool>(rhs);
}

template <class T>
inline bool operator!=(const optional<T>& lhs, nullopt_t rhs) {
  return static_cast<bool>(lhs);
}

template <class T>
inline bool operator!=(nullopt_t lhs, const optional<T>& rhs) {
  return static_cast<bool>(rhs);
}

template <class T>
inline bool operator<(const optional<T>& lhs, nullopt_t rhs) {
  return false;
}

template <class T>
inline bool operator<(nullopt_t lhs, const optional<T>& rhs) {
  return static_cast<bool>(rhs);
}

template <class T>
inline bool operator<=(const optional<T>& lhs, nullopt_t rhs) {
  return !static_cast<bool>(lhs);
}

template <class T>
inline bool operator<=(nullopt_t lhs, const optional<T>& rhs) {
  return true;
}

template <class T>
inline bool operator>(const optional<T>& lhs, nullopt_t rhs) {
  return static_cast<bool>(lhs);
}

template <class T>
inline bool operator>(nullopt_t lhs, const optional<T>& rhs) {
  return false;
}

template <class T>
inline bool operator>=(const optional<T>& lhs, nullopt_t rhs) {
  return true;
}

template <class T>
inline bool operator>=(nullopt_t lhs, const optional<T>& rhs) {
  return !static_cast<bool>(rhs);
}

template <class T>
inline bool operator==(const optional<T>& lhs, const T& rhs) {
  return static_cast<bool>(lhs) ? *lhs == rhs : false;
}

template <class T>
inline bool operator==(const T& lhs, const optional<T>& rhs) {
  return static_cast<bool>(rhs) ? lhs == *rhs : false;
}

template <class T>
inline bool operator!=(const optional<T>& lhs, const T& rhs) {
  return static_cast<bool>(lhs) ? !(*lhs == rhs) : true;
}

template <class T>
inline bool operator!=(const T& lhs, const optional<T>& rhs) {
  return static_cast<bool>(rhs) ? !(lhs == *rhs) : true;
}

template <class T>
inline bool operator<(const optional<T>& lhs, const T& rhs) {
  return static_cast<bool>(lhs) ? std::less<T>()(*lhs, rhs) : true;
}

template <class T>
inline bool operator<(const T& lhs, const optional<T>& rhs) {
  return static_cast<bool>(rhs) ? std::less<T>()(lhs, *rhs) : false;
}

template <class T>
inline bool operator<=(const optional<T>& lhs, const T& rhs) {
  return !(lhs > rhs);
}

template <class T>
inline bool operator<=(const T& lhs, const optional<T>& rhs) {
  return !(lhs > rhs);
}

template <class T>
inline bool operator>(const optional<T>& lhs, const T& rhs) {
  return static_cast<bool>(lhs) ? rhs < lhs : false;
}

template <class T>
inline bool operator>(const T& lhs, const optional<T>& rhs) {
  return static_cast<bool>(rhs) ? rhs < lhs : true;
}

template <class T>
inline bool operator>=(const optional<T>& lhs, const T& rhs) {
  return !(lhs < rhs);
}

template <class T>
inline bool operator>=(const T& lhs, const optional<T>& rhs) {
  return !(lhs < rhs);
}

template <class T>
inline void swap(optional<T>& lhs, optional<T>& rhs) {
  lhs.swap(rhs);
}

template <class T>
inline optional<typename std::decay<T>::type> make_optional(
    T&& val) {  // NOLINT
  return optional<typename std::decay<T>::type>(
      std::forward<T>(val));  // NOLINT
}

}  // namespace base

namespace std {

template <class T>
struct hash<base::optional<T>> {
  size_t operator()(const base::optional<T>& optional) const {
    return static_cast<bool>(optional) ? hash<T>()(*optional) : 0;
  }
};

}  // namespace BASE_HASH_NAMESPACE

#endif  // BASE_OPTIONAL_H_
