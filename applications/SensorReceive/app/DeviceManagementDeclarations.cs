using System;
using System.Runtime.InteropServices; 

namespace GenericHid
{
	internal sealed partial class DeviceManagement
	{
		internal static class NativeMethods
		{
			///<summary >
			// API declarations relating to device management (SetupDixxx and 
			// RegisterDeviceNotification functions).   
			/// </summary>

			// from setupapi.h

			internal const Int32 DIGCF_PRESENT = 2;
			internal const Int32 DIGCF_DEVICEINTERFACE = 0X10;

			internal struct SP_DEVICE_INTERFACE_DATA
			{
				internal Int32 cbSize;
				internal Guid InterfaceClassGuid;
				internal Int32 Flags;
				internal IntPtr Reserved;
			}

			internal struct SP_DEVICE_INTERFACE_DETAIL_DATA
			{
				internal Int32 cbSize;
				internal String DevicePath;
			}

			internal struct SP_DEVINFO_DATA
			{
				internal Int32 cbSize;
				internal Guid ClassGuid;
				internal Int32 DevInst;
				internal Int32 Reserved;
			}

			[DllImport("setupapi.dll", SetLastError = true)]
			internal static extern IntPtr SetupDiCreateDeviceInfoList(ref Guid ClassGuid, IntPtr hwndParent);

			[DllImport("setupapi.dll", SetLastError = true)]
			internal static extern Int32 SetupDiDestroyDeviceInfoList(IntPtr DeviceInfoSet);

			[DllImport("setupapi.dll", SetLastError = true)]
			internal static extern Boolean SetupDiEnumDeviceInterfaces(IntPtr DeviceInfoSet, IntPtr DeviceInfoData, ref Guid InterfaceClassGuid, Int32 MemberIndex, ref SP_DEVICE_INTERFACE_DATA DeviceInterfaceData);

			[DllImport("setupapi.dll", SetLastError = true, CharSet = CharSet.Auto)]
			internal static extern IntPtr SetupDiGetClassDevs(ref Guid ClassGuid, IntPtr Enumerator, IntPtr hwndParent, Int32 Flags);

			[DllImport("setupapi.dll", SetLastError = true, CharSet = CharSet.Auto)]
			internal static extern Boolean SetupDiGetDeviceInterfaceDetail(IntPtr DeviceInfoSet, ref SP_DEVICE_INTERFACE_DATA DeviceInterfaceData, IntPtr DeviceInterfaceDetailData, Int32 DeviceInterfaceDetailDataSize, ref Int32 RequiredSize, IntPtr DeviceInfoData);
		}
	}
}
