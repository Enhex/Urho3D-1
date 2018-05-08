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


#include "../Core/Mutex.h"
#include "../Core/Object.h"

namespace Urho3D
{

class URHO3D_API ScriptSubsystem : public Object
{
    URHO3D_OBJECT(ScriptSubsystem, Object);

public:
    struct RuntimeSettings
    {
        /// Name of new jit domain.
        String domainName_ = "DefaultDomain";
        /// Path to executable which will be executed. Must have static main method. Can be empty.
        String defaultExecutable_;
        /// Jit options.
        Vector<String> jitOptions_ = {
            "--debugger-agent=transport=dt_socket,address=127.0.0.1:53631,server=y,suspend=n",
            "--optimize=float32"
        };
    };

    explicit ScriptSubsystem(Context* context);

    /// Register types of inheritable native objects. They are used in factory which creates managed subclasses of these objects.
    template<typename T>
    void RegisterType() { typeInfos_[T::GetTypeStatic()] = T::GetTypeInfoStatic(); }
    /// Returns type information of registered base native type.
    const TypeInfo* GetRegisteredType(StringHash type);
    /// Schedule ReleaseRef() call to be called on the main thread.
    void QueueReleaseRef(RefCounted* instance);
    /// Registers current thread with .net runtime.
    void RegisterCurrentThread();

    /// Creates a new managed domain and optionally executes an assembly.
    void* HostManagedRuntime(RuntimeSettings& settings);
    /// Load managed assembly and return it.
    void* LoadAssembly(const String& pathToAssembly);
    /// Call a managed method and return result.
    Variant CallMethod(void* assembly, const String& methodDesc, void* object = nullptr,
        const VariantVector& args = Variant::emptyVariantVector);

    /// Frees handle of managed object.
    void FreeGCHandle(void* gcHandle);
    /// Clones handle of managed object.
    void* CloneGCHandle(void* gcHandle);
    /// Creates managed object and returns it's native instance.
    Object* CreateObject(Context* context, unsigned managedType);
    /// Converts instance to managed object.
    void* ToManagedObject(const char* imageName, const char* className, RefCounted* instance);
    /// Acquires a reference to a managed object preventing it's garbage collection. Pass `pin=true` to prevent GC moving object from it's current memory location.
    unsigned Lock(void* object, bool pin=false);
    /// Releases object reference. GC will be able to collect this object when no more references exist.
    void Unlock(unsigned handle);

protected:
    /// Initializes object.
    void Init();
    /// Perform housekeeping tasks.
    void OnEndFrame(StringHash, VariantMap&);

    /// Types of inheritable native classes.
    HashMap<StringHash, const TypeInfo*> typeInfos_;
    /// Queue of objects that should have their ReleaseRef() method called on main thread.
    PODVector<RefCounted*> releaseQueue_;
    /// Mutex protecting resources related to queuing ReleaseRef() calls.
    Mutex mutex_;
    /// Managed API function pointers.
    void(*FreeGCHandle_)(void* gcHandle, void* exception);
    void*(*CloneGCHandle_)(void* gcHandle, void* exception);
    Object*(*CreateObject_)(Context* context, unsigned managedType, void* exception);
};

}
