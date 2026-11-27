/*****************************************************************************
 * @file stack_test.cpp
 * @brief Tests stack ordering, lifecycle behavior, and value ownership.
 *****************************************************************************/

#include "systemcore/concurrency/containers/concurrent_stack.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <stop_token>
#include <string>
#include <utility>

namespace systemcore::concurrency::containers
{

TEST(ConcurrentStackTest, PushSupportsCopiesAndMovesInLifoOrder)
{
    ConcurrentStack<std::string> stack;
    const std::string first = "first";
    std::string second = "second";

    ASSERT_EQ(stack.push(first), ContainerStatus::kSuccess);
    ASSERT_EQ(stack.push(std::move(second)), ContainerStatus::kSuccess);

    EXPECT_EQ(stack.size(), 2);
    const auto newest = stack.tryPop();
    const auto oldest = stack.waitPop();

    ASSERT_EQ(newest.status, ContainerStatus::kSuccess);
    ASSERT_EQ(oldest.status, ContainerStatus::kSuccess);
    ASSERT_TRUE(newest);
    ASSERT_TRUE(oldest);
    EXPECT_EQ(*newest.value, "second");
    EXPECT_EQ(*oldest.value, "first");
    EXPECT_TRUE(stack.empty());
}

TEST(ConcurrentStackTest, SupportsMoveOnlyValues)
{
    ConcurrentStack<std::unique_ptr<int>> stack;
    ASSERT_EQ(stack.push(std::make_unique<int>(42)), ContainerStatus::kSuccess);

    const auto result = stack.tryPop();

    ASSERT_TRUE(result);
    ASSERT_NE(*result.value, nullptr);
    EXPECT_EQ(**result.value, 42);
}

TEST(ConcurrentStackTest, CloseRejectsPushesAndDrainsExistingValues)
{
    ConcurrentStack<int> stack;
    ASSERT_EQ(stack.push(1), ContainerStatus::kSuccess);
    ASSERT_EQ(stack.push(2), ContainerStatus::kSuccess);

    stack.close();

    EXPECT_TRUE(stack.isClosed());
    EXPECT_EQ(stack.push(3), ContainerStatus::kClosed);

    const auto newest = stack.waitPop();
    const auto oldest = stack.waitPop();
    ASSERT_EQ(newest.status, ContainerStatus::kSuccess);
    ASSERT_EQ(oldest.status, ContainerStatus::kSuccess);
    EXPECT_EQ(*newest.value, 2);
    EXPECT_EQ(*oldest.value, 1);
    EXPECT_EQ(stack.waitPop().status, ContainerStatus::kClosed);
    EXPECT_EQ(stack.tryPop().status, ContainerStatus::kClosed);
}

TEST(ConcurrentStackTest, WaitPopReturnsStoppedWhenCancelled)
{
    ConcurrentStack<int> stack;
    std::stop_source stop_source;
    stop_source.request_stop();

    EXPECT_EQ(
        stack.waitPop(stop_source.get_token()).status, ContainerStatus::kStopped);
    EXPECT_FALSE(stack.isClosed());
}

} // namespace systemcore::concurrency::containers
