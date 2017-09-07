#include <gtest/gtest.h>

#include "subprog.h"

class SubprogramFixture : public ::testing::Test {
public:
    SubprogramFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~SubprogramFixture()
    {
    }
};

TEST_F(SubprogramFixture, echo)
{
    Environment env;
    Subprogram prog("cmd");

    ASSERT_TRUE( prog.start({"/c echo fuga&:"}, env) );
    auto& input = prog.inputStream();
    std::string buf;
    try {
        while (true)
        {
            buf += input.readChar();
        }
    } catch (StreamException&)
    {
    }
    int status;
    ASSERT_TRUE( prog.wait(&status) );
    EXPECT_EQ(0, status);
    EXPECT_EQ(buf, "fuga\r\n");
}
