using System.Runtime.InteropServices;

namespace PictNet;

internal static class NativeLoader
{
    static NativeLoader()
    {
        var asm = typeof(NativeLoader).Assembly;

        // Adjust this to your real resource name; easiest is to inspect asm.GetManifestResourceNames()
        const string resourceName = "PictNet.native.pictApiDll.dll";

        using var stream = asm.GetManifestResourceStream(resourceName)
                         ?? throw new InvalidOperationException($"Resource '{resourceName}' not found.");

        var dllPath = Path.Combine(AppContext.BaseDirectory, "pictApiDll.dll");

        if (!File.Exists(dllPath))
        {
            using var fs = File.Create(dllPath);
            stream.CopyTo(fs);
        }

        // Make sure the DLL is actually loaded into the process
#if NETSTANDARD2_0 || NETFRAMEWORK
        // .NET Framework/Standard: rely on LoadLibrary through kernel32
        LoadLibrary(dllPath);
#else
        // .NET Core / .NET 5+: use NativeLibrary
        NativeLibrary.Load(dllPath);
#endif
    }

    public static void EnsureLoaded() { }

#if NETSTANDARD2_0 || NETFRAMEWORK
    [DllImport("kernel32", SetLastError = true, CharSet = CharSet.Unicode)]
    static extern IntPtr LoadLibrary(string lpFileName);
#endif
}
