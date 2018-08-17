#include "gtest/gtest.h"



static void test() {
    EXPECT_EQ(1, 1) << "random failure test";

}
static void test2() {
	printf("start random success...\n");
    EXPECT_EQ(1, 1) << "random success test";

}

TEST(SerialTest, RandomTest) {
	test();
}

TEST(SerialTest, RandomTestSuccess) {
	test2();
}
