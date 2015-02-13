
#include <core/tree.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>


TEST(treeTest, isConstructible)
{
    EXPECT_NO_THROW(
    {
        tree<int> tr;
    });
}



TEST(treeTest, isConstructedEmpty)
{
    tree<int> tr;

    EXPECT_EQ(true, tr.empty());
    EXPECT_EQ(true, tr.end() == tr.begin());
    EXPECT_EQ(0, tr.end() - tr.begin());
}


TEST(treeTest, acceptsRootLevelInserts)
{
    tree<int> tr;

    tr.insert(tr.begin(), 1);

    EXPECT_EQ(1, tr.end() - tr.begin());
}
