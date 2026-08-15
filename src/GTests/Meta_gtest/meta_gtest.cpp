#include <Meta/Meta.h>

#include <fstream>
#include <sstream>
#include <string>

#include <Converter/Converter.h>
#include <gtest/gtest.h>

class TestMetaDoubleBase : public Meta::MetaObject
{
    DECLARE_META_OBJECT(TestMetaDoubleBase)
public:
    TestMetaDoubleBase() = default;
    virtual ~TestMetaDoubleBase() = default;

    int aFunc(int val) const
    {
        return val * 2;
    }
    int bFunc(int val)
    {
        return val / 2;
    }

private:
    std::string m_description = "I am the real base.";
};

IMPLEMENT_META_OBJECT(TestMetaDoubleBase)
{
    w.addMember<&TestMetaDoubleBase::m_description>("description")
        .setDescription("A description about a description")
        .setReadOnly();

    w.addFunction<&TestMetaDoubleBase::aFunc>("aFunc").setDefaultArgs(20).setDescription("A random function");

    w.addFunction<&TestMetaDoubleBase::bFunc>("bFunc").setDefaultArgs(20).setDescription("A second random function");
}

class TestMetaBase : public TestMetaDoubleBase
{
    DECLARE_META_OBJECT(TestMetaBase, TestMetaDoubleBase)
public:
    TestMetaBase() = default;
    virtual ~TestMetaBase() = default;

    virtual void test() = 0;

private:
    char m_char = 'C';
};

IMPLEMENT_META_OBJECT(TestMetaBase)
{
    w.addMember<&TestMetaBase::m_char>("char").setDefault('D').setDescription("Char member property").setReadOnly();
}

class TestMetaObject : public TestMetaBase
{
    DECLARE_META_OBJECT(TestMetaObject, TestMetaBase)
public:
    struct Example
    {
        int one = 5;
        int two = 10;

        bool operator==(const Example &other) const
        {
            return one == other.one && two == other.two;
        }
    };

    TestMetaObject() = default;
    ~TestMetaObject() = default;

    void set_m_int(int i)
    {
        m_int = i;
    }
    int get_m_int() const
    {
        return m_int;
    }

    Example memberFunction(int i) const
    {
        return Example{(m_int * 10) + i, (m_int * 20) + i};
    }
    void otherMemberFunc()
    {
        m_string = "MODIFIED";
    }

    void test() override
    {
    }

private:
    int m_int = 10;
    bool m_bool = true;
    std::string m_string = "STRING";
    float m_float = -5.26f;
};

IMPLEMENT_META_OBJECT(TestMetaObject)
{
    w.addMember<&TestMetaObject::set_m_int, &TestMetaObject::get_m_int>("int").setDefault(20).setDescription(
        "Integer member property (setter/getter protected)");

    w.addMember<&TestMetaObject::m_bool>("bool").setDefault(false).setDescription("Bool member property");

    w.addMember<&TestMetaObject::m_string>("string").setDefault("NOT STRING").setDescription("String member property");

    w.addMember<&TestMetaObject::m_float>("float").setDefault(100.1f).setDescription("Float member property");

    w.addFunction<&TestMetaObject::memberFunction>("memberFunction")
        .setDefaultArgs(2)
        .setDescription("Const member function");

    w.addFunction<&TestMetaObject::otherMemberFunc>("otherMemberFunc").setDescription("Non-const member function");
}

class MetaTest : public testing::Test
{
protected:
    MetaTest() = default;
    ~MetaTest() override = default;

    static void SetUpTestSuite()
    {
        Log::LogInitOptions lInit{
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

#define GET_META_POINTERS()                                                                                            \
    auto *metaDoubleBase = Meta::getClassMeta<TestMetaDoubleBase>();                                                   \
    auto *metaBase = Meta::getClassMeta<TestMetaBase>();                                                               \
    auto *metaDerived = Meta::getClassMeta<TestMetaObject>();                                                          \
    ASSERT_TRUE(metaDoubleBase);                                                                                       \
    ASSERT_TRUE(metaBase);                                                                                             \
    ASSERT_TRUE(metaDerived);

TEST_F(MetaTest, ClassPointerIdentity)
{
    // Concrete type
    {
        auto *metaTemplated = Meta::getClassMeta<TestMetaObject>();
        auto *metaNamed = Meta::getClassMeta("TestMetaObject");
        auto *metaIndexed = Meta::getClassMeta(std::type_index(typeid(TestMetaObject)));

        ASSERT_TRUE(metaTemplated);
        ASSERT_TRUE(metaNamed);
        ASSERT_TRUE(metaIndexed);

        EXPECT_EQ(metaTemplated, metaNamed);
        EXPECT_EQ(metaNamed, metaIndexed);
    }

    // Abstract type
    {
        auto *metaTemplated = Meta::getClassMeta<TestMetaBase>();
        auto *metaNamed = Meta::getClassMeta("TestMetaBase");
        auto *metaIndexed = Meta::getClassMeta(std::type_index(typeid(TestMetaBase)));

        ASSERT_TRUE(metaTemplated);
        ASSERT_TRUE(metaNamed);
        ASSERT_TRUE(metaIndexed);

        EXPECT_EQ(metaTemplated, metaNamed);
        EXPECT_EQ(metaNamed, metaIndexed);
    }

    // Base type
    {
        auto *metaTemplated = Meta::getClassMeta<TestMetaDoubleBase>();
        auto *metaNamed = Meta::getClassMeta("TestMetaDoubleBase");
        auto *metaIndexed = Meta::getClassMeta(std::type_index(typeid(TestMetaDoubleBase)));

        ASSERT_TRUE(metaTemplated);
        ASSERT_TRUE(metaNamed);
        ASSERT_TRUE(metaIndexed);

        EXPECT_EQ(metaTemplated, metaNamed);
        EXPECT_EQ(metaNamed, metaIndexed);
    }
}

TEST_F(MetaTest, PropCount)
{
    GET_META_POINTERS();

    auto getMemberCount = [](const Meta::ClassMetaBase *meta) {
        int count = 0;
        meta->forAllMemberProps([&count](const Meta::MemberPropertyBase *prop) -> Meta::ClassMetaBase::InvokeResult {
            count++;
            return Meta::ClassMetaBase::InvokeResult::CONTINUE;
        });
        return count;
    };
    auto getConstFuncCount = [](const Meta::ClassMetaBase *meta) {
        int count = 0;
        meta->forAllConstMemberFuncs(
            [&count](const Meta::MemberConstFunctionPropBase *prop) -> Meta::ClassMetaBase::InvokeResult {
                count++;
                return Meta::ClassMetaBase::InvokeResult::CONTINUE;
            });
        return count;
    };
    auto getNonConstFuncCount = [](const Meta::ClassMetaBase *meta) {
        int count = 0;
        meta->forAllNonConstMemberFuncs(
            [&count](const Meta::MemberNonConstFunctionPropBase *prop) -> Meta::ClassMetaBase::InvokeResult {
                count++;
                return Meta::ClassMetaBase::InvokeResult::CONTINUE;
            });
        return count;
    };
    EXPECT_EQ(getMemberCount(metaDoubleBase), 1);
    EXPECT_EQ(getMemberCount(metaBase), 2);
    EXPECT_EQ(getMemberCount(metaDerived), 6);

    EXPECT_EQ(getConstFuncCount(metaDoubleBase), 1);
    EXPECT_EQ(getConstFuncCount(metaBase), 1);
    EXPECT_EQ(getConstFuncCount(metaDerived), 2);

    EXPECT_EQ(getNonConstFuncCount(metaDoubleBase), 1);
    EXPECT_EQ(getNonConstFuncCount(metaBase), 1);
    EXPECT_EQ(getNonConstFuncCount(metaDerived), 2);
}

TEST_F(MetaTest, HelperFuncs)
{
    GET_META_POINTERS();

    EXPECT_EQ(metaDoubleBase->getName(), "TestMetaDoubleBase");
    EXPECT_EQ(metaBase->getName(), "TestMetaBase");
    EXPECT_EQ(metaDerived->getName(), "TestMetaObject");

    EXPECT_EQ(metaDoubleBase->getTypeIndex(), std::type_index(typeid(TestMetaDoubleBase)));
    EXPECT_EQ(metaBase->getTypeIndex(), std::type_index(typeid(TestMetaBase)));
    EXPECT_EQ(metaDerived->getTypeIndex(), std::type_index(typeid(TestMetaObject)));

    EXPECT_EQ(metaDoubleBase->getParentName(), "Meta::Impl::NoParent");
    EXPECT_EQ(metaBase->getParentName(), metaDoubleBase->getName());
    EXPECT_EQ(metaDerived->getParentName(), metaBase->getName());

    EXPECT_EQ(metaDerived->getParent(), metaBase);
    EXPECT_EQ(metaBase->getParent(), metaDoubleBase);
}

TEST_F(MetaTest, MemberProps)
{
    GET_META_POINTERS();

    TestMetaObject testObj;
    // string - DoubleBaseClass prop
    {
        auto *prop = metaBase->getMemberProp("description");
        ASSERT_TRUE(prop);

        EXPECT_EQ(prop->getClassName(), metaDoubleBase->getName());
        EXPECT_EQ(prop->getDescription(), "A description about a description");
        EXPECT_EQ(prop->getName(), "description");
        EXPECT_EQ(prop->getReadOnly(), true);
        EXPECT_EQ(prop->getTypeIndex(), std::type_index(typeid(std::string)));
        EXPECT_EQ(prop->hasDefault(), false);

        EXPECT_EQ(std::any_cast<std::string>(prop->getAsAny(testObj)), "I am the real base.");
        EXPECT_EQ(prop->getAsType<std::string>(testObj), "I am the real base.");
    }

    // char - Base class prop
    {
        auto *prop = metaBase->getMemberProp("char");
        ASSERT_TRUE(prop);

        EXPECT_EQ(prop->getClassName(), metaBase->getName());
        EXPECT_EQ(prop->getDescription(), "Char member property");
        EXPECT_EQ(prop->getName(), "char");
        EXPECT_EQ(prop->getReadOnly(), true);
        EXPECT_EQ(prop->getTypeIndex(), std::type_index(typeid(char)));
        EXPECT_EQ(prop->hasDefault(), true);
        EXPECT_EQ(std::any_cast<char>(prop->createDefaultAsAny()), 0);

        EXPECT_EQ(std::any_cast<char>(prop->getAsAny(testObj)), 'C');
        EXPECT_EQ(prop->getAsType<char>(testObj), 'C');

        // TODO: Need to see how I want to test debug assertions and release
        // handling
        // prop->applyDefault(testObj);
        // EXPECT_EQ(std::any_cast<char>(prop->getAsAny(testObj)), 'D');
        // EXPECT_EQ(prop->getAsType<char>(testObj), 'D');
        // prop->setFromAny(testObj, std::any('T'));
        // EXPECT_EQ(std::any_cast<char>(prop->getAsAny(testObj)), 'T');
        // EXPECT_EQ(prop->getAsType<char>(testObj), 'T');
    }

    // int - Derived class prop
    {
        auto *prop = metaDerived->getMemberProp("int");
        ASSERT_TRUE(prop);

        EXPECT_EQ(prop->getClassName(), metaDerived->getName());
        EXPECT_EQ(prop->getDescription(), "Integer member property (setter/getter protected)");
        EXPECT_EQ(prop->getName(), "int");
        EXPECT_EQ(prop->getReadOnly(), false);
        EXPECT_EQ(prop->getTypeIndex(), std::type_index(typeid(int)));
        EXPECT_EQ(prop->hasDefault(), true);
        EXPECT_EQ(std::any_cast<int>(prop->createDefaultAsAny()), 0);

        EXPECT_EQ(std::any_cast<int>(prop->getAsAny(testObj)), 10);
        EXPECT_EQ(prop->getAsType<int>(testObj), 10);

        prop->applyDefault(testObj);
        EXPECT_EQ(std::any_cast<int>(prop->getAsAny(testObj)), 20);
        EXPECT_EQ(prop->getAsType<int>(testObj), 20);

        prop->setFromAny(testObj, std::any(30));
        EXPECT_EQ(std::any_cast<int>(prop->getAsAny(testObj)), 30);
        EXPECT_EQ(prop->getAsType<int>(testObj), 30);
    }

    // bool - Derived class prop
    {
        auto *prop = metaDerived->getMemberProp("bool");
        ASSERT_TRUE(prop);

        EXPECT_EQ(prop->getClassName(), metaDerived->getName());
        EXPECT_EQ(prop->getDescription(), "Bool member property");
        EXPECT_EQ(prop->getName(), "bool");
        EXPECT_EQ(prop->getReadOnly(), false);
        EXPECT_EQ(prop->getTypeIndex(), std::type_index(typeid(bool)));
        EXPECT_EQ(prop->hasDefault(), true);
        EXPECT_EQ(std::any_cast<bool>(prop->createDefaultAsAny()), false);

        EXPECT_EQ(std::any_cast<bool>(prop->getAsAny(testObj)), true);
        EXPECT_EQ(prop->getAsType<bool>(testObj), true);

        prop->applyDefault(testObj);
        EXPECT_EQ(std::any_cast<bool>(prop->getAsAny(testObj)), false);
        EXPECT_EQ(prop->getAsType<bool>(testObj), false);

        prop->setFromAny(testObj, std::any(true));
        EXPECT_EQ(std::any_cast<bool>(prop->getAsAny(testObj)), true);
        EXPECT_EQ(prop->getAsType<bool>(testObj), true);
    }

    // string - Derived class prop
    {
        auto *prop = metaDerived->getMemberProp("string");
        ASSERT_TRUE(prop);

        EXPECT_EQ(prop->getClassName(), metaDerived->getName());
        EXPECT_EQ(prop->getDescription(), "String member property");
        EXPECT_EQ(prop->getName(), "string");
        EXPECT_EQ(prop->getReadOnly(), false);
        EXPECT_EQ(prop->getTypeIndex(), std::type_index(typeid(std::string)));
        EXPECT_EQ(prop->hasDefault(), true);
        EXPECT_EQ(std::any_cast<std::string>(prop->createDefaultAsAny()), "");

        EXPECT_EQ(std::any_cast<std::string>(prop->getAsAny(testObj)), "STRING");
        EXPECT_EQ(prop->getAsType<std::string>(testObj), "STRING");

        prop->applyDefault(testObj);
        EXPECT_EQ(std::any_cast<std::string>(prop->getAsAny(testObj)), "NOT STRING");
        EXPECT_EQ(prop->getAsType<std::string>(testObj), "NOT STRING");

        prop->setFromAny(testObj, std::any(std::string("YES STRING")));
        EXPECT_EQ(std::any_cast<std::string>(prop->getAsAny(testObj)), "YES STRING");
        EXPECT_EQ(prop->getAsType<std::string>(testObj), "YES STRING");
    }

    // float - Derived class prop
    {
        auto *prop = metaDerived->getMemberProp("float");
        ASSERT_TRUE(prop);

        EXPECT_EQ(prop->getClassName(), metaDerived->getName());
        EXPECT_EQ(prop->getDescription(), "Float member property");
        EXPECT_EQ(prop->getName(), "float");
        EXPECT_EQ(prop->getReadOnly(), false);
        EXPECT_EQ(prop->getTypeIndex(), std::type_index(typeid(float)));
        EXPECT_EQ(prop->hasDefault(), true);
        EXPECT_EQ(std::any_cast<float>(prop->createDefaultAsAny()), 0.0f);

        EXPECT_EQ(std::any_cast<float>(prop->getAsAny(testObj)), -5.26f);
        EXPECT_EQ(prop->getAsType<float>(testObj), -5.26f);

        prop->applyDefault(testObj);
        EXPECT_EQ(std::any_cast<float>(prop->getAsAny(testObj)), 100.1f);
        EXPECT_EQ(prop->getAsType<float>(testObj), 100.1f);

        prop->setFromAny(testObj, std::any(451.0f));
        EXPECT_EQ(std::any_cast<float>(prop->getAsAny(testObj)), 451.0f);
        EXPECT_EQ(prop->getAsType<float>(testObj), 451.0f);
    }
}

TEST_F(MetaTest, FunctionProps)
{
    GET_META_POINTERS();

    TestMetaObject testObj;

    // memberFunction - DoubleBaseClass prop
    {
        auto *prop = metaDoubleBase->getConstFunc("aFunc");
        ASSERT_TRUE(prop);

        EXPECT_EQ(prop->getClassName(), metaDoubleBase->getName());
        EXPECT_EQ(prop->getDescription(), "A random function");
        EXPECT_EQ(prop->getName(), "aFunc");
        EXPECT_EQ(prop->getTypeIndex(), std::type_index(typeid(int)));

        int expected = 20;
        EXPECT_EQ(std::any_cast<int>(prop->invoke(testObj, {10})), expected);
        EXPECT_EQ(prop->invokeAsType<int>(testObj, {10}), expected);

        int expected2 = 40;
        EXPECT_EQ(std::any_cast<int>(prop->invokeDefaultArgs(testObj)), expected2);
        EXPECT_EQ(prop->invokeDefaultArgsAsType<int>(testObj), expected2);
    }

    // memberFunction - DoubleBaseClass prop
    {
        auto *prop = metaDoubleBase->getNonConstFunc("bFunc");
        ASSERT_TRUE(prop);

        EXPECT_EQ(prop->getClassName(), metaDoubleBase->getName());
        EXPECT_EQ(prop->getDescription(), "A second random function");
        EXPECT_EQ(prop->getName(), "bFunc");
        EXPECT_EQ(prop->getTypeIndex(), std::type_index(typeid(int)));

        int expected = 20;
        EXPECT_EQ(std::any_cast<int>(prop->invoke(testObj, {40})), expected);
        EXPECT_EQ(prop->invokeAsType<int>(testObj, {40}), expected);

        int expected2 = 10;
        EXPECT_EQ(std::any_cast<int>(prop->invokeDefaultArgs(testObj)), expected2);
        EXPECT_EQ(prop->invokeDefaultArgsAsType<int>(testObj), expected2);
    }

    // memberFunction - Derived class prop
    {
        auto *prop = metaDerived->getConstFunc("memberFunction");
        ASSERT_TRUE(prop);

        EXPECT_EQ(prop->getClassName(), metaDerived->getName());
        EXPECT_EQ(prop->getDescription(), "Const member function");
        EXPECT_EQ(prop->getName(), "memberFunction");
        EXPECT_EQ(prop->getTypeIndex(), std::type_index(typeid(TestMetaObject::Example)));

        TestMetaObject::Example expected{105, 205};
        EXPECT_EQ(std::any_cast<TestMetaObject::Example>(prop->invoke(testObj, {5})), expected);
        EXPECT_EQ(prop->invokeAsType<TestMetaObject::Example>(testObj, {5}), expected);

        TestMetaObject::Example expected2{102, 202};
        EXPECT_EQ(std::any_cast<TestMetaObject::Example>(prop->invokeDefaultArgs(testObj)), expected2);
        EXPECT_EQ(prop->invokeDefaultArgsAsType<TestMetaObject::Example>(testObj), expected2);
    }

    // otherMemberFunc - Derived class prop
    {
        auto *prop = metaDerived->getNonConstFunc("otherMemberFunc");
        ASSERT_TRUE(prop);

        EXPECT_EQ(prop->getClassName(), metaDerived->getName());
        EXPECT_EQ(prop->getDescription(), "Non-const member function");
        EXPECT_EQ(prop->getName(), "otherMemberFunc");
        EXPECT_EQ(prop->getTypeIndex(), std::type_index(typeid(void)));

        prop->invoke(testObj, {});

        auto *strProp = metaDerived->getMemberProp("string");
        ASSERT_TRUE(strProp);
        EXPECT_EQ(std::any_cast<std::string>(strProp->getAsAny(testObj)), "MODIFIED");
        EXPECT_EQ(strProp->getAsType<std::string>(testObj), "MODIFIED");
    }
}
