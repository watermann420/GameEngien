using System;
using System.Runtime.InteropServices;

// Minimal C# wrapper to mirror the C++ GameObject API via P/Invoke.
namespace GameEngine.Objects
{
    public sealed class GameObject : IDisposable
    {
        private bool _owns;
        public ulong Handle { get; private set; }

        public GameObject(string name = "GameObject", string tag = "")
        {
            Handle = GE_CreateObject(name, tag);
            _owns = true;
        }

        private GameObject(ulong handle)
        {
            Handle = handle;
            _owns = false;
        }

        public GameObject AddChild(string name, string tag = "")
        {
            var childHandle = GE_AddChild(Handle, name, tag);
            return new GameObject(childHandle);
        }

        public uint ChildCount => GE_GetChildCount(Handle);

        public GameObject GetChild(uint index)
        {
            var childHandle = GE_GetChildAt(Handle, index);
            return childHandle == 0 ? null : new GameObject(childHandle);
        }

        public string Name
        {
            get => Marshal.PtrToStringAnsi(GE_GetName(Handle)) ?? "";
            set => GE_SetName(Handle, value);
        }

        public string Tag
        {
            get => Marshal.PtrToStringAnsi(GE_GetTag(Handle)) ?? "";
            set => GE_SetTag(Handle, value);
        }

        public static GameObject Find(string name)
        {
            var handle = GE_FindByName(name);
            return handle == 0 ? null : new GameObject(handle);
        }

        public void Dispose()
        {
            if (_owns && Handle != 0)
            {
                GE_DestroyObject(Handle);
                Handle = 0;
            }
        }

        private const string Dll = "GameEngine.dll"; // match output target name

        [DllImport(Dll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern ulong GE_CreateObject(string name, string tag);

        [DllImport(Dll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern void GE_DestroyObject(ulong id);

        [DllImport(Dll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern ulong GE_AddChild(ulong parentId, string name, string tag);

        [DllImport(Dll, CallingConvention = CallingConvention.Cdecl)]
        private static extern uint GE_GetChildCount(ulong parentId);

        [DllImport(Dll, CallingConvention = CallingConvention.Cdecl)]
        private static extern ulong GE_GetChildAt(ulong parentId, uint index);

        [DllImport(Dll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern ulong GE_FindByName(string name);

        [DllImport(Dll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern void GE_SetName(ulong id, string name);

        [DllImport(Dll, CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr GE_GetName(ulong id);

        [DllImport(Dll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern void GE_SetTag(ulong id, string tag);

        [DllImport(Dll, CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr GE_GetTag(ulong id);
    }
}

