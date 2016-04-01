//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <base/aliases.h>
#include <base/optional.h>

// What follows is an assortment of tests adapted from libc++ with exclusion of:
// 1. Testing constexpr.
// 2. Testing exceptions.
// 3. Testing general failures (e.g. operator* of unset optional).
// 4. Testing compilation failures (e.g. ill-formed types like optional<void>).
// Each test is prefixed by a path at which you can find it's original in libc++
// source tree. Common root is test/std/experimental/optional.

#include STL(algorithm)
#include STL(memory)
#include STL(string)
#include STL(type_traits)

#include <third_party/gtest/exported/include/gtest/gmock.h>
#include <third_party/gtest/exported/include/gtest/gtest.h>

using ::testing::_;
using ::testing::StrictMock;

namespace {

struct EmptyType {};

class CustomPair {
 public:
  CustomPair() : left_(0) {}
  // implicit
  CustomPair(int left) : left_(left) {}  // NOLINT
  CustomPair(int left, int right) : left_(left), right_(right) {}

  friend bool operator==(const CustomPair& lhs, const CustomPair& rhs) {
    return lhs.left_ == rhs.left_ && lhs.right_ == rhs.right_;
  }

 private:
  int left_;
  int right_ = 0;
};

struct CustomIntWrapper {
  CustomIntWrapper(int i) : i_(i) {}  // NOLINT

  int i_;
};

struct ConvertibleFromCustomIntWrapperWithMove {
  ConvertibleFromCustomIntWrapperWithMove(int i) : i_(i) {}  // NOLINT
  ConvertibleFromCustomIntWrapperWithMove(
      ConvertibleFromCustomIntWrapperWithMove&& x)  // NOLINT
      : i_(x.i_) {
    x.i_ = 0;
  }
  ConvertibleFromCustomIntWrapperWithMove(const CustomIntWrapper& y)  // NOLINT
      : i_(y.i_) {}
  ConvertibleFromCustomIntWrapperWithMove(CustomIntWrapper&& y)  // NOLINT
      : i_(y.i_ + 1) {}

  int i_;
};

bool operator==(const ConvertibleFromCustomIntWrapperWithMove& lhs,
                const ConvertibleFromCustomIntWrapperWithMove& rhs) {
  return lhs.i_ == rhs.i_;
}

struct ConvertibleFromCustomIntWrapperWithCopy {
  ConvertibleFromCustomIntWrapperWithCopy(int i) : i_(i) {}           // NOLINT
  ConvertibleFromCustomIntWrapperWithCopy(const CustomIntWrapper& y)  // NOLINT
      : i_(y.i_) {}
  ConvertibleFromCustomIntWrapperWithCopy(CustomIntWrapper&& y)  // NOLINT
      : i_(y.i_ + 1) {}

  int i_;
};

bool operator==(const ConvertibleFromCustomIntWrapperWithCopy& lhs,
                const ConvertibleFromCustomIntWrapperWithCopy& rhs) {
  return lhs.i_ == rhs.i_;
}

class Object;

class ObjectRecorder {
 public:
  MOCK_METHOD2(Ctor, void(Object*, int));
  MOCK_METHOD2(CopyCtor, void(Object*, const Object&));
  MOCK_METHOD2(CopyAssignment, void(Object*, const Object&));
  MOCK_METHOD2(MoveCtor, void(Object*, const Object&));
  MOCK_METHOD2(MoveAssignment, void(Object*, const Object&));
  MOCK_METHOD2(Dtor, void(Object*, int));
};

StrictMock<ObjectRecorder>* g_object_recorder = nullptr;

// This together with Object and g_object_recorder
// allows you to check for ctor and dtor calls of underlying Object
// in base::optional<Object>.
class ScopedObjectRecorder : public StrictMock<ObjectRecorder> {
 public:
  ScopedObjectRecorder() {
    DCHECK(!g_object_recorder);
    g_object_recorder = this;
  }

  ~ScopedObjectRecorder() {
    DCHECK_EQ(this, g_object_recorder);
    g_object_recorder = nullptr;
  }

 private:
};

class Object {
 public:
  Object() = delete;

  explicit Object(int value) : value_(value) {
    if (g_object_recorder)
      g_object_recorder->Ctor(this, value_);
  }

  Object(const Object& rhs) : value_(rhs.value_) {
    if (g_object_recorder)
      g_object_recorder->CopyCtor(this, rhs);
  }

  Object(Object&& rhs) : value_(rhs.value_) {  // NOLINT
    if (g_object_recorder)
      g_object_recorder->MoveCtor(this, rhs);
  }

  Object& operator=(const Object& rhs) {
    value_ = rhs.value_;
    if (g_object_recorder)
      g_object_recorder->CopyAssignment(this, rhs);
    return *this;
  }

  Object& operator=(Object&& rhs) {  // NOLINT
    value_ = rhs.value_;
    if (g_object_recorder)
      g_object_recorder->MoveAssignment(this, rhs);
    return *this;
  }

  virtual ~Object() {
    if (g_object_recorder)
      g_object_recorder->Dtor(this, value_);
  }

  int value() const { return value_; }

 protected:
  int value_;
};

bool operator==(const Object& lhs, const Object& rhs) {
  return lhs.value() == rhs.value();
}

bool operator<(const Object& lhs, const Object& rhs) {
  return lhs.value() < rhs.value();
}

class ObjectWithSwap : private Object {
 public:
  ObjectWithSwap() = default;
  explicit ObjectWithSwap(int value) : Object(value) {}
  ObjectWithSwap(const ObjectWithSwap&) = default;
  // MSVC 2013 cannot generate move constructors.
  ObjectWithSwap(ObjectWithSwap&& rhs) : Object(std::move(rhs)) {}  // NOLINT
  ObjectWithSwap& operator=(const ObjectWithSwap&) = default;
  // MSVC 2013 cannot generate move assignment operators.
  ObjectWithSwap& operator=(ObjectWithSwap&& rhs) {  // NOLINT
    Object::operator=(std::move(rhs));
    return *this;
  }
  ~ObjectWithSwap() override = default;

  using Object::value;

  friend void swap(ObjectWithSwap& lhs, ObjectWithSwap& rhs) {
    using std::swap;
    swap(lhs.value_, rhs.value_);
  }
};

int in_place_t_test(const base::in_place_t&) {
  return 3;
}

int nullopt_t_test(const base::nullopt_t&) {
  return 3;
}

}  // namespace

// optional.comp_with_t/equal.pass.cpp
TEST(OptionalTest, EqualWithT) {
  Object val(2);
  base::optional<Object> o1;
  base::optional<Object> o2(Object(1));
  base::optional<Object> o3(val);

  EXPECT_FALSE(o1 == Object(1));
  EXPECT_TRUE(o2 == Object(1));
  EXPECT_FALSE(o3 == Object(1));
  EXPECT_TRUE(o3 == Object(2));
  EXPECT_TRUE(o3 == val);

  EXPECT_FALSE(Object(1) == o1);
  EXPECT_TRUE(Object(1) == o2);
  EXPECT_FALSE(Object(1) == o3);
  EXPECT_TRUE(Object(2) == o3);
  EXPECT_TRUE(val == o3);
}

// optional.comp_with_t/greater.pass.cpp
TEST(OptionalTest, GreaterWithT) {
  Object val(2);
  base::optional<Object> o1;
  base::optional<Object> o2(Object(1));
  base::optional<Object> o3(val);

  EXPECT_FALSE(o1 > Object(1));
  EXPECT_FALSE(o2 > Object(1));
  EXPECT_TRUE(o3 > Object(1));
  EXPECT_FALSE(o2 > val);
  EXPECT_FALSE(o3 > val);
  EXPECT_FALSE(o3 > Object(3));

  EXPECT_TRUE(Object(1) > o1);
  EXPECT_FALSE(Object(1) > o2);
  EXPECT_FALSE(Object(1) > o3);
  EXPECT_TRUE(val > o2);
  EXPECT_FALSE(val > o3);
  EXPECT_TRUE(Object(3) > o3);
}

// optional.comp_with_t/greater_equal.pass.cpp
TEST(OptionalTest, GreaterEqualWithT) {
  Object val(2);
  base::optional<Object> o1;
  base::optional<Object> o2(Object(1));
  base::optional<Object> o3(val);

  EXPECT_FALSE(o1 >= Object(1));
  EXPECT_TRUE(o2 >= Object(1));
  EXPECT_TRUE(o3 >= Object(1));
  EXPECT_FALSE(o2 >= val);
  EXPECT_TRUE(o3 >= val);
  EXPECT_FALSE(o3 >= Object(3));

  EXPECT_TRUE(Object(1) >= o1);
  EXPECT_TRUE(Object(1) >= o2);
  EXPECT_FALSE(Object(1) >= o3);
  EXPECT_TRUE(val >= o2);
  EXPECT_TRUE(val >= o3);
  EXPECT_TRUE(Object(3) >= o3);
}

// optional.comp_with_t/less_equal.pass.cpp
TEST(OptionalTest, LessEqualWithT) {
  Object val(2);
  base::optional<Object> o1;
  base::optional<Object> o2(Object(1));
  base::optional<Object> o3(val);

  EXPECT_TRUE(o1 <= Object(1));
  EXPECT_TRUE(o2 <= Object(1));
  EXPECT_FALSE(o3 <= Object(1));
  EXPECT_TRUE(o2 <= val);
  EXPECT_TRUE(o3 <= val);
  EXPECT_TRUE(o3 <= Object(3));

  EXPECT_FALSE(Object(1) <= o1);
  EXPECT_TRUE(Object(1) <= o2);
  EXPECT_TRUE(Object(1) <= o3);
  EXPECT_FALSE(val <= o2);
  EXPECT_TRUE(val <= o3);
  EXPECT_FALSE(Object(3) <= o3);
}

// optional.comp_with_t/less_than.pass.cpp
TEST(OptionalTest, LessThanWithT) {
  Object val(2);
  base::optional<Object> o1;
  base::optional<Object> o2(Object(1));
  base::optional<Object> o3(val);

  EXPECT_TRUE(o1 < Object(1));
  EXPECT_FALSE(o2 < Object(1));
  EXPECT_FALSE(o3 < Object(1));
  EXPECT_TRUE(o2 < val);
  EXPECT_FALSE(o3 < val);
  EXPECT_TRUE(o3 < Object(3));

  EXPECT_FALSE(Object(1) < o1);
  EXPECT_FALSE(Object(1) < o2);
  EXPECT_TRUE(Object(1) < o3);
  EXPECT_FALSE(val < o2);
  EXPECT_FALSE(val < o3);
  EXPECT_FALSE(Object(3) < o3);
}

// optional.comp_with_t/not_equal.pass.cpp
TEST(OptionalTest, NotEqualWithT) {
  Object val(2);
  base::optional<Object> o1;
  base::optional<Object> o2(Object(1));
  base::optional<Object> o3(val);

  EXPECT_TRUE(o1 != Object(1));
  EXPECT_FALSE(o2 != Object(1));
  EXPECT_TRUE(o3 != Object(1));
  EXPECT_FALSE(o3 != Object(2));
  EXPECT_FALSE(o3 != val);

  EXPECT_TRUE(Object(1) != o1);
  EXPECT_FALSE(Object(1) != o2);
  EXPECT_TRUE(Object(1) != o3);
  EXPECT_FALSE(Object(2) != o3);
  EXPECT_FALSE(val != o3);
}

// optional.hash/hash.pass.cpp
TEST(OptionalTest, HashInt) {
  using BASE_HASH_NAMESPACE::hash;
  typedef int T;
  base::optional<T> opt;
  EXPECT_EQ(0ul, hash<base::optional<T>>()(opt));
  opt = 2;
  EXPECT_EQ(hash<T>()(*opt), hash<base::optional<T>>()(opt));
}

// optional.hash/hash.pass.cpp
TEST(OptionalTest, HashString) {
  using BASE_HASH_NAMESPACE::hash;
  typedef std::string T;
  base::optional<T> opt;
  EXPECT_EQ(0ul, hash<base::optional<T>>()(opt));
  opt = std::string("123");
  EXPECT_EQ(hash<T>()(*opt), hash<base::optional<T>>()(opt));
}

// optional.inplace/in_place_t.pass.cpp
TEST(OptionalTest, InPlaceT) {
  EXPECT_TRUE(std::is_class<base::in_place_t>::value);
  EXPECT_TRUE(std::is_empty<base::in_place_t>::value);
  EXPECT_EQ(3, in_place_t_test(base::in_place));
}

// optional.nullops/equal.pass.cpp
TEST(OptionalTest, EqualWithNullopt) {
  base::optional<int> o1;
  base::optional<int> o2(1);

  EXPECT_TRUE(base::nullopt == o1);
  EXPECT_FALSE(base::nullopt == o2);
  EXPECT_TRUE(o1 == base::nullopt);
  EXPECT_FALSE(o2 == base::nullopt);
}

// optional.nullops/greater.pass.cpp
TEST(OptionalTest, GreaterWithNullopt) {
  base::optional<int> o1;
  base::optional<int> o2(1);

  EXPECT_FALSE(base::nullopt > o1);
  EXPECT_FALSE(base::nullopt > o2);
  EXPECT_FALSE(o1 > base::nullopt);
  EXPECT_TRUE(o2 > base::nullopt);
}

// optional.nullops/greater_equal.pass.cpp
TEST(OptionalTest, GreaterEqualWithNullopt) {
  base::optional<int> o1;
  base::optional<int> o2(1);

  EXPECT_TRUE(base::nullopt >= o1);
  EXPECT_FALSE(base::nullopt >= o2);
  EXPECT_TRUE(o1 >= base::nullopt);
  EXPECT_TRUE(o2 >= base::nullopt);
}

// optional.nullops/less_equal.pass.cpp
TEST(OptionalTest, LessEqualWithNullopt) {
  base::optional<int> o1;
  base::optional<int> o2(1);

  EXPECT_TRUE(base::nullopt <= o1);
  EXPECT_TRUE(base::nullopt <= o2);
  EXPECT_TRUE(o1 <= base::nullopt);
  EXPECT_FALSE(o2 <= base::nullopt);
}

// optional.nullops/less_than.pass.cpp
TEST(OptionalTest, LessThanWithNullopt) {
  base::optional<int> o1;
  base::optional<int> o2(1);

  EXPECT_FALSE(base::nullopt < o1);
  EXPECT_TRUE(base::nullopt < o2);
  EXPECT_FALSE(o1 < base::nullopt);
  EXPECT_FALSE(o2 < base::nullopt);
}

// optional.nullops/not_equal.pass.cpp
TEST(OptionalTest, NotEqualWithNullopt) {
  base::optional<int> o1;
  base::optional<int> o2(1);

  EXPECT_FALSE(base::nullopt != o1);
  EXPECT_TRUE(base::nullopt != o2);
  EXPECT_FALSE(o1 != base::nullopt);
  EXPECT_TRUE(o2 != base::nullopt);
}

// optional.nullopt/nullopt_t.pass.cpp
TEST(OptionalTest, NulloptT) {
  EXPECT_TRUE(std::is_class<base::nullopt_t>::value);
  EXPECT_TRUE(std::is_empty<base::nullopt_t>::value);
  // TODO: base::nullopt_t is required to have at least one
  // constexpr contructor for this to work. And constexpr is forbidden for
  // now.
  // EXPECT_TRUE(std::is_literal_type<base::nullopt_t>::value);
  EXPECT_FALSE(base::is_default_constructible<base::nullopt_t>::value);

  EXPECT_EQ(3, nullopt_t_test(base::nullopt));
}

// optional.object/optional.object.assign/assign_value.pass.cpp
TEST(OptionalTest, AssignValue) {
  EXPECT_TRUE((base::is_assignable<base::optional<int>, int>::value));
  EXPECT_TRUE((base::is_assignable<base::optional<int>, int&>::value));
  EXPECT_TRUE((base::is_assignable<base::optional<int>&, int>::value));
  EXPECT_TRUE((base::is_assignable<base::optional<int>&, int&>::value));
  EXPECT_TRUE((base::is_assignable<base::optional<int>&, const int&>::value));
  EXPECT_FALSE(
      (base::is_assignable<const base::optional<int>&, const int&>::value));
  EXPECT_FALSE((base::is_assignable<base::optional<int>, EmptyType>::value));
}

// optional.object/optional.object.assign/assign_value.pass.cpp
TEST(OptionalTest, AssignValueIntLiteral) {
  base::optional<int> opt;
  opt = 1;
  EXPECT_TRUE(static_cast<bool>(opt));
  EXPECT_EQ(1, *opt);
}

// optional.object/optional.object.assign/assign_value.pass.cpp
TEST(OptionalTest, AssignValueIntVariable) {
  base::optional<int> opt;
  const int i = 2;
  opt = i;
  EXPECT_TRUE(static_cast<bool>(opt));
  EXPECT_EQ(i, *opt);
}

// optional.object/optional.object.assign/assign_value.pass.cpp
TEST(OptionalTest, AssignValueIntVariableToSet) {
  base::optional<int> opt(3);
  const int i = 2;
  opt = i;
  EXPECT_TRUE(static_cast<bool>(opt));
  EXPECT_EQ(i, *opt);
}

// optional.object/optional.object.assign/assign_value.pass.cpp
TEST(OptionalTest, AssignValueUniquePtr) {
  base::optional<std::unique_ptr<int>> opt;
  opt = std::unique_ptr<int>(new int(3));
  EXPECT_TRUE(static_cast<bool>(opt));
  EXPECT_EQ(3, **opt);
}

// optional.object/optional.object.assign/assign_value.pass.cpp
TEST(OptionalTest, AssignValueUniquePtrToSet) {
  base::optional<std::unique_ptr<int>> opt(std::unique_ptr<int>(new int(2)));
  opt = std::unique_ptr<int>(new int(3));
  EXPECT_TRUE(static_cast<bool>(opt));
  EXPECT_EQ(3, **opt);
}

// optional.object/optional.object.assign/copy.pass.cpp
TEST(OptionalTest, CopyAssignmentUnsetFromUnset) {
  base::optional<int> opt;
  base::optional<int> opt2;
  opt = opt2;
  EXPECT_FALSE(static_cast<bool>(opt2));
  EXPECT_EQ(static_cast<bool>(opt2), static_cast<bool>(opt));
}

// optional.object/optional.object.assign/copy.pass.cpp
TEST(OptionalTest, CopyAssignmentUnsetFromSet) {
  base::optional<int> opt;
  base::optional<int> opt2(2);
  opt = opt2;
  EXPECT_TRUE(static_cast<bool>(opt2));
  EXPECT_EQ(2, *opt2);
  EXPECT_EQ(static_cast<bool>(opt2), static_cast<bool>(opt));
  EXPECT_EQ(*opt2, *opt);
}

// optional.object/optional.object.assign/copy.pass.cpp
TEST(OptionalTest, CopyAssignmentSetFromUnset) {
  base::optional<int> opt(3);
  base::optional<int> opt2;
  opt = opt2;
  EXPECT_FALSE(static_cast<bool>(opt2));
  EXPECT_EQ(static_cast<bool>(opt2), static_cast<bool>(opt));
}

// optional.object/optional.object.assign/copy.pass.cpp
TEST(OptionalTest, CopyAssignmentSetFromSet) {
  base::optional<int> opt(3);
  base::optional<int> opt2(2);
  opt = opt2;
  EXPECT_TRUE(static_cast<bool>(opt2));
  EXPECT_EQ(2, *opt2);
  EXPECT_EQ(static_cast<bool>(opt2), static_cast<bool>(opt));
  EXPECT_EQ(*opt2, *opt);
}

// optional.object/optional.object.assign/emplace.pass.cpp
TEST(OptionalTest, EmplaceIntDefaultWithUnset) {
  base::optional<int> opt;
  opt.emplace();
  EXPECT_TRUE(static_cast<bool>(opt));
  EXPECT_EQ(0, *opt);
}

// optional.object/optional.object.assign/emplace.pass.cpp
TEST(OptionalTest, EmplaceIntWithUnset) {
  base::optional<int> opt;
  opt.emplace(1);
  EXPECT_TRUE(static_cast<bool>(opt));
  EXPECT_EQ(1, *opt);
}

// optional.object/optional.object.assign/emplace.pass.cpp
TEST(OptionalTest, EmplaceIntDefaultWithSet) {
  base::optional<int> opt(2);
  opt.emplace();
  EXPECT_TRUE(static_cast<bool>(opt));
  EXPECT_EQ(0, *opt);
}

// optional.object/optional.object.assign/emplace.pass.cpp
TEST(OptionalTest, EmplaceIntWithSet) {
  base::optional<int> opt(2);
  opt.emplace(1);
  EXPECT_TRUE(static_cast<bool>(opt));
  EXPECT_EQ(1, *opt);
}

// optional.object/optional.object.assign/emplace.pass.cpp
TEST(OptionalTest, EmplaceCustomPairDefaultWithUnset) {
  base::optional<CustomPair> opt;
  opt.emplace();
  EXPECT_TRUE(static_cast<bool>(opt));
  EXPECT_EQ(CustomPair(), *opt);
}

// optional.object/optional.object.assign/emplace.pass.cpp
TEST(OptionalTest, EmplaceCustomPairOneWithUnset) {
  base::optional<CustomPair> opt;
  opt.emplace(1);
  EXPECT_TRUE(static_cast<bool>(opt));
  EXPECT_EQ(CustomPair(1), *opt);
}

// optional.object/optional.object.assign/emplace.pass.cpp
TEST(OptionalTest, EmplaceCustomPairTwoWithUnset) {
  base::optional<CustomPair> opt;
  opt.emplace(1, 2);
  EXPECT_TRUE(static_cast<bool>(opt));
  EXPECT_EQ(CustomPair(1, 2), *opt);
}

// optional.object/optional.object.assign/emplace.pass.cpp
TEST(OptionalTest, EmplaceCustomPairDefaultWithSet) {
  base::optional<CustomPair> opt(CustomPair(3));
  opt.emplace();
  EXPECT_TRUE(static_cast<bool>(opt));
  EXPECT_EQ(CustomPair(), *opt);
}

// optional.object/optional.object.assign/emplace.pass.cpp
TEST(OptionalTest, EmplaceCustomPairOneWithSet) {
  base::optional<CustomPair> opt(CustomPair(3));
  opt.emplace(1);
  EXPECT_TRUE(static_cast<bool>(opt));
  EXPECT_EQ(CustomPair(1), *opt);
}

// optional.object/optional.object.assign/emplace.pass.cpp
TEST(OptionalTest, EmplaceCustomPairTwoWithSet) {
  base::optional<CustomPair> opt(CustomPair(3));
  opt.emplace(1, 2);
  EXPECT_TRUE(static_cast<bool>(opt));
  EXPECT_EQ(CustomPair(1, 2), *opt);
}

// optional.object/optional.object.assign/emplace.pass.cpp
TEST(OptionalTest, Emplace) {
  Object obj(0);
  base::optional<Object> opt(obj);
  {
    ScopedObjectRecorder recorder;
    EXPECT_CALL(recorder, Dtor(_, _));
    EXPECT_CALL(recorder, Ctor(_, _));
    opt.emplace(1);
  }
}

// optional.object/optional.object.assign/move.pass.cpp
TEST(OptionalTest, MoveAssignmentUnsetFromUnset) {
  base::optional<int> opt;
  base::optional<int> opt2;
  opt = std::move(opt2);
  EXPECT_FALSE(static_cast<bool>(opt2));
  EXPECT_EQ(static_cast<bool>(opt2), static_cast<bool>(opt));
}

// optional.object/optional.object.assign/move.pass.cpp
TEST(OptionalTest, MoveAssignmentUnsetFromSet) {
  base::optional<int> opt;
  base::optional<int> opt2(2);
  opt = std::move(opt2);
  EXPECT_TRUE(static_cast<bool>(opt2));
  EXPECT_EQ(2, *opt2);
  EXPECT_EQ(static_cast<bool>(opt2), static_cast<bool>(opt));
  EXPECT_EQ(*opt2, *opt);
}

// optional.object/optional.object.assign/move.pass.cpp
TEST(OptionalTest, MoveAssignmentSetFromUnset) {
  base::optional<int> opt(3);
  base::optional<int> opt2;
  opt = std::move(opt2);
  EXPECT_FALSE(static_cast<bool>(opt2));
  EXPECT_EQ(static_cast<bool>(opt2), static_cast<bool>(opt));
}

// optional.object/optional.object.assign/move.pass.cpp
TEST(OptionalTest, MoveAssignmentSetFromSet) {
  base::optional<int> opt(3);
  base::optional<int> opt2(2);
  opt = std::move(opt2);
  EXPECT_TRUE(static_cast<bool>(opt2));
  EXPECT_EQ(2, *opt2);
  EXPECT_EQ(static_cast<bool>(opt2), static_cast<bool>(opt));
  EXPECT_EQ(*opt2, *opt);
}

// optional.object/optional.object.assign/nullopt_t.pass.cpp
TEST(OptionalTest, AssignmentUnsetFromNulloptT) {
  base::optional<int> opt;
  opt = base::nullopt;
  EXPECT_FALSE(static_cast<bool>(opt));
}

// optional.object/optional.object.assign/nullopt_t.pass.cpp
TEST(OptionalTest, AssignmentSetFromNulloptT) {
  base::optional<int> opt(3);
  opt = base::nullopt;
  EXPECT_FALSE(static_cast<bool>(opt));
}

// optional.object/optional.object.assign/nullopt_t.pass.cpp
TEST(OptionalTest, AssignmentUnsetFromNulloptTNoDtorCall) {
  base::optional<Object> opt;
  {
    ScopedObjectRecorder recorder;
    opt = base::nullopt;
    // Expecting nothing in recorder to fire;
  }
  EXPECT_FALSE(static_cast<bool>(opt));
}

// optional.object/optional.object.assign/nullopt_t.pass.cpp
TEST(OptionalTest, AssignmentSetFromNulloptTDtorCall) {
  Object obj(0);
  base::optional<Object> opt(obj);
  {
    ScopedObjectRecorder recorder;
    EXPECT_CALL(recorder, Dtor(_, _));
    opt = base::nullopt;
  }
  EXPECT_FALSE(static_cast<bool>(opt));
}

// optional.object/optional.object.ctor/const_T.pass.cpp
TEST(OptionalTest, CtorConstTInt) {
  int t(5);
  base::optional<int> opt(t);
  EXPECT_TRUE(static_cast<bool>(opt));
  EXPECT_EQ(5, *opt);
}

// optional.object/optional.object.ctor/const_T.pass.cpp
TEST(OptionalTest, CtorConstTDouble) {
  double t(3);
  base::optional<double> opt(t);
  EXPECT_TRUE(static_cast<bool>(opt));
  EXPECT_EQ(3, *opt);
}

// optional.object/optional.object.ctor/const_T.pass.cpp
TEST(OptionalTest, CtorConstTObject) {
  const Object t(3);
  base::optional<Object> opt(t);
  EXPECT_TRUE(static_cast<bool>(opt));
  EXPECT_EQ(Object(3), *opt);
}

// optional.object/optional.object.ctor/copy.pass.cpp
TEST(OptionalTest, CtorCopyIntFromUnset) {
  base::optional<int> rhs;
  base::optional<int> lhs = rhs;
  EXPECT_FALSE(static_cast<bool>(lhs));
}

// optional.object/optional.object.ctor/copy.pass.cpp
TEST(OptionalTest, CtorCopyIntFromSet) {
  base::optional<int> rhs(3);
  base::optional<int> lhs = rhs;
  EXPECT_TRUE(static_cast<bool>(lhs));
  EXPECT_EQ(*rhs, *lhs);
}

// optional.object/optional.object.ctor/copy.pass.cpp
TEST(OptionalTest, CtorCopyObjectFromUnset) {
  base::optional<Object> rhs;
  base::optional<Object> lhs = rhs;
  EXPECT_FALSE(static_cast<bool>(lhs));
}

// optional.object/optional.object.ctor/copy.pass.cpp
TEST(OptionalTest, CtorCopyObjectFromSet) {
  base::optional<Object> rhs(Object(3));
  base::optional<Object> lhs = rhs;
  EXPECT_TRUE(static_cast<bool>(lhs));
  EXPECT_EQ(*rhs, *lhs);
}

// optional.object/optional.object.ctor/default.pass.cpp
TEST(OptionalTest, CtorDefaultInt) {
  base::optional<int> opt;
  EXPECT_FALSE(static_cast<bool>(opt));
}

// optional.object/optional.object.ctor/default.pass.cpp
TEST(OptionalTest, CtorDefaultIntPtr) {
  base::optional<int*> opt;
  EXPECT_FALSE(static_cast<bool>(opt));
}

// optional.object/optional.object.ctor/default.pass.cpp
TEST(OptionalTest, CtorDefaultObject) {
  base::optional<Object> opt;
  EXPECT_FALSE(static_cast<bool>(opt));
}

// optional.object/optional.object.ctor/in_place_t.pass.cpp
TEST(OptionalTest, CtorInPlaceTInt) {
  base::optional<int> opt(base::in_place, 5);
  EXPECT_TRUE(static_cast<bool>(opt));
  EXPECT_EQ(5, *opt);
}

// optional.object/optional.object.ctor/in_place_t.pass.cpp
TEST(OptionalTest, CtorInPlaceTCustomPairDefault) {
  base::optional<CustomPair> opt(base::in_place);
  EXPECT_TRUE(static_cast<bool>(opt));
  EXPECT_EQ(CustomPair(), *opt);
}

// optional.object/optional.object.ctor/in_place_t.pass.cpp
TEST(OptionalTest, CtorInPlaceTCustomPairOne) {
  base::optional<CustomPair> opt(base::in_place, 5);
  EXPECT_TRUE(static_cast<bool>(opt));
  EXPECT_EQ(CustomPair(5), *opt);
}

// optional.object/optional.object.ctor/in_place_t.pass.cpp
TEST(OptionalTest, CtorInPlaceTCustomPairTwo) {
  base::optional<CustomPair> opt(base::in_place, 5, 4);
  EXPECT_TRUE(static_cast<bool>(opt));
  EXPECT_EQ(CustomPair(5, 4), *opt);
}

// optional.object/optional.object.ctor/move.pass.cpp
TEST(OptionalTest, CtorMoveIntFromUnset) {
  base::optional<int> rhs;
  base::optional<int> lhs = std::move(rhs);
  EXPECT_FALSE(static_cast<bool>(lhs));
}

// optional.object/optional.object.ctor/move.pass.cpp
TEST(OptionalTest, CtorMoveIntFromSet) {
  base::optional<int> rhs(3);
  base::optional<int> lhs = std::move(rhs);
  EXPECT_TRUE(static_cast<bool>(lhs));
}

// optional.object/optional.object.ctor/move.pass.cpp
TEST(OptionalTest, CtorMoveObjectFromUnset) {
  base::optional<Object> rhs;
  base::optional<Object> lhs = std::move(rhs);
  EXPECT_FALSE(static_cast<bool>(lhs));
}

// optional.object/optional.object.ctor/move.pass.cpp
TEST(OptionalTest, CtorMoveObjectFromSet) {
  base::optional<Object> rhs(Object(3));
  base::optional<Object> lhs = std::move(rhs);
  EXPECT_TRUE(static_cast<bool>(lhs));
}

// optional.object/optional.object.ctor/nullopt_t.pass.cpp
TEST(OptionalTest, CtorNulloptTInt) {
  base::optional<int> opt(base::nullopt);
  EXPECT_FALSE(static_cast<bool>(opt));
}

// optional.object/optional.object.ctor/nullopt_t.pass.cpp
TEST(OptionalTest, CtorNulloptTIntPtr) {
  base::optional<int*> opt(base::nullopt);
  EXPECT_FALSE(static_cast<bool>(opt));
}

// optional.object/optional.object.ctor/nullopt_t.pass.cpp
TEST(OptionalTest, CtorNulloptTObject) {
  base::optional<Object> opt(base::nullopt);
  EXPECT_FALSE(static_cast<bool>(opt));
}

// optional.object/optional.object.ctor/rvalue_T.pass.cpp
TEST(OptionalTest, CtorRValueTInt) {
  using T = int;
  base::optional<int> opt(T(5));
  EXPECT_TRUE(static_cast<bool>(opt));
  EXPECT_EQ(5, *opt);
}

// optional.object/optional.object.ctor/rvalue_T.pass.cpp
TEST(OptionalTest, CtorRValueTDouble) {
  using T = double;
  base::optional<double> opt(T(3));
  EXPECT_TRUE(static_cast<bool>(opt));
  EXPECT_EQ(3, *opt);
}

// optional.object/optional.object.ctor/rvalue_T.pass.cpp
TEST(OptionalTest, CtorRValueTObject) {
  base::optional<Object> opt(Object(3));
  EXPECT_TRUE(static_cast<bool>(opt));
  EXPECT_EQ(Object(3), *opt);
}

// optional.object/optional.object.dtor/dtor.pass.cpp
TEST(OptionalTest, DtorInt) {
  ASSERT_TRUE(base::is_trivially_destructible<int>::value);
  EXPECT_TRUE(base::is_trivially_destructible<base::optional<int>>::value);
}

// optional.object/optional.object.dtor/dtor.pass.cpp
TEST(OptionalTest, DtorDouble) {
  ASSERT_TRUE(base::is_trivially_destructible<double>::value);
  EXPECT_TRUE(base::is_trivially_destructible<base::optional<double>>::value);
}

// optional.object/optional.object.dtor/dtor.pass.cpp
TEST(OptionalTest, DtorObject) {
  ASSERT_FALSE(base::is_trivially_destructible<Object>::value);
  EXPECT_FALSE(base::is_trivially_destructible<base::optional<Object>>::value);
  Object obj(0);
  scoped_ptr<base::optional<Object>> opt(new base::optional<Object>(obj));
  {
    ScopedObjectRecorder recorder;
    EXPECT_CALL(recorder, Dtor(_, _));
    opt.reset();
  }
}

// optional.object/optional.object.observe/bool.pass.cpp
TEST(OptionalTest, BoolUnset) {
  base::optional<int> opt;
  EXPECT_FALSE(opt);
}

// optional.object/optional.object.observe/bool.pass.cpp
TEST(OptionalTest, BoolSet) {
  base::optional<int> opt(0);
  EXPECT_TRUE(opt);
}

// optional.object/optional.object.observe/value_or.pass.cpp
TEST(OptionalTest, ValueOrRValueSetLValue) {
  base::optional<ConvertibleFromCustomIntWrapperWithMove> opt(base::in_place,
                                                              2);
  CustomIntWrapper y(3);
  EXPECT_EQ(2, std::move(opt).value_or(y));
  EXPECT_EQ(0, *opt);
}

// optional.object/optional.object.observe/value_or.pass.cpp
TEST(OptionalTest, ValueOrRValueSetRValue) {
  base::optional<ConvertibleFromCustomIntWrapperWithMove> opt(base::in_place,
                                                              2);
  EXPECT_EQ(2, std::move(opt).value_or(CustomIntWrapper(3)));
  EXPECT_EQ(0, *opt);
}

// optional.object/optional.object.observe/value_or.pass.cpp
TEST(OptionalTest, ValueOrRValueUnsetLValue) {
  base::optional<ConvertibleFromCustomIntWrapperWithMove> opt;
  CustomIntWrapper y(3);
  EXPECT_EQ(3, std::move(opt).value_or(y));
  EXPECT_FALSE(opt);
}

// optional.object/optional.object.observe/value_or.pass.cpp
TEST(OptionalTest, ValueOrRValueUnsetRValue) {
  base::optional<ConvertibleFromCustomIntWrapperWithMove> opt;
  EXPECT_EQ(4, std::move(opt).value_or(CustomIntWrapper(3)));
  EXPECT_FALSE(opt);
}

// optional.object/optional.object.observe/value_or_const.pass.cpp
TEST(OptionalTest, ValueOrSetLValue) {
  base::optional<ConvertibleFromCustomIntWrapperWithCopy> opt(2);
  CustomIntWrapper y(3);
  EXPECT_EQ(2, opt.value_or(y));
}

// optional.object/optional.object.observe/value_or_const.pass.cpp
TEST(OptionalTest, ValueOrSetRValue) {
  base::optional<ConvertibleFromCustomIntWrapperWithCopy> opt(2);
  EXPECT_EQ(2, opt.value_or(CustomIntWrapper(3)));
}

// optional.object/optional.object.observe/value_or_const.pass.cpp
TEST(OptionalTest, ValueOrUnsetLValue) {
  base::optional<ConvertibleFromCustomIntWrapperWithCopy> opt;
  CustomIntWrapper y(3);
  EXPECT_EQ(3, opt.value_or(y));
}

// optional.object/optional.object.observe/value_or_const.pass.cpp
TEST(OptionalTest, ValueOrUnsetRValue) {
  base::optional<ConvertibleFromCustomIntWrapperWithCopy> opt;
  EXPECT_EQ(4, opt.value_or(CustomIntWrapper(3)));
}

// optional.object/optional.object.observe/value_or_const.pass.cpp
TEST(OptionalTest, ValueOrConstSetLValue) {
  const base::optional<ConvertibleFromCustomIntWrapperWithCopy> opt(2);
  const CustomIntWrapper y(3);
  EXPECT_EQ(2, opt.value_or(y));
}

// optional.object/optional.object.observe/value_or_const.pass.cpp
TEST(OptionalTest, ValueOrConstSetRValue) {
  const base::optional<ConvertibleFromCustomIntWrapperWithCopy> opt(2);
  EXPECT_EQ(2, opt.value_or(CustomIntWrapper(3)));
}

// optional.object/optional.object.observe/value_or_const.pass.cpp
TEST(OptionalTest, ValueOrConstUnsetLValue) {
  const base::optional<ConvertibleFromCustomIntWrapperWithCopy> opt{};
  const CustomIntWrapper y(3);
  EXPECT_EQ(3, opt.value_or(y));
}

// optional.object/optional.object.observe/value_or_const.pass.cpp
TEST(OptionalTest, ValueOrConstUnsetRValue) {
  const base::optional<ConvertibleFromCustomIntWrapperWithCopy> opt{};
  EXPECT_EQ(4, opt.value_or(CustomIntWrapper(3)));
}

// optional.object/optional.object.swap/swap.pass.cpp
TEST(OptionalTest, MemberSwapIntUnsetWithUnset) {
  base::optional<int> opt1;
  base::optional<int> opt2;
  ASSERT_FALSE(static_cast<bool>(opt1));
  ASSERT_FALSE(static_cast<bool>(opt2));
  opt1.swap(opt2);
  EXPECT_FALSE(static_cast<bool>(opt1));
  EXPECT_FALSE(static_cast<bool>(opt2));
}

// optional.object/optional.object.swap/swap.pass.cpp
TEST(OptionalTest, MemberSwapIntSetWithUnset) {
  base::optional<int> opt1(1);
  base::optional<int> opt2;
  ASSERT_TRUE(static_cast<bool>(opt1));
  ASSERT_EQ(1, *opt1);
  ASSERT_FALSE(static_cast<bool>(opt2));
  opt1.swap(opt2);
  EXPECT_FALSE(static_cast<bool>(opt1));
  EXPECT_TRUE(static_cast<bool>(opt2));
  EXPECT_EQ(1, *opt2);
}

// optional.object/optional.object.swap/swap.pass.cpp
TEST(OptionalTest, MemberSwapIntUnsetWithSet) {
  base::optional<int> opt1;
  base::optional<int> opt2(2);
  ASSERT_FALSE(static_cast<bool>(opt1));
  ASSERT_TRUE(static_cast<bool>(opt2));
  ASSERT_EQ(2, *opt2);
  opt1.swap(opt2);
  EXPECT_TRUE(static_cast<bool>(opt1));
  EXPECT_EQ(2, *opt1);
  EXPECT_FALSE(static_cast<bool>(opt2));
}

// optional.object/optional.object.swap/swap.pass.cpp
TEST(OptionalTest, MemberSwapIntSetWithSet) {
  base::optional<int> opt1(1);
  base::optional<int> opt2(2);
  ASSERT_TRUE(static_cast<bool>(opt1));
  ASSERT_EQ(1, *opt1);
  ASSERT_TRUE(static_cast<bool>(opt2));
  ASSERT_EQ(2, *opt2);
  opt1.swap(opt2);
  EXPECT_TRUE(static_cast<bool>(opt1));
  EXPECT_EQ(2, *opt1);
  EXPECT_TRUE(static_cast<bool>(opt2));
  EXPECT_EQ(1, *opt2);
}

// optional.object/optional.object.swap/swap.pass.cpp
TEST(OptionalTest, MemberSwapObjectUnsetWithUnset) {
  base::optional<Object> opt1;
  base::optional<Object> opt2;
  ASSERT_FALSE(static_cast<bool>(opt1));
  ASSERT_FALSE(static_cast<bool>(opt2));
  {
    ScopedObjectRecorder recorder;
    opt1.swap(opt2);
    // Expecting nothing in the recorder to fire.
  }
  EXPECT_FALSE(static_cast<bool>(opt1));
  EXPECT_FALSE(static_cast<bool>(opt2));
}

// optional.object/optional.object.swap/swap.pass.cpp
TEST(OptionalTest, MemberSwapObjectSetWithUnset) {
  base::optional<Object> opt1(Object(1));
  base::optional<Object> opt2;
  ASSERT_TRUE(static_cast<bool>(opt1));
  ASSERT_EQ(1, opt1->value());
  ASSERT_FALSE(static_cast<bool>(opt2));
  {
    ScopedObjectRecorder recorder;
    EXPECT_CALL(recorder, MoveCtor(_, _));
    EXPECT_CALL(recorder, Dtor(_, _));
    opt1.swap(opt2);
    // Expecting nothing in the recorder to fire.
  }
  EXPECT_FALSE(static_cast<bool>(opt1));
  EXPECT_TRUE(static_cast<bool>(opt2));
  EXPECT_EQ(1, opt2->value());
}

// optional.object/optional.object.swap/swap.pass.cpp
TEST(OptionalTest, MemberSwapObjectUnsetWithSet) {
  base::optional<Object> opt1;
  base::optional<Object> opt2(Object(2));
  ASSERT_FALSE(static_cast<bool>(opt1));
  ASSERT_TRUE(static_cast<bool>(opt2));
  ASSERT_EQ(2, opt2->value());
  {
    ScopedObjectRecorder recorder;
    EXPECT_CALL(recorder, MoveCtor(_, _));
    EXPECT_CALL(recorder, Dtor(_, _));
    opt1.swap(opt2);
    // Expecting nothing in the recorder to fire.
  }
  EXPECT_TRUE(static_cast<bool>(opt1));
  EXPECT_EQ(2, opt1->value());
  EXPECT_FALSE(static_cast<bool>(opt2));
}

// optional.object/optional.object.swap/swap.pass.cpp
TEST(OptionalTest, MemberSwapObjectSetWithSet) {
  base::optional<Object> opt1(Object(1));
  base::optional<Object> opt2(Object(2));
  ASSERT_TRUE(static_cast<bool>(opt1));
  ASSERT_EQ(1, opt1->value());
  ASSERT_TRUE(static_cast<bool>(opt2));
  ASSERT_EQ(2, opt2->value());
  {
    ScopedObjectRecorder recorder;
    EXPECT_CALL(recorder, MoveCtor(_, _));
    EXPECT_CALL(recorder, MoveAssignment(_, _)).Times(2);
    EXPECT_CALL(recorder, Dtor(_, _));
    opt1.swap(opt2);
  }
  EXPECT_TRUE(static_cast<bool>(opt1));
  EXPECT_EQ(2, opt1->value());
  EXPECT_TRUE(static_cast<bool>(opt2));
  EXPECT_EQ(1, opt2->value());
}

// optional.object/optional.object.swap/swap.pass.cpp
TEST(OptionalTest, MemberSwapObjectWithSwapUnsetWithUnset) {
  base::optional<ObjectWithSwap> opt1;
  base::optional<ObjectWithSwap> opt2;
  ASSERT_FALSE(static_cast<bool>(opt1));
  ASSERT_FALSE(static_cast<bool>(opt2));
  {
    ScopedObjectRecorder recorder;
    opt1.swap(opt2);
    // Expecting nothing in the recorder to fire.
  }
  EXPECT_FALSE(static_cast<bool>(opt1));
  EXPECT_FALSE(static_cast<bool>(opt2));
}

// optional.object/optional.object.swap/swap.pass.cpp
TEST(OptionalTest, MemberSwapObjectWithSwapSetWithUnset) {
  base::optional<ObjectWithSwap> opt1(ObjectWithSwap(1));
  base::optional<ObjectWithSwap> opt2;
  ASSERT_TRUE(static_cast<bool>(opt1));
  ASSERT_EQ(1, opt1->value());
  ASSERT_FALSE(static_cast<bool>(opt2));
  {
    ScopedObjectRecorder recorder;
    EXPECT_CALL(recorder, MoveCtor(_, _));
    EXPECT_CALL(recorder, Dtor(_, _));
    opt1.swap(opt2);
    // Expecting nothing in the recorder to fire.
  }
  EXPECT_FALSE(static_cast<bool>(opt1));
  EXPECT_TRUE(static_cast<bool>(opt2));
  EXPECT_EQ(1, opt2->value());
}

// optional.object/optional.object.swap/swap.pass.cpp
TEST(OptionalTest, MemberSwapObjectWithSwapUnsetWithSet) {
  base::optional<ObjectWithSwap> opt1;
  base::optional<ObjectWithSwap> opt2(ObjectWithSwap(2));
  ASSERT_FALSE(static_cast<bool>(opt1));
  ASSERT_TRUE(static_cast<bool>(opt2));
  ASSERT_EQ(2, opt2->value());
  {
    ScopedObjectRecorder recorder;
    EXPECT_CALL(recorder, MoveCtor(_, _));
    EXPECT_CALL(recorder, Dtor(_, _));
    opt1.swap(opt2);
    // Expecting nothing in the recorder to fire.
  }
  EXPECT_TRUE(static_cast<bool>(opt1));
  EXPECT_EQ(2, opt1->value());
  EXPECT_FALSE(static_cast<bool>(opt2));
}

// optional.object/optional.object.swap/swap.pass.cpp
TEST(OptionalTest, MemberSwapObjectWithSwapSetWithSet) {
  base::optional<ObjectWithSwap> opt1(ObjectWithSwap(1));
  base::optional<ObjectWithSwap> opt2(ObjectWithSwap(2));
  ASSERT_TRUE(static_cast<bool>(opt1));
  ASSERT_EQ(1, opt1->value());
  ASSERT_TRUE(static_cast<bool>(opt2));
  ASSERT_EQ(2, opt2->value());
  {
    ScopedObjectRecorder recorder;
    opt1.swap(opt2);
    // Expecting nothing in the recorder to fire.
  }
  EXPECT_TRUE(static_cast<bool>(opt1));
  EXPECT_EQ(2, opt1->value());
  EXPECT_TRUE(static_cast<bool>(opt2));
  EXPECT_EQ(1, opt2->value());
}

// optional.relops/equal.pass.cpp
TEST(OptionalTest, EqualWithOptional) {
  base::optional<Object> o1;
  base::optional<Object> o2;
  base::optional<Object> o3(Object(1));
  base::optional<Object> o4(Object(2));
  base::optional<Object> o5(Object(1));

  EXPECT_TRUE(o1 == o1);
  EXPECT_TRUE(o1 == o2);
  EXPECT_FALSE(o1 == o3);
  EXPECT_FALSE(o1 == o4);
  EXPECT_FALSE(o1 == o5);

  EXPECT_TRUE(o2 == o1);
  EXPECT_TRUE(o2 == o2);
  EXPECT_FALSE(o2 == o3);
  EXPECT_FALSE(o2 == o4);
  EXPECT_FALSE(o2 == o5);

  EXPECT_FALSE(o3 == o1);
  EXPECT_FALSE(o3 == o2);
  EXPECT_TRUE(o3 == o3);
  EXPECT_FALSE(o3 == o4);
  EXPECT_TRUE(o3 == o5);

  EXPECT_FALSE(o4 == o1);
  EXPECT_FALSE(o4 == o2);
  EXPECT_FALSE(o4 == o3);
  EXPECT_TRUE(o4 == o4);
  EXPECT_FALSE(o4 == o5);

  EXPECT_FALSE(o5 == o1);
  EXPECT_FALSE(o5 == o2);
  EXPECT_TRUE(o5 == o3);
  EXPECT_FALSE(o5 == o4);
  EXPECT_TRUE(o5 == o5);
}

// optional.relops/greater_equal.pass.cpp
TEST(OptionalTest, GreaterEqualWithOptional) {
  base::optional<Object> o1;
  base::optional<Object> o2;
  base::optional<Object> o3(Object(1));
  base::optional<Object> o4(Object(2));
  base::optional<Object> o5(Object(1));

  EXPECT_TRUE(o1 >= o1);
  EXPECT_TRUE(o1 >= o2);
  EXPECT_FALSE(o1 >= o3);
  EXPECT_FALSE(o1 >= o4);
  EXPECT_FALSE(o1 >= o5);

  EXPECT_TRUE(o2 >= o1);
  EXPECT_TRUE(o2 >= o2);
  EXPECT_FALSE(o2 >= o3);
  EXPECT_FALSE(o2 >= o4);
  EXPECT_FALSE(o2 >= o5);

  EXPECT_TRUE(o3 >= o1);
  EXPECT_TRUE(o3 >= o2);
  EXPECT_TRUE(o3 >= o3);
  EXPECT_FALSE(o3 >= o4);
  EXPECT_TRUE(o3 >= o5);

  EXPECT_TRUE(o4 >= o1);
  EXPECT_TRUE(o4 >= o2);
  EXPECT_TRUE(o4 >= o3);
  EXPECT_TRUE(o4 >= o4);
  EXPECT_TRUE(o4 >= o5);

  EXPECT_TRUE(o5 >= o1);
  EXPECT_TRUE(o5 >= o2);
  EXPECT_TRUE(o5 >= o3);
  EXPECT_FALSE(o5 >= o4);
  EXPECT_TRUE(o5 >= o5);
}

// optional.relops/greater_than.pass.cpp
TEST(OptionalTest, GreaterThanWithOptional) {
  base::optional<Object> o1;
  base::optional<Object> o2;
  base::optional<Object> o3(Object(1));
  base::optional<Object> o4(Object(2));
  base::optional<Object> o5(Object(1));

  EXPECT_FALSE(o1 > o1);
  EXPECT_FALSE(o1 > o2);
  EXPECT_FALSE(o1 > o3);
  EXPECT_FALSE(o1 > o4);
  EXPECT_FALSE(o1 > o5);

  EXPECT_FALSE(o2 > o1);
  EXPECT_FALSE(o2 > o2);
  EXPECT_FALSE(o2 > o3);
  EXPECT_FALSE(o2 > o4);
  EXPECT_FALSE(o2 > o5);

  EXPECT_TRUE(o3 > o1);
  EXPECT_TRUE(o3 > o2);
  EXPECT_FALSE(o3 > o3);
  EXPECT_FALSE(o3 > o4);
  EXPECT_FALSE(o3 > o5);

  EXPECT_TRUE(o4 > o1);
  EXPECT_TRUE(o4 > o2);
  EXPECT_TRUE(o4 > o3);
  EXPECT_FALSE(o4 > o4);
  EXPECT_TRUE(o4 > o5);

  EXPECT_TRUE(o5 > o1);
  EXPECT_TRUE(o5 > o2);
  EXPECT_FALSE(o5 > o3);
  EXPECT_FALSE(o5 > o4);
  EXPECT_FALSE(o5 > o5);
}

// optional.relops/less_equal.pass.cpp
TEST(OptionalTest, LessEqualWithOptional) {
  base::optional<Object> o1;
  base::optional<Object> o2;
  base::optional<Object> o3(Object(1));
  base::optional<Object> o4(Object(2));
  base::optional<Object> o5(Object(1));

  EXPECT_TRUE(o1 <= o1);
  EXPECT_TRUE(o1 <= o2);
  EXPECT_TRUE(o1 <= o3);
  EXPECT_TRUE(o1 <= o4);
  EXPECT_TRUE(o1 <= o5);

  EXPECT_TRUE(o2 <= o1);
  EXPECT_TRUE(o2 <= o2);
  EXPECT_TRUE(o2 <= o3);
  EXPECT_TRUE(o2 <= o4);
  EXPECT_TRUE(o2 <= o5);

  EXPECT_FALSE(o3 <= o1);
  EXPECT_FALSE(o3 <= o2);
  EXPECT_TRUE(o3 <= o3);
  EXPECT_TRUE(o3 <= o4);
  EXPECT_TRUE(o3 <= o5);

  EXPECT_FALSE(o4 <= o1);
  EXPECT_FALSE(o4 <= o2);
  EXPECT_FALSE(o4 <= o3);
  EXPECT_TRUE(o4 <= o4);
  EXPECT_FALSE(o4 <= o5);

  EXPECT_FALSE(o5 <= o1);
  EXPECT_FALSE(o5 <= o2);
  EXPECT_TRUE(o5 <= o3);
  EXPECT_TRUE(o5 <= o4);
  EXPECT_TRUE(o5 <= o5);
}

// optional.relops/less_than.pass.cpp
TEST(OptionalTest, LessThanWithOptional) {
  base::optional<Object> o1;
  base::optional<Object> o2;
  base::optional<Object> o3(Object(1));
  base::optional<Object> o4(Object(2));
  base::optional<Object> o5(Object(1));

  EXPECT_FALSE(o1 < o1);
  EXPECT_FALSE(o1 < o2);
  EXPECT_TRUE(o1 < o3);
  EXPECT_TRUE(o1 < o4);
  EXPECT_TRUE(o1 < o5);

  EXPECT_FALSE(o2 < o1);
  EXPECT_FALSE(o2 < o2);
  EXPECT_TRUE(o2 < o3);
  EXPECT_TRUE(o2 < o4);
  EXPECT_TRUE(o2 < o5);

  EXPECT_FALSE(o3 < o1);
  EXPECT_FALSE(o3 < o2);
  EXPECT_FALSE(o3 < o3);
  EXPECT_TRUE(o3 < o4);
  EXPECT_FALSE(o3 < o5);

  EXPECT_FALSE(o4 < o1);
  EXPECT_FALSE(o4 < o2);
  EXPECT_FALSE(o4 < o3);
  EXPECT_FALSE(o4 < o4);
  EXPECT_FALSE(o4 < o5);

  EXPECT_FALSE(o5 < o1);
  EXPECT_FALSE(o5 < o2);
  EXPECT_FALSE(o5 < o3);
  EXPECT_TRUE(o5 < o4);
  EXPECT_FALSE(o5 < o5);
}

// optional.relops/not_equal.pass.cpp
TEST(OptionalTest, NotEqualWithOptional) {
  base::optional<Object> o1;
  base::optional<Object> o2;
  base::optional<Object> o3(Object(1));
  base::optional<Object> o4(Object(2));
  base::optional<Object> o5(Object(1));

  EXPECT_FALSE(o1 != o1);
  EXPECT_FALSE(o1 != o2);
  EXPECT_TRUE(o1 != o3);
  EXPECT_TRUE(o1 != o4);
  EXPECT_TRUE(o1 != o5);

  EXPECT_FALSE(o2 != o1);
  EXPECT_FALSE(o2 != o2);
  EXPECT_TRUE(o2 != o3);
  EXPECT_TRUE(o2 != o4);
  EXPECT_TRUE(o2 != o5);

  EXPECT_TRUE(o3 != o1);
  EXPECT_TRUE(o3 != o2);
  EXPECT_FALSE(o3 != o3);
  EXPECT_TRUE(o3 != o4);
  EXPECT_FALSE(o3 != o5);

  EXPECT_TRUE(o4 != o1);
  EXPECT_TRUE(o4 != o2);
  EXPECT_TRUE(o4 != o3);
  EXPECT_FALSE(o4 != o4);
  EXPECT_TRUE(o4 != o5);

  EXPECT_TRUE(o5 != o1);
  EXPECT_TRUE(o5 != o2);
  EXPECT_FALSE(o5 != o3);
  EXPECT_TRUE(o5 != o4);
  EXPECT_FALSE(o5 != o5);
}

// optional.specalg/make_optional.pass.cpp
TEST(OptionalTest, MakeOptionalInt) {
  base::optional<int> opt = base::make_optional(2);
  EXPECT_EQ(2, *opt);
}

// optional.specalg/make_optional.pass.cpp
TEST(OptionalTest, MakeOptionalString) {
  std::string s("123");
  base::optional<std::string> opt = base::make_optional(s);
  EXPECT_EQ(s, *opt);
}

// optional.specalg/make_optional.pass.cpp
TEST(OptionalTest, MakeOptionalStringMove) {
  std::string s("123");
  base::optional<std::string> opt = base::make_optional(std::move(s));
  EXPECT_EQ("123", *opt);
  EXPECT_TRUE(s.empty());
}

// test/std/experimental/optional/optional.specalg/make_optional.pass.cpp
TEST(OptionalTest, MakeOptionalUniquePtr) {
  std::unique_ptr<int> s(new int(3));
  base::optional<std::unique_ptr<int>> opt = base::make_optional(std::move(s));
  EXPECT_EQ(3, **opt);
  EXPECT_EQ(nullptr, s);
}

// optional.specalg/swap.pass.cpp
TEST(OptionalTest, SwapIntUnsetWithUnset) {
  base::optional<int> opt1;
  base::optional<int> opt2;
  ASSERT_FALSE(static_cast<bool>(opt1));
  ASSERT_FALSE(static_cast<bool>(opt2));
  swap(opt1, opt2);
  EXPECT_FALSE(static_cast<bool>(opt1));
  EXPECT_FALSE(static_cast<bool>(opt2));
}

// optional.specalg/swap.pass.cpp
TEST(OptionalTest, SwapIntSetWithUnset) {
  base::optional<int> opt1(1);
  base::optional<int> opt2;
  ASSERT_TRUE(static_cast<bool>(opt1));
  ASSERT_EQ(1, *opt1);
  ASSERT_FALSE(static_cast<bool>(opt2));
  swap(opt1, opt2);
  EXPECT_FALSE(static_cast<bool>(opt1));
  EXPECT_TRUE(static_cast<bool>(opt2));
  EXPECT_EQ(1, *opt2);
}

// optional.specalg/swap.pass.cpp
TEST(OptionalTest, SwapIntUnsetWithSet) {
  base::optional<int> opt1;
  base::optional<int> opt2(2);
  ASSERT_FALSE(static_cast<bool>(opt1));
  ASSERT_TRUE(static_cast<bool>(opt2));
  ASSERT_EQ(2, *opt2);
  swap(opt1, opt2);
  EXPECT_TRUE(static_cast<bool>(opt1));
  EXPECT_EQ(2, *opt1);
  EXPECT_FALSE(static_cast<bool>(opt2));
}

// optional.specalg/swap.pass.cpp
TEST(OptionalTest, SwapIntSetWithSet) {
  base::optional<int> opt1(1);
  base::optional<int> opt2(2);
  ASSERT_TRUE(static_cast<bool>(opt1));
  ASSERT_EQ(1, *opt1);
  ASSERT_TRUE(static_cast<bool>(opt2));
  ASSERT_EQ(2, *opt2);
  swap(opt1, opt2);
  EXPECT_TRUE(static_cast<bool>(opt1));
  EXPECT_EQ(2, *opt1);
  EXPECT_TRUE(static_cast<bool>(opt2));
  EXPECT_EQ(1, *opt2);
}

// optional.specalg/swap.pass.cpp
TEST(OptionalTest, SwapObjectUnsetWithUnset) {
  base::optional<Object> opt1;
  base::optional<Object> opt2;
  ASSERT_FALSE(static_cast<bool>(opt1));
  ASSERT_FALSE(static_cast<bool>(opt2));
  {
    ScopedObjectRecorder recorder;
    swap(opt1, opt2);
    // Expecting nothing in recorder to fire;
  }
  EXPECT_FALSE(static_cast<bool>(opt1));
  EXPECT_FALSE(static_cast<bool>(opt2));
}

// optional.specalg/swap.pass.cpp
TEST(OptionalTest, SwapObjectSetWithUnset) {
  base::optional<Object> opt1(Object(1));
  base::optional<Object> opt2;
  ASSERT_TRUE(static_cast<bool>(opt1));
  ASSERT_EQ(1, opt1->value());
  ASSERT_FALSE(static_cast<bool>(opt2));
  {
    ScopedObjectRecorder recorder;
    EXPECT_CALL(recorder, MoveCtor(_, _));
    EXPECT_CALL(recorder, Dtor(_, _));
    swap(opt1, opt2);
  }
  EXPECT_FALSE(static_cast<bool>(opt1));
  EXPECT_TRUE(static_cast<bool>(opt2));
  EXPECT_EQ(1, opt2->value());
}

// optional.specalg/swap.pass.cpp
TEST(OptionalTest, SwapObjectUnsetWithSet) {
  base::optional<Object> opt1;
  base::optional<Object> opt2(Object(2));
  ASSERT_FALSE(static_cast<bool>(opt1));
  ASSERT_TRUE(static_cast<bool>(opt2));
  ASSERT_EQ(2, opt2->value());
  {
    ScopedObjectRecorder recorder;
    EXPECT_CALL(recorder, MoveCtor(_, _));
    EXPECT_CALL(recorder, Dtor(_, _));
    swap(opt1, opt2);
  }
  EXPECT_TRUE(static_cast<bool>(opt1));
  EXPECT_EQ(2, opt1->value());
  EXPECT_FALSE(static_cast<bool>(opt2));
}

// optional.specalg/swap.pass.cpp
TEST(OptionalTest, SwapObjectSetWithSet) {
  base::optional<Object> opt1(Object(1));
  base::optional<Object> opt2(Object(2));
  ASSERT_TRUE(static_cast<bool>(opt1));
  ASSERT_EQ(1, opt1->value());
  ASSERT_TRUE(static_cast<bool>(opt2));
  ASSERT_EQ(2, opt2->value());
  {
    ScopedObjectRecorder recorder;
    EXPECT_CALL(recorder, MoveCtor(_, _));
    EXPECT_CALL(recorder, MoveAssignment(_, _)).Times(2);
    EXPECT_CALL(recorder, Dtor(_, _));
    swap(opt1, opt2);
  }
  EXPECT_TRUE(static_cast<bool>(opt1));
  EXPECT_EQ(2, opt1->value());
  EXPECT_TRUE(static_cast<bool>(opt2));
  EXPECT_EQ(1, opt2->value());
}

// optional.specalg/swap.pass.cpp
TEST(OptionalTest, SwapObjectWithSwapUnsetWithUnset) {
  base::optional<ObjectWithSwap> opt1;
  base::optional<ObjectWithSwap> opt2;
  ASSERT_FALSE(static_cast<bool>(opt1));
  ASSERT_FALSE(static_cast<bool>(opt2));
  {
    ScopedObjectRecorder recorder;
    swap(opt1, opt2);
    // Expecting nothing in recorder to fire;
  }
  EXPECT_FALSE(static_cast<bool>(opt1));
  EXPECT_FALSE(static_cast<bool>(opt2));
}

// optional.specalg/swap.pass.cpp
TEST(OptionalTest, SwapObjectWithSwapSetWithUnset) {
  base::optional<ObjectWithSwap> opt1(ObjectWithSwap(1));
  base::optional<ObjectWithSwap> opt2;
  ASSERT_TRUE(static_cast<bool>(opt1));
  ASSERT_EQ(1, opt1->value());
  ASSERT_FALSE(static_cast<bool>(opt2));
  {
    ScopedObjectRecorder recorder;
    EXPECT_CALL(recorder, MoveCtor(_, _));
    EXPECT_CALL(recorder, Dtor(_, _));
    swap(opt1, opt2);
  }
  EXPECT_FALSE(static_cast<bool>(opt1));
  EXPECT_TRUE(static_cast<bool>(opt2));
  EXPECT_EQ(1, opt2->value());
}

// optional.specalg/swap.pass.cpp
TEST(OptionalTest, SwapObjectWithSwapUnsetWithSet) {
  base::optional<ObjectWithSwap> opt1;
  base::optional<ObjectWithSwap> opt2(ObjectWithSwap(2));
  ASSERT_FALSE(static_cast<bool>(opt1));
  ASSERT_TRUE(static_cast<bool>(opt2));
  ASSERT_EQ(2, opt2->value());
  {
    ScopedObjectRecorder recorder;
    EXPECT_CALL(recorder, MoveCtor(_, _));
    EXPECT_CALL(recorder, Dtor(_, _));
    swap(opt1, opt2);
  }
  EXPECT_TRUE(static_cast<bool>(opt1));
  EXPECT_EQ(2, opt1->value());
  EXPECT_FALSE(static_cast<bool>(opt2));
}

// optional.specalg/swap.pass.cpp
TEST(OptionalTest, SwapObjectWithSwapSetWithSet) {
  base::optional<ObjectWithSwap> opt1(ObjectWithSwap(1));
  base::optional<ObjectWithSwap> opt2(ObjectWithSwap(2));
  ASSERT_TRUE(static_cast<bool>(opt1));
  ASSERT_EQ(1, opt1->value());
  ASSERT_TRUE(static_cast<bool>(opt2));
  ASSERT_EQ(2, opt2->value());
  {
    ScopedObjectRecorder recorder;
    swap(opt1, opt2);
    // Expecting nothing in recorder to fire.
  }
  EXPECT_TRUE(static_cast<bool>(opt1));
  EXPECT_EQ(2, opt1->value());
  EXPECT_TRUE(static_cast<bool>(opt2));
  EXPECT_EQ(1, opt2->value());
}
