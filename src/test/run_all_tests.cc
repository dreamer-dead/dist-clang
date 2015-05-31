#include <base/c_utils.h>
#include <base/constants.h>
#include <base/logging.h>
#include <base/string_utils.h>

#include <third_party/gtest/exported/include/gtest/gtest.h>

#include <base/using_log.h>

int main(int argc, char* argv[]) {
  // Ignore SIGPIPE to prevent application crashes.
  signal(SIGPIPE, SIG_IGN);

  using namespace dist_clang;

  Immutable clangd_log_levels = base::GetEnv(base::kEnvLogLevels);

  if (!clangd_log_levels.empty()) {
    Immutable clangd_log_mark = base::GetEnv(base::kEnvLogErrorMark);
    List<String> numbers;

    base::SplitString<' '>(clangd_log_levels, numbers);
    if (numbers.size() % 2 == 0) {
      base::Log::RangeSet ranges;
      for (auto number = numbers.begin(); number != numbers.end(); ++number) {
        ui32 left = base::StringTo<ui32>(*number++);
        ui32 right = base::StringTo<ui32>(*number);
        ranges.emplace(right, left);
      }
      base::Log::Reset(base::StringTo<ui32>(clangd_log_mark),
                       std::move(ranges));
    }
  }

  ::testing::InitGoogleTest(&argc, argv);
  ::testing::FLAGS_gtest_death_test_style = "fast";
  return RUN_ALL_TESTS();
}
