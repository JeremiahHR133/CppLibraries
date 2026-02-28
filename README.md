# C++ Libraries

This project contians three libraries: Logger, Converter, and Meta. The Meta library and the Converter library both depend on the Logger library, but the Meta library and the Converter library can be used independently.
The Converter library exists mostly to help users encode and decode the std::any types that the meta system operates on.

The recomended usage for this project is to add it as a submodule in your own project.

The Meta and Converter library both defer their initialization to a function call. This isn't just done so that logging can be initialized prior to the library.
It was also done so that projects which load plugins at runtime can allow plugins to specify converter and meta info which will can then be initialized along with all the meta and converter info defined in the current module.

## Requirements

I haven't tested this project on Linux yet, but I do plan to. Nothing about this project should be platform dependent; I just haven't had time to setup a Linux box to test it on.

* C++ 23 (mostly just for std::format)
* CMake 3.24 +

## Build Commands

Windows:
1. `cmake -S . -B build`
2. `cmake --build build`

Linux:
1. `cmake -S . -B build -DCMAKE_CXX_COMPILER=/usr/bin/g++-14 -DCMAKE_C_COMPILER=/usr/bin/gcc-14`
2. `cmake --build build`


## Logger

The logger library is essentially a wrapper to std::print. Logs are made by instantiating one of the log level classes (Debug, Info, Warn, Error, Critical) and calling .log() on them.
The syntax for the .log() call matches the syntax for std::print.

Logging must be initialized before any logs can be made. The logger library takes two streams at initialization; one stream is for all logs, and the other stream will only recieve Error and Critical logs.

Here is a basic exmaple of logger initialization
```cpp
int main()
{
	Log::initLogging(std::cout, std::cerr);
	Log::Info().log("Info log {}", "example!");
}
```
See `src/Tests/main.cpp` for more examples of how to use the logging library.




## Converter

The converter library is a library for defining toString and fromString converters for any type. Converters for all the primitive types are automatically provided by the library.
Similar to the Logger library, the Converter library must be initialized in main. Since it depends on the logger library, you must make sure to initialize logging before initializing the converter library.

Converters can be registered via the `REGISTER_CONVERTER_FOR_TYPE` macro.
This macro should be placed anywhere inside of a source file (not in a header). It uses static object initialization to register itself when the module is loaded into memory.

Example converter registration:
```cpp
REGISTER_CONVERTER_FOR_TYPE(
	bool, 
	[](const bool& b) { return b ? "true" : "false"; },
	[](const std::string& str) { return str == "true" ? true : false; }
)
```

Converters can be registered with lambdas (as shown in the example above), or with regular function pointers (shown below)

```cpp
REGISTER_CONVERTER_FOR_TYPE(ExampleType,
	&ExampleType::toString,
	&ExampleType::fromString
)
```
The main usage for the converter library is to operate on std::any types provided by the Meta library.

Example:
```cpp
Log::Info().log("Property: Name = {}, Value = {}", prop->getName(), Converter::getStringFromAny(prop->getTypeIndex(), prop->getAsAny(obj)));
```

See `src/Tests/main.cpp` for more examples of how to use the converter library.
See `src/GTests/Converter_gtest/converter_gtest.cpp` for example usage of the full API.




## Meta

The Meta library provides an API for type retrospection in C++. It seeks to use minimal macros and instead relies primarily on regular C++ function calls to register and interact with the meta info.
One macro is needed in the class definition, and one macro is needed outside of the class definition. Outside of these two macros, everything is done with regular C++ functions and function templates.

Similar to the Logger library, the Meta library must be initialized in main. Since it depends on the Logger library, you must make sure to initialize logging before initializing the converter library.

Currently, the Meta library provides three types of properties: Member properties, const member function properties, and non-const member function properties.
The Meta library also supports meta info inheritence. Currently the meta info inheritence system does not allow property overriding (all properties must have a unique name).

Here is an example of adding meta info to a basic class:
```cpp
// Meta objects must inherit from the Meta::MetaObject class
class ExampleStruct : public Meta::MetaObject
{
    // Macro to declare a class as a meta object
	DECLARE_META_OBJECT(ExampleStruct)
public:
	ExampleStruct()
		: one(0)
		, two(false)
		, three(0)
	{
	}
	ExampleStruct(int i, bool b, float f)
		: one(i)
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

// Macro to implement a meta object
// It provides a type info writer object named 'w'
IMPLEMENT_META_OBJECT(ExampleStruct)
{
    // Member property bound directly to the class member
	w.addMember<&ExampleStruct::one>("one")
		.setDescription("Test description one!")
		.setDefault(80085);

    // Member property bound directly to the class member
	w.addMember<&ExampleStruct::two>("two")
		.setDescription("Test description two!")
		.setReadOnly();

    // Member property bound to a member setter/getter pair
	w.addMember<&ExampleStruct::setThree, &ExampleStruct::getThree>("three")
		.setDescription("Test description three!");

	w.addFunction<&ExampleStruct::exampleRandomFunction>("randomFunction");

	w.addFunction<&ExampleStruct::exampleConstRandomFunction>("constRandomFunction")
		.setDefaultArgs({true});
}
```

See `src/Tests/main.cpp` for more examples of how to use the Meta library.
See `src/GTests/Meta_gtest/meta_gtest.cpp` for example usage of the full API.