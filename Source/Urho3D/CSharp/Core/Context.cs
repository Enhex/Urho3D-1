﻿using System;
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Urho3D.CSharp;

namespace Urho3D
{
    public class ObjectFactoryAttribute : System.Attribute
    {
        public string Category { get; set; } = "";
    }

    public partial class Context
    {
        public static Context Instance { get; private set; }

        static Context()
        {
            // Exchange APIs
            NativeInterface.Initialize();
        }

        private readonly Dictionary<uint, Type> _factoryTypes = new Dictionary<uint, Type>();

        // This method may be overriden in partial class in order to attach extra logic to object constructor
        internal override void SetupInstance(IntPtr instance, bool ownsInstance)
        {
            // Set up this instance
            PerformInstanceSetup(instance, ownsInstance);

            // Set up engine bindings
            Urho3DRegisterCSharp(instance);

            // Register factories marked with attributes
            foreach (var assembly in AppDomain.CurrentDomain.GetAssemblies())
            {
                foreach (var pair in assembly.GetTypesWithAttribute<ObjectFactoryAttribute>())
                    RegisterFactory(pair.Item1, pair.Item2.Category);
            }

            Instance = this;
        }

        protected override void OnDispose()
        {
            Instance = null;
            InstanceCache.Dispose();
        }

        public void RegisterFactory<T>(string category = "") where T : Object
        {
            RegisterFactory(typeof(T), category);
        }

        public void RegisterFactory(Type type, string category="")
        {
            if (!type.IsSubclassOf(typeof(Object)))
                throw new ArgumentException("Type must be sublass of Object.");

            _factoryTypes[StringHash.Calculate(type.Name)] = type;

            // Find a wrapper base type.
            var baseType = type.BaseType;
            while (baseType != null && baseType.Assembly != typeof(Context).Assembly)
                baseType = baseType.BaseType;

            if (baseType == null)
                throw new InvalidOperationException("This type can not be registered as factory.");

            Urho3D_Context_RegisterFactory(GetNativeInstance(this), type.Name, StringHash.Calculate("Wrappers::" + baseType.Name), category);
        }

        internal IntPtr CreateObject(uint managedType)
        {
            Type type;
            if (!_factoryTypes.TryGetValue(managedType, out type))
                return IntPtr.Zero;
            var managed = (Object)Activator.CreateInstance(type, BindingFlags.Public | BindingFlags.Instance,
                null, new object[] { this }, null);
            return managed.NativeInstance;
        }

        [DllImport(Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Context_RegisterFactory(IntPtr context,
            [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(StringUtf8))]string typeName,
            uint baseType,
            [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(StringUtf8))]string category);

        [DllImport(Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void Urho3DRegisterCSharp(IntPtr contextPtr);
    }
}
