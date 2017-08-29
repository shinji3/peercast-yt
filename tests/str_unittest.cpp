﻿#include <gtest/gtest.h>

#include "str.h"
using namespace str;

class strFixture : public ::testing::Test {
public:
};

TEST_F(strFixture, group_digits)
{
    ASSERT_STREQ("0", group_digits("0").c_str());
    ASSERT_STREQ("1", group_digits("1").c_str());
    ASSERT_STREQ("1.0", group_digits("1.0").c_str());
    ASSERT_STREQ("1.1", group_digits("1.1").c_str());
    ASSERT_STREQ("1.1234", group_digits("1.1234").c_str());
    ASSERT_STREQ("123,456", group_digits("123456").c_str());
    ASSERT_STREQ("123,456.789123", group_digits("123456.789123").c_str());

    ASSERT_STREQ("1",             group_digits("1").c_str());
    ASSERT_STREQ("11",            group_digits("11").c_str());
    ASSERT_STREQ("111",           group_digits("111").c_str());
    ASSERT_STREQ("1,111",         group_digits("1111").c_str());
    ASSERT_STREQ("11,111",        group_digits("11111").c_str());
    ASSERT_STREQ("111,111",       group_digits("111111").c_str());
    ASSERT_STREQ("1,111,111",     group_digits("1111111").c_str());
    ASSERT_STREQ("11,111,111",    group_digits("11111111").c_str());
    ASSERT_STREQ("111,111,111",   group_digits("111111111").c_str());
    ASSERT_STREQ("1,111,111,111", group_digits("1111111111").c_str());
}

TEST_F(strFixture, split)
{
    auto vec = split("a b c", " ");

    ASSERT_EQ(3, vec.size());
    ASSERT_STREQ("a", vec[0].c_str());
    ASSERT_STREQ("b", vec[1].c_str());
    ASSERT_STREQ("c", vec[2].c_str());
}

TEST_F(strFixture, codepoint_to_utf8)
{
    ASSERT_STREQ(" ", codepoint_to_utf8(0x20).c_str());
    ASSERT_STREQ("\xE3\x81\x82", codepoint_to_utf8(12354).c_str());
    ASSERT_STREQ("\xf0\x9f\x92\xa9", codepoint_to_utf8(0x1f4a9).c_str()); // PILE OF POO
}

TEST_F(strFixture, format)
{
    EXPECT_STREQ("a", format("a").c_str());
    EXPECT_STREQ("a", format("%s", "a").c_str());
    EXPECT_STREQ("1", format("%d", 1).c_str());
    EXPECT_STREQ("12", format("%d%d", 1, 2).c_str());
}

TEST_F(strFixture, contains)
{
    ASSERT_TRUE(str::contains("abc", "bc"));
    ASSERT_FALSE(str::contains("abc", "d"));
    ASSERT_TRUE(str::contains("abc", ""));
    ASSERT_TRUE(str::contains("", ""));
    ASSERT_TRUE(str::contains("abc", "abc"));
    ASSERT_FALSE(str::contains("", "abc"));
}

TEST_F(strFixture, replace_prefix)
{
    ASSERT_STREQ("", str::replace_prefix("", "", "").c_str());
    ASSERT_STREQ("b", str::replace_prefix("", "", "b").c_str());
    ASSERT_STREQ("", str::replace_prefix("", "a", "b").c_str());
    ASSERT_STREQ("", str::replace_prefix("", "a", "").c_str());
    ASSERT_STREQ("xbc", str::replace_prefix("abc", "a", "x").c_str());
    ASSERT_STREQ("abc", str::replace_prefix("abc", "x", "x").c_str());
    ASSERT_STREQ("xabc", str::replace_prefix("abc", "", "x").c_str());
    ASSERT_STREQ("bc", str::replace_prefix("abc", "a", "").c_str());
}

TEST_F(strFixture, replace_suffix)
{
    ASSERT_STREQ("", str::replace_suffix("", "", "").c_str());
    ASSERT_STREQ("b", str::replace_suffix("", "", "b").c_str());
    ASSERT_STREQ("", str::replace_suffix("", "a", "b").c_str());
    ASSERT_STREQ("", str::replace_suffix("", "a", "").c_str());
    ASSERT_STREQ("abx", str::replace_suffix("abc", "c", "x").c_str());
    ASSERT_STREQ("abc", str::replace_suffix("abc", "x", "x").c_str());
    ASSERT_STREQ("abcx", str::replace_suffix("abc", "", "x").c_str());
    ASSERT_STREQ("ab", str::replace_suffix("abc", "c", "").c_str());
}

TEST_F(strFixture, capitalize)
{
    ASSERT_STREQ("", str::capitalize("").c_str());
    ASSERT_STREQ("A", str::capitalize("a").c_str());
    ASSERT_STREQ("@", str::capitalize("@").c_str());
    ASSERT_STREQ("Abc", str::capitalize("abc").c_str());
    ASSERT_STREQ("Abc", str::capitalize("ABC").c_str());
    ASSERT_STREQ("A@B", str::capitalize("a@b").c_str());
    ASSERT_STREQ("Content-Type", str::capitalize("CONTENT-TYPE").c_str());
    ASSERT_STREQ("Content-Type", str::capitalize("content-type").c_str());
    ASSERT_STREQ("Content-Type", str::capitalize("Content-Type").c_str());
    ASSERT_STREQ("あいうえお漢字αβγ", str::downcase("あいうえお漢字αβγ").c_str());
}

TEST_F(strFixture, upcase)
{
    ASSERT_STREQ("", str::upcase("").c_str());
    ASSERT_STREQ("A", str::upcase("a").c_str());
    ASSERT_STREQ("@", str::upcase("@").c_str());
    ASSERT_STREQ("ABC", str::upcase("abc").c_str());
    ASSERT_STREQ("ABC", str::upcase("ABC").c_str());
    ASSERT_STREQ("A@B", str::upcase("a@b").c_str());
    ASSERT_STREQ("CONTENT-TYPE", str::upcase("CONTENT-TYPE").c_str());
    ASSERT_STREQ("CONTENT-TYPE", str::upcase("content-type").c_str());
    ASSERT_STREQ("CONTENT-TYPE", str::upcase("Content-Type").c_str());
    ASSERT_STREQ("あいうえお漢字αβγ", str::downcase("あいうえお漢字αβγ").c_str());
}

TEST_F(strFixture, downcase)
{
    ASSERT_STREQ("", str::downcase("").c_str());
    ASSERT_STREQ("a", str::downcase("a").c_str());
    ASSERT_STREQ("@", str::downcase("@").c_str());
    ASSERT_STREQ("abc", str::downcase("abc").c_str());
    ASSERT_STREQ("abc", str::downcase("ABC").c_str());
    ASSERT_STREQ("a@b", str::downcase("a@b").c_str());
    ASSERT_STREQ("content-type", str::downcase("CONTENT-TYPE").c_str());
    ASSERT_STREQ("content-type", str::downcase("content-type").c_str());
    ASSERT_STREQ("content-type", str::downcase("Content-Type").c_str());
    ASSERT_STREQ("あいうえお漢字αβγ", str::downcase("あいうえお漢字αβγ").c_str());
}

TEST_F(strFixture, is_prefix_of)
{
    ASSERT_TRUE(str::is_prefix_of("", ""));
    ASSERT_TRUE(str::is_prefix_of("", "a"));
    ASSERT_TRUE(str::is_prefix_of("a", "a"));
    ASSERT_TRUE(str::is_prefix_of("a", "abc"));
    ASSERT_FALSE(str::is_prefix_of("b", ""));
    ASSERT_FALSE(str::is_prefix_of("b", "a"));
    ASSERT_FALSE(str::is_prefix_of("b", "abc"));
    ASSERT_TRUE(str::is_prefix_of("abc", "abc"));
    ASSERT_TRUE(str::is_prefix_of("あ", "あいうえお"));
}

TEST_F(strFixture, has_suffix)
{
    ASSERT_TRUE(str::has_suffix("", ""));
    ASSERT_TRUE(str::has_suffix("a", ""));
    ASSERT_TRUE(str::has_suffix("a", "a"));
    ASSERT_TRUE(str::has_suffix("abc", "c"));
    ASSERT_FALSE(str::has_suffix("", "b"));
    ASSERT_FALSE(str::has_suffix("a", "b"));
    ASSERT_FALSE(str::has_suffix("abc", "b"));
    ASSERT_TRUE(str::has_suffix("abc", "abc"));
    ASSERT_TRUE(str::has_suffix("あいうえお", "お"));
}

TEST_F(strFixture, join)
{
    ASSERT_STREQ("", str::join("", {}).c_str());
    ASSERT_STREQ("", str::join(",", {}).c_str());
    ASSERT_STREQ("a,b", str::join(",", {"a", "b"}).c_str());
    ASSERT_STREQ("ab", str::join("", {"a", "b"}).c_str());
    ASSERT_STREQ("ab", str::join("", str::split("a,b", ",")).c_str());
}

TEST_F(strFixture, extension_without_dot)
{
    ASSERT_STREQ("",    str::extension_without_dot("").c_str());
    ASSERT_STREQ("flv", str::extension_without_dot("test.flv").c_str());
    ASSERT_STREQ("FLV", str::extension_without_dot("TEST.FLV").c_str());
    ASSERT_STREQ("",    str::extension_without_dot("test.").c_str());
    ASSERT_STREQ("gz",  str::extension_without_dot("hoge.tar.gz").c_str());
}

TEST_F(strFixture, count)
{
    ASSERT_THROW(str::count("", ""), std::domain_error);
    ASSERT_THROW(str::count("abc", ""), std::domain_error);

    ASSERT_EQ(0, str::count("", "a"));
    ASSERT_EQ(1, str::count("abc", "a"));
    ASSERT_EQ(2, str::count("aba", "a"));

    ASSERT_EQ(1, str::count("aba", "ab"));
    ASSERT_EQ(2, str::count("abab", "ab"));
    ASSERT_EQ(3, str::count("  ab   ab   ab  ", "ab"));
}

TEST_F(strFixture, rstrip)
{
    ASSERT_EQ("", str::rstrip(""));
    ASSERT_EQ("", str::rstrip(" "));
    ASSERT_EQ("", str::rstrip("\t\r\n"));
    ASSERT_EQ("a", str::rstrip("a"));
    ASSERT_EQ("a", str::rstrip("a "));
    ASSERT_EQ("a", str::rstrip("a\t\r\n"));
    ASSERT_EQ(" a", str::rstrip(" a "));
    ASSERT_EQ("a", str::rstrip({ 'a', '\0', '\n' }));
}

TEST_F(strFixture, escapeshellarg_unix)
{
    ASSERT_EQ("\'\'", str::escapeshellarg_unix(""));
    ASSERT_EQ("\'abc\'\"\'\"\'def\'", str::escapeshellarg_unix("abc\'def"));
    ASSERT_EQ("\'ａｂｃ\'\"\'\"\'ｄｅｆ\'", str::escapeshellarg_unix("ａｂｃ\'ｄｅｆ"));
}
