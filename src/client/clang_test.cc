#include "client/clang_flag_set.h"

#include "proto/remote.pb.h"

#include <gtest/gtest.h>

namespace dist_clang {
namespace client {

TEST(ClientTest, ClangFlagSetTest) {
  std::list<std::string> input = {
    "",
    "/home/test/.local/bin/clang", "-cc1",
    "-triple", "x86_64-unknown-linux-gnu",
    "-emit-obj",
    "-mrelax-all",
    "-disable-free",
    "-main-file-name", "test.cc",
    "-mrelocation-model", "static",
    "-mdisable-fp-elim",
    "-fmath-errno",
    "-masm-verbose",
    "-mconstructor-aliases",
    "-munwind-tables",
    "-fuse-init-array",
    "-target-cpu", "x86-64",
    "-target-linker-version", "2.23.2",
    "-coverage-file", "/tmp/test.o",
    "-resource-dir", "/home/test/.local/lib/clang/3.4",
    "-internal-isystem", "/usr/include/c++/4.8.2",
    "-internal-isystem", "/usr/include/c++/4.8.2/x86_64-redhat-linux",
    "-internal-isystem", "/usr/include/c++/4.8.2/backward",
    "-internal-isystem", "/usr/include/x86_64-redhat-linux/c++/4.8.2",
    "-internal-isystem", "/usr/local/include",
    "-internal-isystem", "/home/test/.local/lib/clang/3.4/include",
    "-internal-externc-isystem", "/include",
    "-internal-externc-isystem", "/usr/include",
    "-fdeprecated-macro",
    "-fdebug-compilation-dir", "/tmp",
    "-ferror-limit", "19",
    "-fmessage-length", "213",
    "-mstackrealign",
    "-fobjc-runtime=gcc",
    "-fcxx-exceptions",
    "-fexceptions",
    "-fdiagnostics-show-option",
    "-fcolor-diagnostics",
    "-vectorize-slp",
    "-o", "test.o",
    "-x", "c++",
    "test.cc"
  };

  proto::Flags expected_flags;
  expected_flags.mutable_compiler()->set_path("/home/test/.local/bin/clang");
  expected_flags.mutable_compiler()->set_version("0.0");
  expected_flags.set_output("test.o");
  expected_flags.set_input("test.cc");
  expected_flags.set_language("c++");
  expected_flags.add_other()->assign("-cc1");
  expected_flags.add_other()->assign("-triple");
  expected_flags.add_other()->assign("x86_64-unknown-linux-gnu");
  expected_flags.add_other()->assign("-emit-obj");
  expected_flags.add_other()->assign("-mrelax-all");
  expected_flags.add_other()->assign("-disable-free");
  expected_flags.add_other()->assign("-mrelocation-model");
  expected_flags.add_other()->assign("static");
  expected_flags.add_other()->assign("-mdisable-fp-elim");
  expected_flags.add_other()->assign("-fmath-errno");
  expected_flags.add_other()->assign("-masm-verbose");
  expected_flags.add_other()->assign("-mconstructor-aliases");
  expected_flags.add_other()->assign("-munwind-tables");
  expected_flags.add_other()->assign("-fuse-init-array");
  expected_flags.add_other()->assign("-target-cpu");
  expected_flags.add_other()->assign("x86-64");
  expected_flags.add_other()->assign("-target-linker-version");
  expected_flags.add_other()->assign("2.23.2");
  expected_flags.add_other()->assign("-fdeprecated-macro");
  expected_flags.add_other()->assign("-ferror-limit");
  expected_flags.add_other()->assign("19");
  expected_flags.add_other()->assign("-fmessage-length");
  expected_flags.add_other()->assign("213");
  expected_flags.add_other()->assign("-mstackrealign");
  expected_flags.add_other()->assign("-fobjc-runtime=gcc");
  expected_flags.add_other()->assign("-fcxx-exceptions");
  expected_flags.add_other()->assign("-fexceptions");
  expected_flags.add_other()->assign("-fdiagnostics-show-option");
  expected_flags.add_other()->assign("-fcolor-diagnostics");
  expected_flags.add_other()->assign("-vectorize-slp");
  expected_flags.add_non_cached()->assign("-main-file-name");
  expected_flags.add_non_cached()->assign("test.cc");
  expected_flags.add_non_cached()->assign("-coverage-file");
  expected_flags.add_non_cached()->assign("/tmp/test.o");
  expected_flags.add_non_cached()->assign("-resource-dir");
  expected_flags.add_non_cached()->assign("/home/test/.local/lib/clang/3.4");
  expected_flags.add_non_cached()->assign("-internal-isystem");
  expected_flags.add_non_cached()->assign("/usr/include/c++/4.8.2");
  expected_flags.add_non_cached()->assign("-internal-isystem");
  expected_flags.add_non_cached()->assign(
      "/usr/include/c++/4.8.2/x86_64-redhat-linux");
  expected_flags.add_non_cached()->assign("-internal-isystem");
  expected_flags.add_non_cached()->assign("/usr/include/c++/4.8.2/backward");
  expected_flags.add_non_cached()->assign("-internal-isystem");
  expected_flags.add_non_cached()->assign(
      "/usr/include/x86_64-redhat-linux/c++/4.8.2");
  expected_flags.add_non_cached()->assign("-internal-isystem");
  expected_flags.add_non_cached()->assign("/usr/local/include");
  expected_flags.add_non_cached()->assign("-internal-isystem");
  expected_flags.add_non_cached()->assign(
      "/home/test/.local/lib/clang/3.4/include");
  expected_flags.add_non_cached()->assign("-internal-externc-isystem");
  expected_flags.add_non_cached()->assign("/include");
  expected_flags.add_non_cached()->assign("-internal-externc-isystem");
  expected_flags.add_non_cached()->assign("/usr/include");
  expected_flags.add_non_cached()->assign("-fdebug-compilation-dir");
  expected_flags.add_non_cached()->assign("/tmp");

  proto::Flags actual_flags;
  actual_flags.mutable_compiler()->set_version("0.0");
  ASSERT_EQ(ClangFlagSet::COMPILE,
            ClangFlagSet::ProcessFlags(input, &actual_flags));
  ASSERT_EQ(expected_flags.SerializeAsString(),
            actual_flags.SerializeAsString());
}

}  // namespace client
}  // namespace dist_clang