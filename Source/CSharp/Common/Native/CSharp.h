//
// Copyright (c) 2018 Rokas Kupstys
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
#pragma once

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/Mutex.h>
#include <Urho3D/Core/Object.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Script/ScriptSubsystem.h>
#include <string>
#include <type_traits>

using namespace Urho3D;

#ifndef EXPORT_API
#   if _WIN32
#       define EXPORT_API __declspec(dllexport)
#   else
#       define EXPORT_API __attribute__ ((visibility("default")))
#   endif
#endif


namespace Urho3D
{

extern ScriptSubsystem* scriptSubsystem;

}

/// Force-cast between incompatible types.
template<typename T0, typename T1>
inline T0 force_cast(T1 input)
{
    union
    {
        T1 input;
        T0 output;
    } u = { input };
    return u.output;
};

#define URHO3D_OBJECT_STATIC(typeName, baseTypeName) \
    public: \
        using ClassName = typeName; \
        using BaseClassName = baseTypeName; \
        static Urho3D::StringHash GetTypeStatic() { return GetTypeInfoStatic()->GetType(); } \
        static const Urho3D::String& GetTypeNameStatic() { return GetTypeInfoStatic()->GetTypeName(); } \
        static const Urho3D::TypeInfo* GetTypeInfoStatic() { static const Urho3D::TypeInfo typeInfoStatic(#typeName, BaseClassName::GetTypeInfoStatic()); return &typeInfoStatic; }


template<typename Derived, typename Base>
inline size_t GetBaseClassOffset()
{
    // Dragons be here
    return reinterpret_cast<uintptr_t>(static_cast<Base*>(reinterpret_cast<Derived*>(1))) - 1;
};

struct CSharpObjConverter
{
    template<typename T> using RefCountedType = typename std::enable_if<std::is_base_of<Urho3D::RefCounted, T>::value, T>::type;
    template<typename T> using NonRefCountedType = typename std::enable_if<!std::is_base_of<Urho3D::RefCounted, T>::value && !std::is_copy_constructible<T>::value, T>::type;
    template<typename T> using CopyableType = typename std::enable_if<!std::is_base_of<Urho3D::RefCounted, T>::value && std::is_copy_constructible<T>::value, T>::type;

    template<typename T> static T* ToCSharp(const SharedPtr<T>& object)          { return object.Get(); }
    template<typename T> static T* ToCSharp(const WeakPtr<T>& object)            { return object.Get(); }
    template<typename T> static T* ToCSharp(const RefCountedType<T>* object)     { return const_cast<T*>(object); }
    template<typename T> static T* ToCSharp(CopyableType<T>&& object)            { return new T(object); }
    template<typename T> static T* ToCSharp(const CopyableType<T>& object)       { return (T*)&object; }
    template<typename T> static T* ToCSharp(const CopyableType<T>* object)       { return (T*)object; }
    template<typename T> static T* ToCSharp(const NonRefCountedType<T>&& object) { return (T*)&object; }
    template<typename T> static T* ToCSharp(const NonRefCountedType<T>& object)  { return (T*)&object; }
    template<typename T> static T* ToCSharp(const NonRefCountedType<T>* object)  { return (T*)object; }
};

template<typename T> struct CSharpConverter;

//////////////////////////////////////// Array converters //////////////////////////////////////////////////////////////

// Convert PODVector<T>
template<typename T>
struct CSharpConverter<PODVector<T>>
{
    using CppType=PODVector<T>;
    using CType=void*;

    // TODO: This can be optimized by passing &value.Front() memory buffer
    static CType ToCSharp(const CppType& value)
    {
        auto length = value.Size() * sizeof(T);
        auto* data = MarshalAllocator::Get().Alloc(length);
        memcpy(data, &value.Front(), length);
        return data;
    }

    static CppType FromCSharp(CType value)
    {
        auto length = *(int32_t*)((uint8_t*)value - 4);
        auto count = length / sizeof(T);
        return CppType((const T*)value, count);
    }
};

// Convert Vector<SharedPtr<T>>
template<typename T>
struct CSharpConverter<Vector<SharedPtr<T>>>
{
    using CppType=Vector<SharedPtr<T>>;
    using CType=void*;

    static CType ToCSharp(const CppType& value)
    {
        int length = (int)value.Size() * sizeof(void*);
        CType result = MarshalAllocator::Get().Alloc(length);

        auto** array = (T**)result;
        for (const auto& ptr : value)
            *array++ = ptr.Get();

        return result;
    }

    static CppType FromCSharp(CType value)
    {
        auto length = *(int32_t*)((uint8_t*)value - 4);
        auto count = length / sizeof(void*);
        CppType result{(unsigned)count};
        auto** array = (T**)value;
        for (auto& ptr : result)
            ptr = *array++;
        return result;
    }
};

// Convert Vector<WeakPtr<T>>
template<typename T>
struct CSharpConverter<Vector<WeakPtr<T>>>
{
    using CppType=Vector<WeakPtr<T>>;
    using CType=void*;

    static CType ToCSharp(const CppType& value)
    {
        int length = (int)value.Size() * sizeof(void*);
        CType result = MarshalAllocator::Get().Alloc(length);

        auto** array = (T**)result;
        for (const auto& ptr : value)
            *array++ = ptr.Get();

        return result;
    }

    static CppType FromCSharp(CType value)
    {
        auto length = *(int32_t*)((uint8_t*)value - 4);
        auto count = length / sizeof(void*);
        CppType result{(unsigned)count};
        auto** array = (T**)value;
        for (auto& ptr : result)
            ptr = *array++;
        return result;
    }
};

// Convert Vector<T*>
template<typename T>
struct CSharpConverter<Vector<T*>>
{
    using CppType=Vector<T*>;
    using CType=void*;

    // TODO: This can be optimized by passing &value.Front() memory buffer
    static CType ToCSharp(const CppType& value)
    {
        int length = (int)value.Size() * sizeof(void*);
        CType result = MarshalAllocator::Get().Alloc(length);

        auto** array = (T**)result;
        for (const auto& ptr : value)
            *array++ = ptr;

        return result;
    }

    static CppType FromCSharp(CType value)
    {
        auto length = *(int32_t*)((uint8_t*)value - 4);
        auto count = length / sizeof(void*);
        CppType result{(unsigned)count};
        auto** array = (T**)value;
        for (auto& ptr : result)
            ptr = *array++;
        return result;
    }
};

////////////////////////////////////// String converters ///////////////////////////////////////////////////////////////

template<> struct CSharpConverter<Urho3D::String>
{
    // TODO: This can be optimized by passing value memory buffer
    static inline const char* ToCSharp(const char* value)
    {
        void* memory = MarshalAllocator::Get().Alloc(strlen(value) + 1);
        strcpy((char*)memory, value);
        return (const char*)memory;
    }

    // TODO: This can be optimized by passing &value.CString() memory buffer
    static inline const char* ToCSharp(const String& value)
    {
        void* memory = MarshalAllocator::Get().Alloc(value.Length() + 1);
        strcpy((char*)memory, value.CString());
        return (const char*)memory;
    }
};

///////////////////////////////////////// Utilities ////////////////////////////////////////////////////////////////////
template<typename T>
std::uintptr_t GetTypeID()
{
    return reinterpret_cast<std::uintptr_t>(&typeid(T));
}

template<typename T>
std::uintptr_t GetTypeID(const T* instance)
{
    return reinterpret_cast<std::uintptr_t>(&typeid(*instance));
}

