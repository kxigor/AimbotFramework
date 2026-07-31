#include <gtest/gtest.h>

#include <afw/win/com_ptr.hpp>
#include <utility>

namespace {

class FakeObject {
 public:
  explicit FakeObject(int& releases) noexcept : releases_{&releases} {}

  void Release() noexcept { ++*releases_; }

 private:
  int* releases_;
};

}  // namespace

TEST(ComPtrUnit, DefaultConstructedHoldsNothing) {
  const afw::win::ComPtr<FakeObject> kPtr{};

  EXPECT_EQ(kPtr.Get(), nullptr);
  EXPECT_FALSE(static_cast<bool>(kPtr));
}

TEST(ComPtrUnit, DestructorReleasesExactlyOnce) {
  int releases{0};
  FakeObject object{releases};

  {
    const afw::win::ComPtr<FakeObject> kPtr{&object};
    ASSERT_TRUE(static_cast<bool>(kPtr));
  }

  EXPECT_EQ(releases, 1);
}

TEST(ComPtrUnit, ArrowReachesTheObject) {
  int releases{0};
  FakeObject object{releases};
  const afw::win::ComPtr<FakeObject> kPtr{&object};

  kPtr->Release();

  EXPECT_EQ(releases, 1);
}

TEST(ComPtrUnit, ReleaseHandsOwnershipToTheCaller) {
  int releases{0};
  FakeObject object{releases};

  {
    afw::win::ComPtr<FakeObject> ptr{&object};
    EXPECT_EQ(ptr.Release(), &object);
    EXPECT_EQ(ptr.Get(), nullptr);
  }

  EXPECT_EQ(releases, 0);
}

TEST(ComPtrUnit, ResetReleasesWhatItReplaces) {
  int firstReleases{0};
  int secondReleases{0};
  FakeObject first{firstReleases};
  FakeObject second{secondReleases};

  afw::win::ComPtr<FakeObject> ptr{&first};
  ptr.Reset(&second);

  EXPECT_EQ(firstReleases, 1);
  EXPECT_EQ(secondReleases, 0);
  EXPECT_EQ(ptr.Get(), &second);
}

TEST(ComPtrUnit, ResettingToTheSamePointerIsANoOp) {
  int releases{0};
  FakeObject object{releases};

  afw::win::ComPtr<FakeObject> ptr{&object};
  ptr.Reset(&object);

  EXPECT_EQ(releases, 0);
  EXPECT_EQ(ptr.Get(), &object);
}

TEST(ComPtrUnit, MoveConstructionLeavesTheSourceEmpty) {
  int releases{0};
  FakeObject object{releases};

  {
    afw::win::ComPtr<FakeObject> source{&object};
    const afw::win::ComPtr<FakeObject> kMoved{std::move(source)};

    EXPECT_EQ(kMoved.Get(), &object);
    EXPECT_EQ(source.Get(), nullptr);
    EXPECT_EQ(releases, 0);
  }

  EXPECT_EQ(releases, 1);
}

TEST(ComPtrUnit, MoveAssignmentReleasesTheTarget) {
  int sourceReleases{0};
  int targetReleases{0};
  FakeObject sourceObject{sourceReleases};
  FakeObject targetObject{targetReleases};

  afw::win::ComPtr<FakeObject> source{&sourceObject};
  afw::win::ComPtr<FakeObject> target{&targetObject};

  target = std::move(source);

  EXPECT_EQ(targetReleases, 1);
  EXPECT_EQ(sourceReleases, 0);
  EXPECT_EQ(target.Get(), &sourceObject);
  EXPECT_EQ(source.Get(), nullptr);
}

TEST(ComPtrUnit, PutDropsWhatIsHeldAndYieldsTheAddress) {
  int firstReleases{0};
  int secondReleases{0};
  FakeObject first{firstReleases};
  FakeObject second{secondReleases};

  afw::win::ComPtr<FakeObject> ptr{&first};
  *ptr.Put() = &second;

  EXPECT_EQ(firstReleases, 1);
  EXPECT_EQ(ptr.Get(), &second);
}
