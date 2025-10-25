#include <Logger/Logger.h>
#include <Converter/Converter.h>
#include <Meta/Meta.h>

#include <iostream>


class ExampleStructBase : public Meta::MetaObject
{
	DECLARE_META_OBJECT(ExampleStructBase)
public:
	ExampleStructBase()
		: zero(0)
	{
	}
	ExampleStructBase(int i)
		: zero(i)
	{
	}
private:
	int zero;
};

IMPLEMENT_META_OBJECT(ExampleStructBase)
{
	w.addMember<&ExampleStructBase::zero>("zero")
		.setDescription("Test description zero!")
		.setDefault(0);
}

class ExampleStruct : public ExampleStructBase
{
	DECLARE_META_OBJECT(ExampleStruct, ExampleStructBase)
public:
	ExampleStruct()
		: one(0)
		, two(false)
		, three(0)
	{
	}
	ExampleStruct(int i, bool b, float f)
		: ExampleStructBase(i * 2)
		, one(i)
		, two(b)
		, three(f)
	{
	}

	void setThree(float val) { three = val; }
	float getThree() const { return three; }

	bool exampleRandomFunction(bool input) { return input; }
	bool exampleConstRandomFunction(bool input) const { return input; }
private:
	int one;
	bool two;
	float three;
};

IMPLEMENT_META_OBJECT(ExampleStruct)
{
	w.addMember<&ExampleStruct::one>("one")
		.setDescription("Test description one!")
		.setDefault(80085);

	w.addMember<&ExampleStruct::two>("two")
		.setDescription("Test description two!")
		.setReadOnly();

	w.addMember<&ExampleStruct::setThree, &ExampleStruct::getThree>("three")
		.setDescription("Test description three!");

	w.addFunction<&ExampleStruct::exampleRandomFunction>("randomFunction");

	w.addFunction<&ExampleStruct::exampleConstRandomFunction>("constRandomFunction")
		.setDefaultArgs({true});
}

class TEST : public Meta::MetaObject
{
	DECLARE_META_OBJECT(TEST)
public:
	int dummy = 0;
};
IMPLEMENT_META_OBJECT(TEST)
{
}

int main()
{
	Log::initLogging(std::cout, std::cerr);
	Converter::initializeConverters();
	Meta::initializeMetaInfo();

	Log::Debug().log("Debug log {}", "example!");
	Log::Info().log("Info log {}", "example!");
	Log::Warn().log("Warn log {}", "example!");
	Log::Error().log("Error log {}", "example!");
	Log::Critical().log("Critical log {}", "example!");

	Log::Info().log("Test double log").log("   1").log("   2");

	Log::Info().log("Test of indentation!");
	Log::Info(1).log("Test of indentation!");
	Log::Info(2).log("Test of indentation!");

	ExampleStruct obj{11, false, 10.0f};
	auto* objMeta = Meta::getClassMeta<ExampleStruct>();
	if (objMeta)
	{
		for (const auto* prop : objMeta->getMemberProps())
		{
			Log::Info().log("Property: Name = {}, Value = {}", prop->getName(), Converter::getStringFromAny(prop->getTypeIndex(), prop->getAsAny(obj)));
			Log::Info(1).log("Description: {}", prop->getDescription());
		}

		auto* prop = objMeta->getMemberProp("one");
		if (prop)
		{
			prop->applyDefault(obj);
			Log::Info().log("Get property by name with default: {}", prop->getAsType<int>(obj));
		}

		prop = objMeta->getMemberProp("doesn't exist");
		if (prop)
			Log::Info().log("Get property by name: {}", Converter::getStringFromAny(prop->getTypeIndex(), prop->getAsAny(obj)));

		auto* func = objMeta->getNonConstFunc("randomFunction");
		if (func)
			Log::Info().log("Run non-const function: {}", Converter::getStringFromAny(func->getTypeIndex(), func->invoke(obj, {false})));

		auto* cfunc = objMeta->getConstFunc("constRandomFunction");
		if (cfunc)
		{
			Log::Info().log("Run const function: {}", cfunc->invokeAsType<bool>(obj, {false}));
			Log::Info().log("Run const function default args: {}", cfunc->invokeDefaultArgsAsType<bool>(obj));
		}
	}

}