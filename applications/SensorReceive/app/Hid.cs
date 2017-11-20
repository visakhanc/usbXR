using Microsoft.Win32.SafeHandles;
using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;

namespace GenericHid
{
	///  <summary>
	///  Supports Windows API functions for accessing HID-class USB devices.
	///  Includes routines for retrieving information about the configuring a HID and 
	///  sending and receiving reports via control and interrupt transfers. 
	///  </summary>

	internal sealed partial class Hid
	{
		//  Used in error messages.

		private const String ModuleName = "Hid";

		internal NativeMethods.HIDP_CAPS Capabilities;
		internal NativeMethods.HIDD_ATTRIBUTES DeviceAttributes;

		//  For viewing results of API calls in debug.write statements:

		internal static Debugging MyDebugging = new Debugging();

		///  <summary>
		///  Provides a central mechanism for exception handling.
		///  Displays a message box that describes the exception.
		///  </summary>
		///  
		///  <param name="moduleName">  the module where the exception occurred. </param>
		///  <param name="e"> the exception </param>

		internal static void DisplayException(String moduleName, Exception e)
		{
			//  Create an error message.

			String message = "Exception: " + e.Message + Environment.NewLine + "Module: " + moduleName + Environment.NewLine + "Method: " + e.TargetSite.Name;

			const String caption = "Unexpected Exception";

			//MessageBox.Show(message, caption, MessageBoxButtons.OK);
			Debug.Write(message);

			// Get the last error and display it. 
			Int32 error = Marshal.GetLastWin32Error();

			Debug.WriteLine("The last Win32 Error was: " + error);
		}

		///  <summary>
		///  Remove any Input reports waiting in the buffer.
		///  </summary>
		///  <param name="hidHandle"> a handle to a device.   </param>
		///  <returns>
		///  True on success, False on failure.
		///  </returns>

		internal Boolean FlushQueue(SafeFileHandle hidHandle)
		{
			try
			{
				//  ***
				//  API function: HidD_FlushQueue

				//  Purpose: Removes any Input reports waiting in the buffer.

				//  Accepts: a handle to the device.

				//  Returns: True on success, False on failure.
				//  ***

				Boolean success = NativeMethods.HidD_FlushQueue(hidHandle);

				return success;
			}

			catch (Exception ex)
			{
				DisplayException(ModuleName, ex);
				throw;
			}
		}

		/// <summary>
		/// Get HID attributes.
		/// </summary>
		/// <param name="hidHandle"> HID handle retrieved with CreateFile </param>
		/// <param name="deviceAttributes"> HID attributes structure </param>
		/// <returns> true on success </returns>

		internal Boolean GetAttributes(SafeFileHandle hidHandle, ref NativeMethods.HIDD_ATTRIBUTES deviceAttributes)
		{
			Boolean success;
			try
			{
				//  ***
				//  API function:
				//  HidD_GetAttributes

				//  Purpose:
				//  Retrieves a HIDD_ATTRIBUTES structure containing the Vendor ID, 
				//  Product ID, and Product Version Number for a device.

				//  Accepts:
				//  A handle returned by CreateFile.
				//  A pointer to receive a HIDD_ATTRIBUTES structure.

				//  Returns:
				//  True on success, False on failure.
				//  ***                            

				success = NativeMethods.HidD_GetAttributes(hidHandle, ref deviceAttributes);
			}

			catch (Exception ex)
			{
				DisplayException(ModuleName, ex);
				throw;
			}
			return success;
		}

		///  <summary>
		///  Retrieves a structure with information about a device's capabilities. 
		///  </summary>
		///  
		///  <param name="hidHandle"> a handle to a device. </param>
		///  
		///  <returns>
		///  An HIDP_CAPS structure.
		///  </returns>

		internal NativeMethods.HIDP_CAPS GetDeviceCapabilities(SafeFileHandle hidHandle)
		{
			var preparsedData = new IntPtr();

			try
			{
				//  ***
				//  API function: HidD_GetPreparsedData

				//  Purpose: retrieves a pointer to a buffer containing information about the device's capabilities.
				//  HidP_GetCaps and other API functions require a pointer to the buffer.

				//  Requires: 
				//  A handle returned by CreateFile.
				//  A pointer to a buffer.

				//  Returns:
				//  True on success, False on failure.
				//  ***

				NativeMethods.HidD_GetPreparsedData(hidHandle, ref preparsedData);

				//  ***
				//  API function: HidP_GetCaps

				//  Purpose: find out a device's capabilities.
				//  For standard devices such as joysticks, you can find out the specific
				//  capabilities of the device.
				//  For a custom device where the software knows what the device is capable of,
				//  this call may be unneeded.

				//  Accepts:
				//  A pointer returned by HidD_GetPreparsedData
				//  A pointer to a HIDP_CAPS structure.

				//  Returns: True on success, False on failure.
				//  ***

				Int32 result = NativeMethods.HidP_GetCaps(preparsedData, ref Capabilities);
				if ((result != 0))
				{
					Debug.WriteLine("");
					Debug.WriteLine("  Usage: " + Convert.ToString(Capabilities.Usage, 16));
					Debug.WriteLine("  Usage Page: " + Convert.ToString(Capabilities.UsagePage, 16));
					Debug.WriteLine("  Input Report Byte Length: " + Capabilities.InputReportByteLength);
					Debug.WriteLine("  Output Report Byte Length: " + Capabilities.OutputReportByteLength);
					Debug.WriteLine("  Feature Report Byte Length: " + Capabilities.FeatureReportByteLength);
					Debug.WriteLine("  Number of Link Collection Nodes: " + Capabilities.NumberLinkCollectionNodes);
					Debug.WriteLine("  Number of Input Button Caps: " + Capabilities.NumberInputButtonCaps);
					Debug.WriteLine("  Number of Input Value Caps: " + Capabilities.NumberInputValueCaps);
					Debug.WriteLine("  Number of Input Data Indices: " + Capabilities.NumberInputDataIndices);
					Debug.WriteLine("  Number of Output Button Caps: " + Capabilities.NumberOutputButtonCaps);
					Debug.WriteLine("  Number of Output Value Caps: " + Capabilities.NumberOutputValueCaps);
					Debug.WriteLine("  Number of Output Data Indices: " + Capabilities.NumberOutputDataIndices);
					Debug.WriteLine("  Number of Feature Button Caps: " + Capabilities.NumberFeatureButtonCaps);
					Debug.WriteLine("  Number of Feature Value Caps: " + Capabilities.NumberFeatureValueCaps);
					Debug.WriteLine("  Number of Feature Data Indices: " + Capabilities.NumberFeatureDataIndices);

					//  ***
					//  API function: HidP_GetValueCaps

					//  Purpose: retrieves a buffer containing an array of HidP_ValueCaps structures.
					//  Each structure defines the capabilities of one value.
					//  This application doesn't use this data.

					//  Accepts:
					//  A report type enumerator from hidpi.h,
					//  A pointer to a buffer for the returned array,
					//  The NumberInputValueCaps member of the device's HidP_Caps structure,
					//  A pointer to the PreparsedData structure returned by HidD_GetPreparsedData.

					//  Returns: True on success, False on failure.
					//  ***                    

					Int32 vcSize = Capabilities.NumberInputValueCaps;
					var valueCaps = new Byte[vcSize];

					NativeMethods.HidP_GetValueCaps(NativeMethods.HidP_Input, valueCaps, ref vcSize, preparsedData);

					// (To use this data, copy the ValueCaps byte array into an array of structures.)              
				}
			}

			catch (Exception ex)
			{
				DisplayException(ModuleName, ex);
				throw;
			}
			finally
			{
				//  ***
				//  API function: HidD_FreePreparsedData

				//  Purpose: frees the buffer reserved by HidD_GetPreparsedData.

				//  Accepts: A pointer to the PreparsedData structure returned by HidD_GetPreparsedData.

				//  Returns: True on success, False on failure.
				//  ***

				if (preparsedData != IntPtr.Zero)
				{
					NativeMethods.HidD_FreePreparsedData(preparsedData);
				}
			}
			return Capabilities;
		}

		///  <summary>
		///  reads a Feature report from the device.
		///  </summary>
		///  
		///  <param name="hidHandle"> the handle for learning about the device and exchanging Feature reports. </param>	
		///  <param name="inFeatureReportBuffer"> contains the requested report.</param>

		internal Boolean GetFeatureReport(SafeFileHandle hidHandle, ref Byte[] inFeatureReportBuffer)
		{
			try
			{
				Boolean success = false;

				//  ***
				//  API function: HidD_GetFeature
				//  Attempts to read a Feature report from the device.

				//  Requires:
				//  A handle to a HID
				//  A pointer to a buffer containing the report ID and report
				//  The size of the buffer. 

				//  Returns: true on success, false on failure.
				//  ***                    

				if (!hidHandle.IsInvalid && !hidHandle.IsClosed)
				{
					success = NativeMethods.HidD_GetFeature(hidHandle, inFeatureReportBuffer, inFeatureReportBuffer.Length);

					Debug.Print("HidD_GetFeature success = " + success);
				}
				return success;
			}

			catch (Exception ex)
			{
				DisplayException(ModuleName, ex);
				throw;
			}
		}

		/// <summary>
		/// Get the HID-class GUID
		/// </summary>
		/// <returns> the GUID </returns>

		internal Guid GetHidGuid()
		{
			Guid hidGuid = Guid.Empty;
			try
			{
				//  ***
				//  API function: 'HidD_GetHidGuid

				//  Purpose: Retrieves the interface class GUID for the HID class.

				//  Accepts: A System.Guid object for storing the GUID.
				//  ***

				NativeMethods.HidD_GetHidGuid(ref hidGuid);
			}

			catch (Exception ex)
			{
				DisplayException(ModuleName, ex);
				throw;
			}
			return hidGuid;
		}

		///  <summary>
		///  Creates a 32-bit Usage from the Usage Page and Usage ID. 
		///  Determines whether the Usage is a system mouse or keyboard.
		///  Can be modified to detect other Usages.
		///  </summary>
		///  
		///  <param name="myCapabilities"> a HIDP_CAPS structure retrieved with HidP_GetCaps. </param>
		///  
		///  <returns>
		///  A String describing the Usage.
		///  </returns>

		internal String GetHidUsage(NativeMethods.HIDP_CAPS myCapabilities)
		{
			String usageDescription = "";

			try
			{
				//  Create32-bit Usage from Usage Page and Usage ID.

				Int32 usage = myCapabilities.UsagePage * 256 + myCapabilities.Usage;

				if (usage == Convert.ToInt32(0X102))
				{
					usageDescription = "mouse";
				}
				if (usage == Convert.ToInt32(0X106))
				{
					usageDescription = "keyboard";
				}
			}

			catch (Exception ex)
			{
				DisplayException(ModuleName, ex);
				throw;
			}

			return usageDescription;
		}

		///  <summary>
		///  reads an Input report from the device using a control transfer.
		///  </summary>  
		///  <param name="hidHandle"> the handle for learning about the device and exchanging Feature reports. </param>
		/// <param name="inputReportBuffer"> contains the requested report. </param>

		internal Boolean GetInputReportViaControlTransfer(SafeFileHandle hidHandle, ref Byte[] inputReportBuffer)
		{
			var success = false;

			try
			{
				//  ***
				//  API function: HidD_GetInputReport

				//  Purpose: Attempts to read an Input report from the device using a control transfer.

				//  Requires:
				//  A handle to a HID
				//  A pointer to a buffer containing the report ID and report
				//  The size of the buffer. 

				//  Returns: true on success, false on failure.
				//  ***

				if (!hidHandle.IsInvalid && !hidHandle.IsClosed)
				{
					success = NativeMethods.HidD_GetInputReport(hidHandle, inputReportBuffer, inputReportBuffer.Length + 1);
					Debug.Print("HidD_GetInputReport success = " + success);
				}
				return success;
			}

			catch (Exception ex)
			{
				DisplayException(ModuleName, ex);
				throw;
			}
		}

		///  <summary>
		///  Reads an Input report from the device using an interrupt transfer.
		///  </summary>
		///  
		///  <param name="deviceData"> the Filestream for writing data. </param> 
		///  <param name="inputReportBuffer"> contains the report ID and report data. </param>
		/// <returns>
		///   True on success. False on failure.
		///  </returns>  

		internal async Task<Int32> GetInputReportViaInterruptTransfer(FileStream deviceData, Byte[] inputReportBuffer, CancellationTokenSource cts)
		{
			try
			{
				Int32 bytesRead = 0;
				
					// Begin reading an Input report. 
					Task<Int32> t = deviceData.ReadAsync(inputReportBuffer, 0, inputReportBuffer.Length, cts.Token);

					bytesRead = await t;

					// Gets to here only if the read operation completed before a timeout.

					//Debug.Print("Asynchronous read completed. Bytes read = " + Convert.ToString(bytesRead));

					// The operation has one of these completion states:

					switch (t.Status)
					{
						case TaskStatus.RanToCompletion:
							//Debug.Print("Input report received from device");
							break;
						case TaskStatus.Canceled:
							Debug.Print("Task canceled");
							break;
						case TaskStatus.Faulted:
							Debug.Print("Unhandled exception");
							break;
					}
				return bytesRead;
			}
			catch (Exception ex)
			{
				DisplayException(ModuleName, ex);
				throw;
			}
		}

		///  <summary>
		///  Retrieves the number of Input reports the HID driver will store.
		///  </summary>
		///  
		///  <param name="hidDeviceObject"> a handle to a device  </param>
		///  <param name="numberOfInputBuffers"> an integer to hold the returned value. </param>
		///  
		///  <returns>
		///  True on success, False on failure.
		///  </returns>

		internal Boolean GetNumberOfInputBuffers(SafeFileHandle hidDeviceObject, ref Int32 numberOfInputBuffers)
		{
			try
			{
				//  ***
				//  API function: HidD_GetNumInputBuffers

				//  Purpose: retrieves the number of Input reports the host can store.
				//  Not supported by Windows 98 Gold.
				//  If the buffer is full and another report arrives, the host drops the 
				//  ldest report.

				//  Accepts: a handle to a device and an integer to hold the number of buffers. 

				//  Returns: True on success, False on failure.
				//  ***

				Boolean success = NativeMethods.HidD_GetNumInputBuffers(hidDeviceObject, ref numberOfInputBuffers);

				return success;
			}

			catch (Exception ex)
			{
				DisplayException(ModuleName, ex);
				throw;
			}
		}

		/// <summary>
		/// Timeout if read or write via interrupt transfer doesn't return.
		/// </summary>

		internal void OnTimeout()
		{
			try
			{
				// No action required.

				Debug.Print("timeout");
			}
			catch (Exception ex)
			{
				DisplayException(ModuleName, ex);
				throw;
			}
		}

		/// <summary>
		/// Attempts to open a handle to a HID.
		/// </summary>
		/// <param name="devicePathName"> device path name returned by SetupDiGetDeviceInterfaceDetail </param>
		/// <param name="readAndWrite"> true if requesting read/write access for Input and Output reports </param>
		/// <returns> hidHandle - a handle to the HID </returns>

		internal SafeFileHandle OpenHandle(String devicePathName, Boolean readAndWrite)
		{
			SafeFileHandle hidHandle;

			try
			{
				if (readAndWrite)
				{
					//  ***
					//  API function:
					//  CreateFile

					//  Purpose:
					//  Retrieves a handle to a device.

					//  Accepts:
					//  A device path name returned by SetupDiGetDeviceInterfaceDetail
					//  The type of access requested (read/write).
					//  FILE_SHARE attributes to allow other processes to access the device while this handle is open.
					//  A Security structure or IntPtr.Zero. 
					//  A creation disposition value. Use OPEN_EXISTING for devices.
					//  Flags and attributes for files. Not used for devices.
					//  Handle to a template file. Not used.

					//  Returns: a handle without read or write access.
					//  This enables obtaining information about all HIDs, even system
					//  keyboards and mice. 
					//  Separate handles are used for reading and writing.
					//  ***

					hidHandle = FileIo.CreateFile(devicePathName, FileIo.GenericRead | FileIo.GenericWrite, FileIo.FileShareRead | FileIo.FileShareWrite, IntPtr.Zero, FileIo.OpenExisting, 0, IntPtr.Zero);
				}
				else
				{
					hidHandle = FileIo.CreateFile(devicePathName, 0, FileIo.FileShareRead | FileIo.FileShareWrite, IntPtr.Zero, FileIo.OpenExisting, 0, IntPtr.Zero);
				}
			}

			catch (Exception ex)
			{
				DisplayException(ModuleName, ex);
				throw;
			}
			return hidHandle;
		}

		///  <summary>
		///  Writes a Feature report to the device.
		///  </summary>
		///  
		///  <param name="outFeatureReportBuffer"> contains the report ID and report data. </param>
		///  <param name="hidHandle"> handle to the device.  </param>
		///  
		///  <returns>
		///   True on success. False on failure.
		///  </returns>            

		internal Boolean SendFeatureReport(SafeFileHandle hidHandle, Byte[] outFeatureReportBuffer)
		{
			try
			{
				//  ***
				//  API function: HidD_SetFeature

				//  Purpose: Attempts to send a Feature report to the device.

				//  Accepts:
				//  A handle to a HID
				//  A pointer to a buffer containing the report ID and report
				//  The size of the buffer. 

				//  Returns: true on success, false on failure.
				//  ***

				Boolean success = NativeMethods.HidD_SetFeature(hidHandle, outFeatureReportBuffer, outFeatureReportBuffer.Length);

				Debug.Print("HidD_SetFeature success = " + success);

				return success;
			}

			catch (Exception ex)
			{
				DisplayException(ModuleName, ex);
				throw;
			}
		}

		///  <summary>
		///  Writes an Output report to the device using a control transfer.
		///  </summary>
		///  
		///  <param name="outputReportBuffer"> contains the report ID and report data. </param>
		///  <param name="hidHandle"> handle to the device.  </param>
		///  
		///  <returns>
		///   True on success. False on failure.
		///  </returns>            

		internal Boolean SendOutputReportViaControlTransfer(SafeFileHandle hidHandle, Byte[] outputReportBuffer)
		{
			try
			{
				//  ***
				//  API function: HidD_SetOutputReport

				//  Purpose: 
				//  Attempts to send an Output report to the device using a control transfer.

				//  Accepts:
				//  A handle to a HID
				//  A pointer to a buffer containing the report ID and report
				//  The size of the buffer. 

				//  Returns: true on success, false on failure.
				//  ***                    

				Boolean success = NativeMethods.HidD_SetOutputReport(hidHandle, outputReportBuffer, outputReportBuffer.Length + 1);

				Debug.Print("HidD_SetOutputReport success = " + success);

				return success;
			}

			catch (Exception ex)
			{
				DisplayException(ModuleName, ex);
				throw;
			}
		}

		///  <summary>
		///  Writes an Output report to the device using an interrupt transfer.
		///  </summary>
		///  
		///  <param name="fileStreamDeviceData"> the Filestream for writing data. </param> 
		///  <param name="hidHandle"> SafeFileHandle to the device.  </param>
		///  <param name="outputReportBuffer"> contains the report ID and report data. </param>
		///  <param name="cts"> CancellationTokenSource </param>
		///  
		///  <returns>
		///   1 on success. 0 on failure.
		///  </returns>            

		internal async Task<Boolean> SendOutputReportViaInterruptTransfer
			(FileStream fileStreamDeviceData, SafeFileHandle hidHandle, Byte[] outputReportBuffer, CancellationTokenSource cts)
		{
			try
			{
				var success = false;
				
					// Begin writing the Output report. 

					Task t = fileStreamDeviceData.WriteAsync(outputReportBuffer, 0, outputReportBuffer.Length, cts.Token);

					await t;

					// Gets to here only if the write operation completed before a timeout.

					Debug.Print("Asynchronous write completed");

					// The operation has one of these completion states:

					switch (t.Status)
					{
						case TaskStatus.RanToCompletion:
							success = true;
							Debug.Print("Output report written to device");
							break;
						case TaskStatus.Canceled:
							Debug.Print("Task canceled");
							break;
						case TaskStatus.Faulted:
							Debug.Print("Unhandled exception");
							break;
					}

				return success;
			}
			catch (Exception ex)
			{
				DisplayException(ModuleName, ex);
				throw;
			}
		}

		///  <summary>
		///  sets the number of input reports the host HID driver store.
		///  </summary>
		///  
		///  <param name="hidDeviceObject"> a handle to the device.</param>
		///  <param name="numberBuffers"> the requested number of input reports.  </param>
		///  
		///  <returns>
		///  True on success. False on failure.
		///  </returns>

		internal Boolean SetNumberOfInputBuffers(SafeFileHandle hidDeviceObject, Int32 numberBuffers)
		{
			try
			{
				//  ***
				//  API function: HidD_SetNumInputBuffers

				//  Purpose: Sets the number of Input reports the host can store.
				//  If the buffer is full and another report arrives, the host drops the 
				//  oldest report.

				//  Requires:
				//  A handle to a HID
				//  An integer to hold the number of buffers. 

				//  Returns: true on success, false on failure.
				//  ***

				NativeMethods.HidD_SetNumInputBuffers(hidDeviceObject, numberBuffers);
				return true;
			}

			catch (Exception ex)
			{
				DisplayException(ModuleName, ex);
				throw;
			}
		}
	}
}
