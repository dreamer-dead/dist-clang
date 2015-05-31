#include <base/file_utils.h>

#include <base/const_string.h>
#include <base/file/file.h>
#include <base/temporary_dir.h>

#include <third_party/gtest/exported/include/gtest/gtest.h>
#include STL(thread)

#include <dirent.h>
#include <fcntl.h>

namespace dist_clang {
namespace base {

TEST(FileUtilsTest, CalculateDirectorySize) {
  const base::TemporaryDir temp_dir;
  const String dir1 = String(temp_dir) + "/1";
  const String dir2 = String(temp_dir) + "/2";
  const String file1 = String(temp_dir) + "/file1";
  const String file2 = dir1 + "/file2";
  const String file3 = dir2 + "/file3";
  const String content1 = "a";
  const String content2 = "ab";
  const String content3 = "abc";

  ASSERT_NE(-1, mkdir(dir1.c_str(), 0777));
  ASSERT_NE(-1, mkdir(dir2.c_str(), 0777));
  int fd1 = open(file1.c_str(), O_CREAT | O_WRONLY);
  int fd2 = open(file2.c_str(), O_CREAT | O_WRONLY);
  int fd3 = open(file3.c_str(), O_CREAT | O_WRONLY);
  ASSERT_TRUE(fd1 != -1 && fd2 != -1 && fd3 != -1);
  ASSERT_EQ(content1.size(),
            static_cast<size_t>(write(fd1, content1.data(), content1.size())));
  ASSERT_EQ(content2.size(),
            static_cast<size_t>(write(fd2, content2.data(), content2.size())));
  ASSERT_EQ(content3.size(),
            static_cast<size_t>(write(fd3, content3.data(), content3.size())));
  close(fd1);
  close(fd2);
  close(fd3);

  String error;
  EXPECT_EQ(content1.size() + content2.size() + content3.size(),
            CalculateDirectorySize(temp_dir, &error))
      << error;
}

TEST(FileUtilsTest, LeastRecentPath) {
  const base::TemporaryDir temp_dir;
  const String dir = String(temp_dir) + "/1";
  const String file1 = String(temp_dir) + "/2";
  const String file2 = dir + "/3";
  const String file3 = dir + "/4";

  ASSERT_NE(-1, mkdir(dir.c_str(), 0777));

  std::this_thread::sleep_for(std::chrono::seconds(1));
  int fd = open(file1.c_str(), O_CREAT, 0777);
  ASSERT_NE(-1, fd);
  close(fd);

  String path;
  EXPECT_TRUE(GetLeastRecentPath(temp_dir, path));
  EXPECT_EQ(dir, path) << "dir mtime is " << GetModificationTime(dir).first
                       << ":" << GetModificationTime(dir).second
                       << " ; path mtime is " << GetModificationTime(path).first
                       << ":" << GetModificationTime(path).second;

  std::this_thread::sleep_for(std::chrono::seconds(1));
  fd = open(file2.c_str(), O_CREAT, 0777);
  ASSERT_NE(-1, fd);
  close(fd);

  EXPECT_TRUE(GetLeastRecentPath(temp_dir, path));
  EXPECT_EQ(file1, path) << "file1 mtime is "
                         << GetModificationTime(file1).first << ":"
                         << GetModificationTime(file1).second
                         << " ; path mtime is "
                         << GetModificationTime(path).first << ":"
                         << GetModificationTime(path).second;

  std::this_thread::sleep_for(std::chrono::seconds(1));
  fd = open(file3.c_str(), O_CREAT, 0777);
  ASSERT_NE(-1, fd);
  close(fd);

  EXPECT_TRUE(GetLeastRecentPath(dir, path));
  EXPECT_EQ(file2, path) << "file2 mtime is "
                         << GetModificationTime(file2).first << ":"
                         << GetModificationTime(file2).second
                         << " ; path mtime is "
                         << GetModificationTime(path).first << ":"
                         << GetModificationTime(path).second;
}

TEST(FileUtilsTest, LeastRecentPathWithRegex) {
  const base::TemporaryDir temp_dir;
  const String file1 = String(temp_dir) + "/1";
  const String file2 = String(temp_dir) + "/2";

  int fd = open(file1.c_str(), O_CREAT, 0777);
  ASSERT_NE(-1, fd);
  close(fd);

  std::this_thread::sleep_for(std::chrono::seconds(1));
  fd = open(file2.c_str(), O_CREAT, 0777);
  ASSERT_NE(-1, fd);
  close(fd);

  String path;
  EXPECT_TRUE(GetLeastRecentPath(temp_dir, path, "2"));
  EXPECT_EQ(file2, path);
}

TEST(FileUtilsTest, TempFile) {
  String error;
  const String temp_file = CreateTempFile(&error);

  ASSERT_FALSE(temp_file.empty())
      << "Failed to create temporary file: " << error;
  ASSERT_TRUE(File::Exists(temp_file));
  ASSERT_TRUE(File::Delete(temp_file));
}

TEST(FileUtilsTest, CreateDirectory) {
  String error;
  const base::TemporaryDir temp_dir;
  const String temp = String(temp_dir) + "/1/2/3";

  ASSERT_TRUE(CreateDirectory(temp, &error)) << error;

  DIR* dir = opendir(temp.c_str());
  EXPECT_TRUE(dir);
  if (dir) {
    closedir(dir);
  }
}

}  // namespace base
}  // namespace dist_clang
