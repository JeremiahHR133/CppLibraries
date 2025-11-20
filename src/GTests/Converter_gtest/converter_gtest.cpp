#include <Converter/Converter.h>

#include <sstream>
#include <fstream>

#include <gtest/gtest.h>

struct ExampleType
{
	int first;
	float second;

	bool operator==(const ExampleType& other) const
	{
		return first == other.first && std::abs(second - other.second) < 1e-20;
	}

	static std::string toString(const ExampleType& t)
	{
		 return std::to_string(t.first) + "," + std::to_string(t.second); 
	}

	static ExampleType fromString(const std::string& str)
	{
		 ExampleType ret{ 0, 0.0f };
		 size_t idx = str.find(',');
		 if (idx == std::string::npos)
			 return ret;  

		 ret.first = std::stoi(str.substr(0, idx));
		 ret.second = std::stof(str.substr(idx + 1));

		 return ret;
	}
};

REGISTER_CONVERTER_FOR_TYPE(ExampleType,
	&ExampleType::toString,
	&ExampleType::fromString
)

class ConverterTest : public testing::Test
{
protected:
	ConverterTest() = default;
	~ConverterTest() override = default;

	static void SetUpTestSuite()
	{
		Log::LogInitOptions lInit
		{
			.printColor = false,
			.printLocationInfo = false,
			.reportLogInitialized = false,
			.logFullFunctionName = false,
			.logFullFilePath = false,
			.timeMode = Log::LogInitOptions::TimeMode::None,
		};
		Log::initLogging(s_stream, lInit);
		Converter::initializeConverters();
	}

	static void TearDownTestSuite()
	{
		std::ofstream outfile("Converter_gtest.log");
		if (outfile.is_open())
			outfile << s_stream.str();
	}

	static std::stringstream s_stream;
};

std::stringstream ConverterTest::s_stream{};

template<typename T>
bool typeIsRegistered()
{
	auto itr = std::find_if(Converter::Impl::getRegisteredConverters().begin(), Converter::Impl::getRegisteredConverters().end(),
		[](const Converter::ConverterInfo& info) { return info.index == std::type_index(typeid(T)); }
	);

	return itr != Converter::Impl::getRegisteredConverters().end();
}

#define TEST_INT_LIKE(type, suffix, min, max, funcName) \
	TEST_F(ConverterTest, funcName) \
	{ \
		ASSERT_TRUE(typeIsRegistered<type>()); \
	\
		type test = 0 ## suffix; \
		EXPECT_EQ(Converter::getTypeFromString<type>(Converter::getStringForType(test)), test); \
		EXPECT_EQ(Converter::getTypeFromString<type>(Converter::getStringFromAny<type>(std::any(test))), test); \
	 \
		test = 10 ## suffix; \
		EXPECT_EQ(Converter::getTypeFromString<type>(Converter::getStringForType(test)), test); \
		EXPECT_EQ(Converter::getTypeFromString<type>(Converter::getStringFromAny<type>(std::any(test))), test); \
	 \
		test = -10 ## suffix; \
		EXPECT_EQ(Converter::getTypeFromString<type>(Converter::getStringForType(test)), test); \
		EXPECT_EQ(Converter::getTypeFromString<type>(Converter::getStringFromAny<type>(std::any(test))), test); \
	 \
		test = min; \
		EXPECT_EQ(Converter::getTypeFromString<type>(Converter::getStringForType(test)), test); \
		EXPECT_EQ(Converter::getTypeFromString<type>(Converter::getStringFromAny<type>(std::any(test))), test); \
	 \
		test = max; \
		EXPECT_EQ(Converter::getTypeFromString<type>(Converter::getStringForType(test)), test); \
		EXPECT_EQ(Converter::getTypeFromString<type>(Converter::getStringFromAny<type>(std::any(test))), test); \
	}

TEST_INT_LIKE(int, 0, INT_MAX, INT_MIN, Test_int)
TEST_INT_LIKE(long, l, LONG_MAX, LONG_MIN, Test_long)
TEST_INT_LIKE(long long, ll, LLONG_MAX, LLONG_MIN, Test_long_long)
TEST_INT_LIKE(unsigned int, u, UINT_MAX, 0, Test_unsigned_int)
TEST_INT_LIKE(unsigned long, ul, ULONG_MAX, 0, Test_unsigned_long)
TEST_INT_LIKE(unsigned long long, ull, ULLONG_MAX, 0, Test_unsigned_long_long)

#define TEST_FLOAT_LIKE(type, suffix, max, min, funcName) \
	TEST_F(ConverterTest, funcName) \
	{ \
		ASSERT_TRUE(typeIsRegistered<type>()); \
 \
		type test = 0.0f; \
		type nearEnough = 1e-20; \
		EXPECT_NEAR(Converter::getTypeFromString<type>(Converter::getStringForType(test)), test, nearEnough); \
		EXPECT_NEAR(Converter::getTypeFromString<type>(Converter::getStringFromAny<type>(std::any(test))), test, nearEnough); \
 \
		test = 10.0f; \
		EXPECT_NEAR(Converter::getTypeFromString<type>(Converter::getStringForType(test)), test, nearEnough); \
		EXPECT_NEAR(Converter::getTypeFromString<type>(Converter::getStringFromAny<type>(std::any(test))), test, nearEnough); \
 \
		test = -10.0f; \
		EXPECT_NEAR(Converter::getTypeFromString<type>(Converter::getStringForType(test)), test, nearEnough); \
		EXPECT_NEAR(Converter::getTypeFromString<type>(Converter::getStringFromAny<type>(std::any(test))), test, nearEnough); \
 \
		test = max; \
		EXPECT_NEAR(Converter::getTypeFromString<type>(Converter::getStringForType(test)), test, nearEnough); \
		EXPECT_NEAR(Converter::getTypeFromString<type>(Converter::getStringFromAny<type>(std::any(test))), test, nearEnough); \
 \
		test = min; \
		EXPECT_NEAR(Converter::getTypeFromString<type>(Converter::getStringForType(test)), test, nearEnough); \
		EXPECT_NEAR(Converter::getTypeFromString<type>(Converter::getStringFromAny<type>(std::any(test))), test, nearEnough); \
	}

TEST_FLOAT_LIKE(float, f, FLT_MAX, FLT_MIN, Test_float)
TEST_FLOAT_LIKE(double, d, DBL_MAX, DBL_MIN, Test_double)
TEST_FLOAT_LIKE(long double, ld, DBL_MAX, DBL_MIN, Test_long_double)

TEST_F(ConverterTest, Test_bool)
{
	bool test = false;
	EXPECT_EQ(Converter::getTypeFromString<bool>(Converter::getStringForType(test)), test);
	EXPECT_EQ(Converter::getTypeFromString<bool>(Converter::getStringFromAny<bool>(std::any(test))), test);

	test = true;
	EXPECT_EQ(Converter::getTypeFromString<bool>(Converter::getStringForType(test)), test);
	EXPECT_EQ(Converter::getTypeFromString<bool>(Converter::getStringFromAny<bool>(std::any(test))), test);
}

TEST_F(ConverterTest, Test_string)
{
	ASSERT_TRUE(typeIsRegistered<std::string>());

	std::string test("");
	EXPECT_EQ(Converter::getTypeFromString<std::string>(Converter::getStringForType(test)), test);
	EXPECT_EQ(Converter::getTypeFromString<std::string>(Converter::getStringFromAny<std::string>(std::any(test))), test);

	test = "Some random data";
	EXPECT_EQ(Converter::getTypeFromString<std::string>(Converter::getStringForType(test)), test);
	EXPECT_EQ(Converter::getTypeFromString<std::string>(Converter::getStringFromAny<std::string>(std::any(test))), test);
}

TEST_F(ConverterTest, Test_char)
{
	ASSERT_TRUE(typeIsRegistered<char>());

	char test = 'a';
	EXPECT_EQ(Converter::getTypeFromString<char>(Converter::getStringForType(test)), test);
	EXPECT_EQ(Converter::getTypeFromString<char>(Converter::getStringFromAny<char>(std::any(test))), test);
}

TEST_F(ConverterTest, UserDefinedType)
{
	ASSERT_TRUE(typeIsRegistered<ExampleType>());
	ExampleType test{ 0, 0.0f };
	EXPECT_EQ(Converter::getTypeFromString<ExampleType>(Converter::getStringForType(test)), test);
	EXPECT_EQ(Converter::getTypeFromString<ExampleType>(Converter::getStringFromAny<ExampleType>(std::any(test))), test);

	test.first = 10;
	test.second = 10.0f;
	EXPECT_EQ(Converter::getTypeFromString<ExampleType>(Converter::getStringForType(test)), test);
	EXPECT_EQ(Converter::getTypeFromString<ExampleType>(Converter::getStringFromAny<ExampleType>(std::any(test))), test);

	test.first = -10;
	test.second = -10.0f;
	EXPECT_EQ(Converter::getTypeFromString<ExampleType>(Converter::getStringForType(test)), test);
	EXPECT_EQ(Converter::getTypeFromString<ExampleType>(Converter::getStringFromAny<ExampleType>(std::any(test))), test);

	test.first = INT_MAX;
	test.second = FLT_MAX;
	EXPECT_EQ(Converter::getTypeFromString<ExampleType>(Converter::getStringForType(test)), test);
	EXPECT_EQ(Converter::getTypeFromString<ExampleType>(Converter::getStringFromAny<ExampleType>(std::any(test))), test);

	test.first = INT_MIN;
	test.second = FLT_MIN;
	EXPECT_EQ(Converter::getTypeFromString<ExampleType>(Converter::getStringForType(test)), test);
	EXPECT_EQ(Converter::getTypeFromString<ExampleType>(Converter::getStringFromAny<ExampleType>(std::any(test))), test);
}

TEST_F(ConverterTest, MinConvertersRegistered)
{
	// 12 specified by Converter lib, 1 user defined type
	constexpr int numConverters = 12 + 1;
	ASSERT_GE(Converter::Impl::getRegisteredConverters().size(), numConverters) << "Some expected converters were not registered!";
	ASSERT_EQ(Converter::Impl::getRegisteredConverters().size(), numConverters) << "A default converter is not being accounted for in this test! Add it!!";
}

