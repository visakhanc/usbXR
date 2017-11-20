using System;
 
namespace GenericHid
{
	/// <summary>
	/// Used only in Debug.Write statements.
	/// </summary>
	/// 
	internal sealed partial class Debugging
    {         
        ///  <summary>
        ///  Get text that describes the result of an API call.
        ///  </summary>
        ///  
        ///  <param name="functionName"> the name of the API function. </param>
        ///  
        ///  <returns>
        ///  The text.
        ///  </returns>
          
        internal String ResultOfApiCall( String functionName ) 
        {
	        var resultString = new String(Convert.ToChar( 0 ), 129 ); 
            
            // Returns the result code for the last API call.
            
            Int32 resultCode = System.Runtime.InteropServices.Marshal.GetLastWin32Error(); 
            
            // Get the result message that corresponds to the code.

            Int64 temp = 0;
			Int32 bytes = NativeMethods.FormatMessage(NativeMethods.FormatMessageFromSystem, ref temp, resultCode, 0, resultString, 128, 0); 
            
            // Subtract two characters from the message to strip the CR and LF.
            
            if ( bytes > 2 ) 
            { 
                resultString = resultString.Remove( bytes - 2, 2 ); 
            }             
            // Create the String to return.

            resultString = Environment.NewLine + functionName + Environment.NewLine + "Result = " + resultString + Environment.NewLine; 
            
            return resultString;             
        }         
    }  
} 
