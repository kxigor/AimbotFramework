#include <gtest/gtest.h>

#include <afw/games/fc3/internal/bones.hpp>
#include <afw/games/fc3/internal/components.hpp>
#include <afw/games/fc3/internal/layout.hpp>
#include <cstdint>
#include <support/fc3_world.hpp>

namespace {

using afw::testing::Fc3World;

// The component walk is what reaches a moving entity's real world matrix. The
// engine picks the component by a class hash it gets through a virtual call;
// this picks it by the data, so the test is about that choice being sound.
class Fc3ComponentUnit : public Fc3World {
 protected:
  static constexpr std::uint32_t kEntity{0x600};
  static constexpr std::uint32_t kComponentArray{0x700};
  static constexpr std::uint32_t kComponentOne{0xA00};
  static constexpr std::uint32_t kComponentTwo{0xB80};
  static constexpr std::uint32_t kCells{16};
  static constexpr std::uint32_t kCellBytes{4};

  void SetUp() override {
    Fc3World::SetUp();
    if (page_ == nullptr) {
      return;
    }

    Poke(kEntity + afw::games::fc3::kEntityComponentArray, At(kComponentArray));
    Poke(kEntity + afw::games::fc3::kEntityComponentCount, 2);

    Poke(kComponentArray, At(kComponentOne));
    Poke(kComponentArray + sizeof(std::uint32_t), At(kComponentTwo));

    PokeRigidMatrix(kComponentTwo + afw::games::fc3::kComponentWorldMatrix,
                    7.0F, 8.0F, 9.0F);
  }

  void PokeRigidMatrix(std::uintptr_t offset, float x, float y, float z) {
    for (std::uint32_t cell{0}; cell < kCells; ++cell) {
      PokeFloat(offset + (cell * kCellBytes), 0.0F);
    }
    PokeFloat(offset + 0, 1.0F);
    PokeFloat(offset + 20, 1.0F);
    PokeFloat(offset + 40, 1.0F);
    PokeFloat(offset + 60, 1.0F);

    PokeFloat(offset + 48, x);
    PokeFloat(offset + 52, y);
    PokeFloat(offset + 56, z);
  }
};

TEST_F(Fc3ComponentUnit, TakesTheMatrixFromTheComponentThatHoldsABasis) {
  const auto kMatrix{afw::games::fc3::ReadComponentWorldMatrix(At(kEntity))};

  ASSERT_TRUE(kMatrix.has_value());
  EXPECT_FLOAT_EQ(kMatrix->Translation().x, 7.0F);
  EXPECT_FLOAT_EQ(kMatrix->Translation().y, 8.0F);
  EXPECT_FLOAT_EQ(kMatrix->Translation().z, 9.0F);
}

// The first component is scanned first and holds nothing basis-shaped, which is
// the normal case: an entity carries many components and only one answers.
TEST_F(Fc3ComponentUnit, StepsPastAComponentWhoseSlotIsNotABasis) {
  PokeFloat(kComponentOne + afw::games::fc3::kComponentWorldMatrix, 3.0F);

  const auto kMatrix{afw::games::fc3::ReadComponentWorldMatrix(At(kEntity))};

  ASSERT_TRUE(kMatrix.has_value());
  EXPECT_FLOAT_EQ(kMatrix->Translation().x, 7.0F);
}

TEST_F(Fc3ComponentUnit, StepsOverAnEmptySlot) {
  Poke(kComponentArray, 0);

  const auto kMatrix{afw::games::fc3::ReadComponentWorldMatrix(At(kEntity))};

  ASSERT_TRUE(kMatrix.has_value());
  EXPECT_FLOAT_EQ(kMatrix->Translation().x, 7.0F);
}

TEST_F(Fc3ComponentUnit, FindsNothingWhenNoComponentHoldsABasis) {
  PokeRigidMatrix(kComponentTwo + afw::games::fc3::kComponentWorldMatrix, 0.0F,
                  0.0F, 0.0F);
  PokeFloat(kComponentTwo + afw::games::fc3::kComponentWorldMatrix, 5.0F);

  EXPECT_FALSE(
      afw::games::fc3::ReadComponentWorldMatrix(At(kEntity)).has_value());
}

TEST_F(Fc3ComponentUnit, RefusesAnImplausibleComponentCount) {
  Poke(kEntity + afw::games::fc3::kEntityComponentCount, 100000);

  EXPECT_FALSE(
      afw::games::fc3::ReadComponentWorldMatrix(At(kEntity)).has_value());
}

// The skeleton's pose rides in the same component that holds the world matrix,
// and a bone is named only by a hash: the retail build keeps no bone name
// strings at all. "Head" hashes to kHeadBoneKey under the engine's own CRC-32.
class Fc3BoneUnit : public Fc3ComponentUnit {
 protected:
  static constexpr std::uint32_t kBoneArray{0x2800};
  static constexpr std::uint32_t kBones{3};
  static constexpr std::uint32_t kHeadSlot{2};

  void SetUp() override {
    Fc3ComponentUnit::SetUp();
    if (page_ == nullptr) {
      return;
    }

    Poke(kComponentTwo + afw::games::fc3::kComponentBoneArray, At(kBoneArray));
    Poke(kComponentTwo + afw::games::fc3::kComponentBoneCount, kBones);

    PokeBone(0, 0x11111111, 0.0F, 0.0F, 0.0F);
    PokeBone(1, 0x22222222, 0.0F, 0.0F, 0.9F);
    PokeBone(kHeadSlot, afw::games::fc3::kHeadBoneKey, 0.02F, 0.0F, 1.5F);
  }

  void PokeBone(std::uint32_t slot, std::uint32_t key, float x, float y,
                float z) {
    const std::uintptr_t kEntry{kBoneArray +
                                (slot * afw::games::fc3::kBoneStride)};
    Poke(kEntry, key);
    PokeFloat(kEntry + afw::games::fc3::kBonePosePosition, x);
    PokeFloat(kEntry + afw::games::fc3::kBonePosePosition + 4, y);
    PokeFloat(kEntry + afw::games::fc3::kBonePosePosition + 8, z);
  }
};

TEST_F(Fc3BoneUnit, PlacesTheHeadBoneInTheWorld) {
  const auto kHead{afw::games::fc3::ReadBonePoints(At(kEntity)).head};

  ASSERT_TRUE(kHead.has_value());
  EXPECT_FLOAT_EQ(kHead->x, 7.02F);
  EXPECT_FLOAT_EQ(kHead->y, 8.0F);
  EXPECT_FLOAT_EQ(kHead->z, 10.5F);
}

// Vehicles carry a skeleton and no head in it, so a miss is the ordinary answer
// rather than a failure.
TEST_F(Fc3BoneUnit, FindsNothingWhenTheSkeletonHasNoHead) {
  PokeBone(kHeadSlot, 0x33333333, 0.0F, 0.0F, 1.5F);

  EXPECT_FALSE(afw::games::fc3::ReadBonePoints(At(kEntity)).head.has_value());
}

// A head that lands a world away came from a matrix belonging to something
// else. One stale component already put an entity 118 m from itself.
TEST_F(Fc3BoneUnit, RefusesAHeadTooFarFromItsOwnBody) {
  PokeBone(kHeadSlot, afw::games::fc3::kHeadBoneKey, 0.0F, 0.0F, 40.0F);

  EXPECT_FALSE(afw::games::fc3::ReadBonePoints(At(kEntity)).head.has_value());
}

// The spine comes out of the same walk as the head, and an animal is read for
// it alone: nothing else on one is both reliably present and inside the body.
TEST_F(Fc3BoneUnit, PlacesTheSpineBoneInTheWorld) {
  PokeBone(1, afw::games::fc3::kSpineBoneKey, 0.0F, 0.0F, 0.75F);

  const auto kBones{afw::games::fc3::ReadBonePoints(At(kEntity))};

  ASSERT_TRUE(kBones.body.has_value());
  EXPECT_FLOAT_EQ(kBones.body->z, 9.75F);
  EXPECT_TRUE(kBones.head.has_value());
}

// An animal has a spine and no head bone under that name, and must still come
// back with the half that exists rather than with nothing at all.
TEST_F(Fc3BoneUnit, KeepsTheSpineWhenThereIsNoHead) {
  PokeBone(1, afw::games::fc3::kSpineBoneKey, 0.0F, 0.0F, 0.75F);
  PokeBone(kHeadSlot, 0x33333333, 0.0F, 0.0F, 1.5F);

  const auto kBones{afw::games::fc3::ReadBonePoints(At(kEntity))};

  EXPECT_FALSE(kBones.head.has_value());
  ASSERT_TRUE(kBones.body.has_value());
  EXPECT_FLOAT_EQ(kBones.body->z, 9.75F);
}

TEST_F(Fc3BoneUnit, RefusesAnImplausibleBoneCount) {
  Poke(kComponentTwo + afw::games::fc3::kComponentBoneCount, 100000);

  EXPECT_FALSE(afw::games::fc3::ReadBonePoints(At(kEntity)).head.has_value());
}

}  // namespace
