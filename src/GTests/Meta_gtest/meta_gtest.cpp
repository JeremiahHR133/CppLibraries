#include <Meta/Meta.h>

#include <string>
#include <sstream>
#include <fstream>

#include <Converter/Converter.h>
#include <gtest/gtest.h>

class TestMetaBase : public Meta::MetaObject
{
	DECLARE_META_OBJECT(TestMetaBase)
public:

	TestMetaBase() = default;
	virtual ~TestMetaBase() = default;

	virtual void test() = 0;

private:
	char m_char = 'C';
};

IMPLEMENT_META_OBJECT(TestMetaBase)
{
	w.addMember<&TestMetaBase::m_char>("char")
		.setDefault('D')
		.setDescription("Char member property")
		.setReadOnly();
}

class TestMetaObject : public TestMetaBase
{
	DECLARE_META_OBJECT(TestMetaObject)
public:

	struct Example
	{
		int one = 5;
		int two = 10;
	};

	TestMetaObject() = default;
	~TestMetaObject() = default;

	void set_m_int(int i) { m_int = i; }
	int get_m_int() const { return m_int; }

	Example memberFunction(int i) const { return Example{(m_int * 10) + i, (m_int * 20) + i}; }
	void otherMemberFunc() { m_string = "MODIFIED"; }

private:
	int m_int = 10;
	bool m_bool = true;
	std::string m_string = "STRING";
	float m_float = -5.26f;
};

IMPLEMENT_META_OBJECT(TestMetaObject)
{
	w.addMember<&TestMetaObject::set_m_int, &TestMetaObject::get_m_int>("int")
		.setDefault(20)
		.setDescription("Integer member property (setter/getter protected)");

	w.addMember<&TestMetaObject::m_bool>("bool")
		.setDefault(false)
		.setDescription("Bool member property");

	w.addMember<&TestMetaObject::m_string>("string")
		.setDefault("NOT STRING")
		.setDescription("String member property");

	w.addMember<&TestMetaObject::m_float>("float")
		.setDefault(100.1f)
		.setDescription("Float member property");

	w.addFunction<&TestMetaObject::memberFunction>("memberFunction")
		.setDefaultArgs(2)
		.setDescription("Const member function");

	w.addFunction<&TestMetaObject::otherMemberFunc>("otherMemberFunc")
		.setDescription("Non-const member function");
}

class MetaTest : public testing::Test
{
protected:
	MetaTest() = default;
	~MetaTest() override = default;

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
		Meta::initializeMetaInfo();
	}

	static void TearDownTestSuite()
	{
		std::ofstream outfile("Meta_gtest.log");
		if (outfile.is_open())
			outfile << s_stream.str();
	}

	static std::stringstream s_stream;
};

std::stringstream MetaTest::s_stream{};

TEST_F(MetaTest, ClassMetaPointer)
{
	auto* metaTemplated = Meta::getClassMeta<TestMetaObject>();
	auto* metaNamed = Meta::getClassMeta("TestMetaObject");
	auto* metaIndexed = Meta::getClassMeta(std::type_index(typeid(TestMetaObject)));

	ASSERT_TRUE(metaTemplated);
	ASSERT_TRUE(metaNamed);
	ASSERT_TRUE(metaIndexed);

	EXPECT_EQ(metaTemplated, metaNamed);
	EXPECT_EQ(metaNamed, metaIndexed);
}
