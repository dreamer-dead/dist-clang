#include <base/const_string.h>

#include <base/string_utils.h>

#include <third_party/gtest/exported/include/gtest/gtest.h>

namespace dist_clang {
namespace base {

TEST(ConstStringTest, Find) {
  ConstString string("cdabcdcef"_l);
  EXPECT_EQ(4u, string.find("cdc"));
  EXPECT_EQ(String::npos, string.find("zz"));
}

TEST(ConstStringTest, Hash) {
  const auto expected_hash = "c9e92e37df1e856cbd0abffe104225b8"_l;
  EXPECT_EQ(expected_hash,
            Hexify(ConstString("All your base are belong to us"_l).Hash()));
  EXPECT_EQ(expected_hash,
            Hexify(ConstString(ConstString::Rope{"All "_l, "your base"_l,
                                                 " are belong to us"_l})
                       .Hash()));
  EXPECT_EQ(expected_hash,
            Hexify(ConstString(ConstString::Rope{"All your"_l,
                                                 " base are belong to us"_l})
                       .Hash()));
}

TEST(ConstStringTest, EmptyString) {
  ConstString empty1;
  EXPECT_EQ(String(), empty1.string_copy(false));
  EXPECT_STREQ("", empty1.c_str());

  ConstString empty2((ConstString::Rope()));
  EXPECT_EQ(String(), empty2.string_copy(false));
  EXPECT_STREQ("", empty2.c_str());

  ConstString empty3((String()));
  EXPECT_EQ(String(), empty3.string_copy(false));
  EXPECT_STREQ("", empty3.c_str());
}

// TODO: write a lot of tests.

}  // namespace base
}  // namespace dist_clang
