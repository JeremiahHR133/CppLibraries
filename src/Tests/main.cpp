#include <Logger/Logger.h>
#include <Converter/Converter.h>
#include <Meta/Meta.h>

#include <iostream>



class ExampleStruct : public Meta::MetaObject
{
	DECLARE_META_OBJECT(ExampleStruct)
public:
	ExampleStruct()
		: one(0)
		, two(false)
	{
	}
	ExampleStruct(int i, bool b)
		: one(i)
		, two(b)
	{
	}
private:
	int one;
	bool two;
};

IMPLEMENT_META_OBJECT(ExampleStruct)
{
	w.addProperty<&ExampleStruct::one>("one");
	w.addProperty<&ExampleStruct::two>("two");
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

	ExampleStruct obj{1, false};
	auto* objMeta = Meta::getClassMeta<ExampleStruct>();
	if (objMeta)
	{
		for (const auto& prop : objMeta->getProps())
		{
			Log::Info().log("Property: Name = {}, Value = {}", prop->getName(), Converter::getStringFromAny(prop->getTypeIndex(), prop->getAsAny(obj)));
		}

		auto* prop = objMeta->getProp("one");
		if (prop)
			Log::Info().log("Get property by name: {}", Converter::getStringFromAny(prop->getTypeIndex(), prop->getAsAny(obj)));

		prop = objMeta->getProp("doesn't exist");
		if (prop)
			Log::Info().log("Get property by name: {}", Converter::getStringFromAny(prop->getTypeIndex(), prop->getAsAny(obj)));
	}

}