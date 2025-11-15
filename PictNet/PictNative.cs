using System.Runtime.InteropServices;


namespace PictNet;

internal class PictNative
{
    const string Dll = "pictApiDll.dll";

    [DllImport(Dll, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int PictGenerateIndices(
        [In] UIntPtr[] valueCounts,
        UIntPtr paramCount,
        uint order,
        uint randomSeed,
        out IntPtr outCells,
        out UIntPtr outRowCount);

    [DllImport(Dll, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void PictFreeIndices(IntPtr cells);
    public PictNative()
    {
        NativeLoader.EnsureLoaded();
    }
}
