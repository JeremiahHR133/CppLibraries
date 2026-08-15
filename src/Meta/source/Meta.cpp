#include <Meta/Meta.h>

#include <mutex>

namespace
{
class MetaInfoRepo
{
public:
    MetaInfoRepo() = default;
    ~MetaInfoRepo() = default;

    void addClass(const Meta::ClassMetaBase *newClass);
    const std::vector<const Meta::ClassMetaBase *> &getAllClasses()
    {
        return m_allClasses;
    }

private:
    std::vector<const Meta::ClassMetaBase *> m_allClasses;
};

void MetaInfoRepo::addClass(const Meta::ClassMetaBase *newClass)
{
    if (!newClass)
    {
        Log::Error().log("Rejected new class which was nullptr!");
        assert(false && "New class was nullptr");
        return;
    }

    auto foundClass =
        std::find_if(m_allClasses.begin(), m_allClasses.end(),
                     [newClass](const Meta::ClassMetaBase *comp) { return comp->getName() == newClass->getName(); });
    if (foundClass == m_allClasses.end())
    {
        Log::Debug().log("Registered new class: {}", newClass->getName());
        m_allClasses.push_back(newClass);
    }
    else
    {
        Log::Error().log("Class already registered! Name: {}", newClass->getName());
        assert(false && "Duplicate class registered!");
    }
}

MetaInfoRepo &getGlobalMeta()
{
    static MetaInfoRepo s_metaInfo;
    return s_metaInfo;
}

std::vector<std::function<void()>> g_addClassCallbacks;
std::mutex g_addClassVecMutex;

std::vector<std::function<void()>> g_parentInitCallbacks;
std::mutex g_parentInitVecMutex;

std::vector<std::function<void()>> g_validateCallbacks;
std::mutex g_validateMutex;

std::vector<std::function<void()>> g_metaInitCallbacks;
std::mutex g_metaInitVecMutex;
} // namespace

namespace Meta
{
bool MetaObject::isOrIsDerivedFrom(std::type_index idx) const
{
    const ClassMetaBase *ptr = Meta::getClassMeta(getTypeName());
    while (ptr)
    {
        if (ptr->getTypeIndex() == idx)
            return true;

        ptr = ptr->getParent();
    }

    return false;
}

void initializeMetaInfo()
{
    // Register all classes
    {
        std::lock_guard<std::mutex> lock(g_addClassVecMutex);
        for (auto &func : g_addClassCallbacks)
        {
            func();
        }
    }

    // Initialize all meta info
    {
        std::lock_guard<std::mutex> lock(g_metaInitVecMutex);
        for (auto &func : g_metaInitCallbacks)
        {
            func();
        }
    }

    // Update meta info based on parent meta
    {
        std::lock_guard<std::mutex> lock(g_parentInitVecMutex);
        for (auto &func : g_parentInitCallbacks)
        {
            func();
        }
    }

    // Validate all parent child relationships
    {
        std::lock_guard<std::mutex> lock(g_validateMutex);
        for (auto &func : g_validateCallbacks)
        {
            func();
        }
    }
}

const ClassMetaBase *getClassMeta(const std::type_index &index)
{
    for (auto *c : getGlobalMeta().getAllClasses())
        if (c->getTypeIndex() == index)
            return c;

    return nullptr;
}

const ClassMetaBase *getClassMeta(const std::string &name)
{
    for (auto *c : getGlobalMeta().getAllClasses())
        if (c->getName() == name)
            return c;

    return nullptr;
}

const MemberPropertyBase *ClassMetaBase::getMemberProp(const std::string &name) const
{
    const MemberPropertyBase *ret = nullptr;
    forAllMemberProps([&name, &ret](const MemberPropertyBase *prop) -> ClassMetaBase::InvokeResult {
        if (prop->getName() == name)
        {
            ret = prop;
            return ClassMetaBase::InvokeResult::BREAK;
        }
        return ClassMetaBase::InvokeResult::CONTINUE;
    });

    return ret;
}

const MemberNonConstFunctionPropBase *ClassMetaBase::getNonConstFunc(const std::string &name) const
{
    const MemberNonConstFunctionPropBase *ret = nullptr;
    forAllNonConstMemberFuncs([&name, &ret](const MemberNonConstFunctionPropBase *prop) -> ClassMetaBase::InvokeResult {
        if (prop->getName() == name)
        {
            ret = prop;
            return ClassMetaBase::InvokeResult::BREAK;
        }
        return ClassMetaBase::InvokeResult::CONTINUE;
    });

    return ret;
}

const MemberConstFunctionPropBase *ClassMetaBase::getConstFunc(const std::string &name) const
{
    const MemberConstFunctionPropBase *ret = nullptr;
    forAllConstMemberFuncs([&name, &ret](const MemberConstFunctionPropBase *prop) -> ClassMetaBase::InvokeResult {
        if (prop->getName() == name)
        {
            ret = prop;
            return ClassMetaBase::InvokeResult::BREAK;
        }
        return ClassMetaBase::InvokeResult::CONTINUE;
    });

    return ret;
}

void ClassMetaBase::logTypeInfo(bool recursive, int indent) const
{
    Log::Info(indent).log("Type info for class \"{}\"", getName());
    Log::Info(indent + 1).log("Name       : {}", getName());
    Log::Info(indent + 1).log("Parent Name: {}", getParentName());
    Log::Info(indent + 1).log("Member Properties:");
    forAllMemberProps(
        [this, indent](const MemberPropertyBase *prop) -> InvokeResult {
            Log::Info(indent + 2).log("Type info for property \"{}\"", prop->getName());
            Log::Info(indent + 3).log("Name       : {}", prop->getName());
            Log::Info(indent + 3).log("Class Name : {}", prop->getClassName());
            Log::Info(indent + 3).log("Description: {}", prop->getDescription());
            Log::Info(indent + 3).log("Has Default: {}", prop->hasDefault());
            Log::Info(indent + 3).log("Read Only  : {}", prop->getReadOnly());

            return InvokeResult::CONTINUE;
        },
        !recursive);
    Log::Info(indent + 1).log("Const Member Functions:");
    forAllConstMemberFuncs(
        [this, indent](const MemberConstFunctionPropBase *prop) -> InvokeResult {
            Log::Info(indent + 2).log("Type info for property \"{}\"", prop->getName());
            Log::Info(indent + 3).log("Name            : {}", prop->getName());
            Log::Info(indent + 3).log("Class Name      : {}", prop->getClassName());
            Log::Info(indent + 3).log("Description     : {}", prop->getDescription());
            Log::Info(indent + 3).log("Has Default Args: {}", prop->hasDefaultArgs());

            return InvokeResult::CONTINUE;
        },
        !recursive);
    Log::Info(indent + 1).log("Non-Const Member Functions:");
    forAllNonConstMemberFuncs(
        [this, indent](const MemberNonConstFunctionPropBase *prop) -> InvokeResult {
            Log::Info(indent + 2).log("Type info for property \"{}\"", prop->getName());
            Log::Info(indent + 3).log("Name            : {}", prop->getName());
            Log::Info(indent + 3).log("Class Name      : {}", prop->getClassName());
            Log::Info(indent + 3).log("Description     : {}", prop->getDescription());
            Log::Info(indent + 3).log("Has Default Args: {}", prop->hasDefaultArgs());

            return InvokeResult::CONTINUE;
        },
        !recursive);

    if (recursive && parent)
    {
        Log::Info(indent + 1).log("Type info for parent class \"{}\"", parent->getName());
        parent->logTypeInfo(recursive, indent + 2);
    }
}

namespace Impl
{
void addClass(const ClassMetaBase *c)
{
    getGlobalMeta().addClass(c);
}

void addDelayClass(std::function<void()> call)
{
    std::lock_guard<std::mutex> lock(g_addClassVecMutex);
    g_addClassCallbacks.push_back(call);
}

void addDelayParentInitialize(std::function<void()> call)
{
    std::lock_guard<std::mutex> lock(g_parentInitVecMutex);
    g_parentInitCallbacks.push_back(call);
}

void addDelayValidate(std::function<void()> call)
{
    std::lock_guard<std::mutex> lock(g_validateMutex);
    g_validateCallbacks.push_back(call);
}

void addDelayMetaInitialize(std::function<void()> call)
{
    std::lock_guard<std::mutex> lock(g_metaInitVecMutex);
    g_metaInitCallbacks.push_back(call);
}
} // namespace Impl
} // namespace Meta