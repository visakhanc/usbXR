using System;
using System.Runtime.InteropServices; 

namespace GenericHid
{   
	/// <summary>
	/// Win32 API declarations for Debug.Write statements.
	/// </summary>
	/// 
    internal sealed partial class Debugging  
    {
		internal static class NativeMethods
		{
			internal const Int16 FormatMessageFromSystem = 0X1000;

			[DllImport("kernel32.dll", CharSet = CharSet.Auto, SetLastError = true)]
			internal static extern Int32 FormatMessage(Int32 dwFlags, ref Int64 lpSource, Int32 dwMessageId, Int32 dwLanguageZId,
			                                           String lpBuffer, Int32 nSize, Int32 arguments);
		}
    } 
} 
