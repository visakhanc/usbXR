using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Timers;
using System.IO;
using System.Management;
using System.Runtime.InteropServices;
using System.Threading;
using Microsoft.Win32.SafeHandles;
using System.Diagnostics;
using System.Windows.Threading;
using GenericHid;

namespace usbXR_receive
{

    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private Hid _myHid;
        private DispatcherTimer transferTimer;
        private DispatcherTimer freqCalculationTimer;
        private IntPtr _deviceNotificationHandle;
        private Boolean _deviceDetected;
        private FileStream _deviceData;
        private SafeFileHandle _hidHandle;
        private Int32 _myProductId;
        private Int32 _myVendorId;
        private ManagementEventWatcher _deviceArrivedWatcher;
        private ManagementEventWatcher _deviceRemovedWatcher;
        private Boolean _deviceHandleObtained;
        private readonly DeviceManagement _myDeviceManagement = new DeviceManagement();
        private String _hidUsage;
        private Boolean _transferInProgress;

        private int totalInputReports;
        private int SensorValue;
        private Byte[] inputReportBuf;
        private PointCollection accXPoints;
        private PointCollection velXPoints;
        private PointCollection accYPoints;
        private PointCollection velYPoints;
        private PointCollection accZPoints;
        private PointCollection velZPoints;
        Thread getInputThread;
        private int inputReportFrequency;
        private int prevReportCount;
        private double accXMax;
        private double accXMin;
        private double accXFreq;
        private int accXCount;

        private enum WmiDeviceProperties
        {
            Name,
            Caption,
            Description,
            Manufacturer,
            PNPDeviceID,
            DeviceID,
            ClassGUID
        }

        public MainWindow()
        {
            InitializeComponent();
            Startup();
        }

        private void Startup()
        {
            try
            {
                _myHid = new Hid();

                transferTimer = new DispatcherTimer(DispatcherPriority.Normal);
                transferTimer.Interval = new TimeSpan(0, 0, 0, 0, 30);
                transferTimer.Tick += DoPeriodicTransfers;
                transferTimer.Stop();
                freqCalculationTimer = new DispatcherTimer(DispatcherPriority.Normal);
                freqCalculationTimer.Interval = new TimeSpan(0, 0, 0, 1);
                freqCalculationTimer.Tick += DoFreqCalculation;
                freqCalculationTimer.Stop();
                //_periodicTransfersRequested = false;
                totalInputReports = 0;
                prevReportCount = 0;
                accXFreq = accXMax = accXMin = accXCount = 0;
                _transferInProgress = false;
                getInputThread = new Thread(new ThreadStart(GetInputThreadFunction));
                AccXPolyline.Points = accXPoints = new PointCollection();
                VelXPolyline.Points = velXPoints = new PointCollection();
                AccYPolyline.Points = accYPoints = new PointCollection();
                VelYPolyline.Points = velYPoints = new PointCollection();
                AccZPolyline.Points = accZPoints = new PointCollection();
                VelZPolyline.Points = velZPoints = new PointCollection();
                DeviceNotificationsStart();
                FindDeviceUsingWmi();
                FindTheHid();
#if false
                if (_myHid.Capabilities.InputReportByteLength > 0)
                {
                    inputReportBuf = new Byte[_myHid.Capabilities.InputReportByteLength];
                }
#endif
                
            }
            catch (Exception ex)
            {
                DisplayException(Name, ex);
                throw;
            }
        }



        ///  <summary>
        ///  Add a handler to detect arrival of devices using WMI.
        ///  </summary>

        private void AddDeviceArrivedHandler()
        {
            const Int32 pollingIntervalSeconds = 3;
            var scope = new ManagementScope("root\\CIMV2");
            scope.Options.EnablePrivileges = true;

            try
            {
                var q = new WqlEventQuery();
                q.EventClassName = "__InstanceCreationEvent";
                q.WithinInterval = new TimeSpan(0, 0, pollingIntervalSeconds);
                q.Condition = @"TargetInstance ISA 'Win32_USBControllerdevice'";
                _deviceArrivedWatcher = new ManagementEventWatcher(scope, q);
                _deviceArrivedWatcher.EventArrived += DeviceAdded;

                _deviceArrivedWatcher.Start();
            }
            catch (Exception e)
            {
                Debug.WriteLine(e.Message);
                if (_deviceArrivedWatcher != null)
                    _deviceArrivedWatcher.Stop();
            }
        }



        ///  <summary>
        ///  Add a handler to detect removal of devices using WMI.
        ///  </summary>

        private void AddDeviceRemovedHandler()
        {
            const Int32 pollingIntervalSeconds = 3;
            var scope = new ManagementScope("root\\CIMV2");
            scope.Options.EnablePrivileges = true;

            try
            {
                var q = new WqlEventQuery();
                q.EventClassName = "__InstanceDeletionEvent";
                q.WithinInterval = new TimeSpan(0, 0, pollingIntervalSeconds);
                q.Condition = @"TargetInstance ISA 'Win32_USBControllerdevice'";
                _deviceRemovedWatcher = new ManagementEventWatcher(scope, q);
                _deviceRemovedWatcher.EventArrived += DeviceRemoved;
                _deviceRemovedWatcher.Start();
            }
            catch (Exception e)
            {
                Debug.WriteLine(e.Message);
                if (_deviceRemovedWatcher != null)
                    _deviceRemovedWatcher.Stop();
            }
        }

        /// <summary>
        /// Close the handle and FileStreams for a device.
        /// </summary>
        /// 
        private void CloseCommunications()
        {
            if (_deviceData != null)
            {
                _deviceData.Close();
            }

            if ((_hidHandle != null) && (!(_hidHandle.IsInvalid)))
            {
                _hidHandle.Close();
            }

            // The next attempt to communicate will get a new handle and FileStreams.
            _deviceHandleObtained = false;
        }


        ///  <summary>
        ///  Called on arrival of any device.
        ///  Calls a routine that searches to see if the desired device is present.
        ///  </summary>

        private void DeviceAdded(object sender, EventArrivedEventArgs e)
        {
            try
            {
                Debug.WriteLine("A USB device has been inserted");

                _deviceDetected = FindDeviceUsingWmi();
            }
            catch (Exception ex)
            {
                DisplayException(Name, ex);
                throw;
            }
        }



        ///  <summary>
        ///  Add handlers to detect device arrival and removal.
        ///  </summary>

        private void DeviceNotificationsStart()
        {
            AddDeviceArrivedHandler();
            AddDeviceRemovedHandler();
        }



        ///  <summary>
        ///  Stop receiving notifications about device arrival and removal
        ///  </summary>

        private void DeviceNotificationsStop()
        {
            try
            {
                if (_deviceArrivedWatcher != null)
                    _deviceArrivedWatcher.Stop();
                if (_deviceRemovedWatcher != null)
                    _deviceRemovedWatcher.Stop();
            }
            catch (Exception ex)
            {
                DisplayException(Name, ex);
                throw;
            }
        }



        ///  <summary>
        ///  Called on removal of any device.
        ///  Calls a routine that searches to see if the desired device is still present.
        ///  </summary>
        /// 
        private void DeviceRemoved(object sender, EventArgs e)
        {
            try
            {
                Debug.WriteLine("A USB device has been removed");

                _deviceDetected = FindDeviceUsingWmi();

                if (!_deviceDetected)
                {
                    _deviceHandleObtained = false;
                    CloseCommunications();
                }
            }
            catch (Exception ex)
            {
                DisplayException(Name, ex);
                throw;
            }
        }



        ///  <summary>
        ///  Use the System.Management class to find a device by Vendor ID and Product ID using WMI. If found, display device properties.
        ///  </summary>
        /// <remarks> 
        /// During debugging, if you stop the firmware but leave the device attached, the device may still be detected as present
        /// but will be unable to communicate. The device will show up in Windows Device Manager as well. 
        /// This situation is unlikely to occur with a final product.
        /// </remarks>

        private Boolean FindDeviceUsingWmi()
        {
            try
            {
                // Prepend "@" to string below to treat backslash as a normal character (not escape character):

                String deviceIdString = @"USB\VID_" + _myVendorId.ToString("X4") + "&PID_" + _myProductId.ToString("X4");

                _deviceDetected = false;
                var searcher = new ManagementObjectSearcher("root\\CIMV2", "SELECT * FROM Win32_PnPEntity");

                foreach (ManagementObject queryObj in searcher.Get())
                {
                    if (queryObj["PNPDeviceID"].ToString().Contains(deviceIdString))
                    {
                        _deviceDetected = true;
                        //InfoBox.Text += "My device found (WMI)";
                        Debug.WriteLine("My device found (WMI)");

                        // Display device properties.

                        foreach (WmiDeviceProperties wmiDeviceProperty in Enum.GetValues(typeof(WmiDeviceProperties)))
                        {
                            //InfoBox.Text += wmiDeviceProperty.ToString() + ": " + queryObj[wmiDeviceProperty.ToString()];
                            Debug.WriteLine(wmiDeviceProperty.ToString() + ": {0}", queryObj[wmiDeviceProperty.ToString()]);
                        }
                    }
                }
                if (!_deviceDetected)
                {
                    //InfoBox.Text += "My device not found (WMI)";
                    Debug.WriteLine("My device not found (WMI)");
                }
                return _deviceDetected;
            }
            catch (Exception ex)
            {
                DisplayException(Name, ex);
                throw;
            }
        }



        ///  <summary>
        ///  Call HID functions that use Win32 API functions to locate a HID-class device
        ///  by its Vendor ID and Product ID. Open a handle to the device.
        ///  </summary>
        ///          
        ///  <returns>
        ///   True if the device is detected, False if not detected.
        ///  </returns>

        private Boolean FindTheHid()
        {
            var devicePathName = new String[128];
            String myDevicePathName = "";

            try
            {
                _deviceHandleObtained = false;
                CloseCommunications();
                _myVendorId = 0x16c0;
                _myProductId = 0x05df;
                // Get the HID-class GUID.
                Guid hidGuid = _myHid.GetHidGuid();

                //  Fill an array with the device path names of all attached HIDs.
                Boolean availableHids = _myDeviceManagement.FindDeviceFromGuid(hidGuid, ref devicePathName);

                //  If there is at least one HID, attempt to read the Vendor ID and Product ID
                //  of each device until there is a match or all devices have been examined.

                if (availableHids)
                {
                    Int32 memberIndex = 0;
                    do
                    {
                        // Open the handle without read/write access to enable getting information about any HID, even system keyboards and mice.
                        _hidHandle = _myHid.OpenHandle(devicePathName[memberIndex], false);

                        if (!_hidHandle.IsInvalid)
                        {
                            // The returned handle is valid, 
                            // so find out if this is the device we're looking for.

                            //_myHid.DeviceAttributes.Size = Marshal.SizeOf(_myHid.DeviceAttributes);

                            Boolean success = _myHid.GetAttributes(_hidHandle, ref _myHid.DeviceAttributes);
                            if (success)
                            {
                                //Debug.WriteLine("  HIDD_ATTRIBUTES structure filled without error.");
                                //Debug.WriteLine("  Structure size: " + _myHid.DeviceAttributes.Size);
                                //Debug.WriteLine("  Vendor ID: " + Convert.ToString(_myHid.DeviceAttributes.VendorID, 16));
                                //Debug.WriteLine("  Product ID: " + Convert.ToString(_myHid.DeviceAttributes.ProductID, 16));
                                //Debug.WriteLine("  Version Number: " + Convert.ToString(_myHid.DeviceAttributes.VersionNumber, 16));

                                if ((_myHid.DeviceAttributes.VendorID == _myVendorId) && (_myHid.DeviceAttributes.ProductID == _myProductId))
                                {
                                    //Debug.WriteLine("  Handle obtained to my device");
                                    //  Display the information in form's list box.
                                    //InfoBox.Text += "\nHandle obtained to my device:";
                                    InfoBox.Text = "  VID=" + Convert.ToString(_myHid.DeviceAttributes.VendorID, 16);
                                    InfoBox.Text += "  PID=" + Convert.ToString(_myHid.DeviceAttributes.ProductID, 16);
                                    _deviceHandleObtained = true;

                                    myDevicePathName = devicePathName[memberIndex];
                                }
                                else
                                {
                                    //  It's not a match, so close the handle.

                                    _deviceHandleObtained = false;
                                    _hidHandle.Close();
                                }
                            }
                            else
                            {
                                //  There was a problem retrieving the information.

                                //Debug.WriteLine("  Error in filling HIDD_ATTRIBUTES structure.");
                                _deviceHandleObtained = false;
                                _hidHandle.Close();
                            }
                        }

                        //  Keep looking until we find the device or there are no devices left to examine.

                        memberIndex = memberIndex + 1;
                    }
                    while (!((_deviceHandleObtained || (memberIndex == devicePathName.Length))));
                }

                if (_deviceHandleObtained)
                {
                    //  The device was detected.
                    //  Learn the capabilities of the device.

                    _myHid.Capabilities = _myHid.GetDeviceCapabilities(_hidHandle);

                    //  Find out if the device is a system mouse or keyboard.
                    _hidUsage = _myHid.GetHidUsage(_myHid.Capabilities);

                    //Close the handle and reopen it with read/write access.
                    _hidHandle.Close();
                    _hidHandle = _myHid.OpenHandle(myDevicePathName, true);
                    if (_hidHandle.IsInvalid)
                    {
                        InfoBox.Text += "The device is a system " + _hidUsage + ".";
                    }
                    else
                    {
                        if (_myHid.Capabilities.InputReportByteLength > 0)
                        {
                            //  Set the size of the Input report buffer. 
                            var inputReportBuffer = new Byte[_myHid.Capabilities.InputReportByteLength];
                            _deviceData = new FileStream(_hidHandle, FileAccess.Read | FileAccess.Write, inputReportBuffer.Length, false);
                            inputReportBuf = new Byte[_myHid.Capabilities.InputReportByteLength];
                        }

                        if (_myHid.Capabilities.OutputReportByteLength > 0)
                        {
                            Byte[] outputReportBuffer = null;
                        }
                        //  Flush any waiting reports in the input buffer. (optional)
                        _myHid.FlushQueue(_hidHandle);
                    }
                    ErrorBox.Text = "";
                }
                else
                {
                    ErrorBox.Text = "Device not found.";
                }
                return _deviceHandleObtained;
            }
            catch (Exception ex)
            {
                DisplayException(Name, ex);
                throw;
            }
        }

        ///  <summary>
        ///  Provides a central mechanism for exception handling.
        ///  Displays a message box that describes the exception.
        ///  </summary>
        ///  
        ///  <param name="moduleName"> the module where the exception occurred. </param>
        ///  <param name="e"> the exception </param>

        internal static void DisplayException(String moduleName, Exception e)
        {
            //  Create an error message.
            String message = "Exception: " + e.Message + Environment.NewLine + "Module: " + moduleName + Environment.NewLine + "Method: " + e.TargetSite.Name;
            const String caption = "Unexpected Exception";
            MessageBox.Show(message, caption, MessageBoxButton.OK);
        }

        private void StartStopButton_Click(object sender, RoutedEventArgs e)
        {
            if(StartStopButton.Content.ToString() == "Start")
            {
                //StartStopButton.Content = "Stop";
                //transferTimer.Start();
                if (!_deviceHandleObtained)
                {
                    FindTheHid();
                }
                getInputThread.Start();
                freqCalculationTimer.Start();
                StartStopButton.IsEnabled = false;
            }
        }

        private void DoPeriodicTransfers(object source, object e)
        {
            try
            {
                if (!_transferInProgress)
                {
                    RequestToGetInputReport();
                }
            }
            catch (Exception ex)
            {
                DisplayException(Name, ex);
                throw;
            }
        }


        private void DoFreqCalculation(object source, object e)
        {
            inputReportFrequency = totalInputReports - prevReportCount;
            prevReportCount = totalInputReports;
            ValueTextBlock.Text = inputReportFrequency.ToString();
            //AccXMaxText.Text = ((int)accXMax).ToString();
            //AccXMinText.Text = ((int)accXMin).ToString();
        }

        private async void GetInputThreadFunction()
        {
            int count = 0;
            double accXVal, accYVal, accZVal;
            double velXVal = 0, velYVal = 0, velZVal = 0;
            Vector x_shift = new Vector(-1, 0);
            //int time = 0;
            Action onReadTimeoutAction = OnReadTimeout; // Create a delegate to execute on a timeout.
            var cts = new CancellationTokenSource(); // The CancellationTokenSource specifies the timeout value and the action to take on a timeout.
            cts.Token.Register(onReadTimeoutAction); // Specify the function to call on a timeout.
            PointCollection points = new PointCollection();
            bool zcDetect = true;
            int oldCount = 0;
            int packetCounterVal;
            int lostPacketCount = 0;
            int prevPacketCounter = 0;
            double lossPercentage = 0;
            double prevLossPercentage = 0; ;

            while (true)
            {
                try
                {
                    if (_deviceHandleObtained && (inputReportBuf != null))
                    {
                        cts.CancelAfter(60*1000);  // Cancel the read if it hasn't completed after a timeout.
                        
                        // Stops waiting when data is available or on timeout:
                        Int32 bytesRead = await _myHid.GetInputReportViaInterruptTransfer(_deviceData, inputReportBuf, cts);
                        if (bytesRead > 0)
                        {
                            ErrorBox.Dispatcher.Invoke(() => { ErrorBox.Text = "bytes read (includes report ID) = " + Convert.ToString(bytesRead); });
                            //for (int i = 0; i < inputReportBuffer.Length; i++)
                            //   InfoBox.Text += inputReportBuffer[i].ToString() + " ";
                            packetCounterVal = inputReportBuf[1] + inputReportBuf[2] * 256;
                            if(0 != prevPacketCounter) {
                                if(packetCounterVal != (prevPacketCounter + 1)) {
                                    lostPacketCount += (packetCounterVal - (prevPacketCounter + 1));
                                }
                            }
                            prevPacketCounter = packetCounterVal;
                            

                            //ValueTextBlock.Dispatcher.Invoke(() => { ValueTextBlock.Text = ((sbyte)inputReportBuf[3]).ToString(); });
                            //AccValProgressBar.Dispatcher.Invoke(() => { AccValProgressBar.Value = (double)((sbyte)inputReportBuf[3]); });
                            totalInputReports++;
                            accXVal = (double)((sbyte)inputReportBuf[3]);
                            accYVal = (double)((sbyte)inputReportBuf[4]);
                            accZVal = (double)((sbyte)(inputReportBuf[5] - 64));
                            velXVal += accXVal * 0.0327 * 15.31 / 2.0;
                            velYVal += accYVal * 0.0327 * 15.31 / 2.0;
                            velZVal += accZVal * 0.0327 * 15.31 / 2.0;
                            if(accXVal > accXMax) { accXMax = accXVal; }
                            if(accXVal < accXMin) { accXMin = accXVal; }
                            if (accXVal > 0)
                            {
                                zcDetect = true;
                            }
                            if((accXVal < 0) && (zcDetect == true))
                            {
                                if ((oldCount > 0) && (inputReportFrequency > 0))
                                {
                                    accXCount++;
                                    accXFreq = (double)inputReportFrequency / (double)(totalInputReports - oldCount);
                                    AccXFreqText.Dispatcher.Invoke(() => 
                                    {
                                        AccXFreqText.Text = string.Format("{0}", (int)(accXFreq * 60));
                                        AccXCountText.Text = string.Format("{0}", accXCount);
                                    });
                                    
                                }
                                oldCount = totalInputReports;
                                zcDetect = false;
                            }
                            count++;
                            AccXPolyline.Dispatcher.Invoke(() =>
                            {
                                if (count >= 200)
                                {
                                    accXPoints.RemoveAt(0);
                                    //velXPoints.RemoveAt(0);
                                    accYPoints.RemoveAt(0);
                                    //velYPoints.RemoveAt(0);
                                    accZPoints.RemoveAt(0);
                                    //velZPoints.RemoveAt(0);
                                    for (int i = 0; i < accXPoints.Count; i++)
                                    {
                                        accXPoints[i] += x_shift;
                                       // velXPoints[i] += x_shift;
                                        accYPoints[i] += x_shift;
                                       // velYPoints[i] += x_shift;
                                        accZPoints[i] += x_shift;
                                       // velZPoints[i] += x_shift;
                                    }
                                    accXPoints.Add(new Point(200, 150 + accXVal));
                                  //  velXPoints.Add(new Point(200, 150 + velXVal));
                                    accYPoints.Add(new Point(200, 150 + accYVal));
                                  //  velYPoints.Add(new Point(200, 150 + velYVal));
                                    accZPoints.Add(new Point(200, 150 + accZVal));
                                  //  velZPoints.Add(new Point(200, 150 + velZVal));
                                }
                                else
                                {
                                    accXPoints.Add(new Point(count, 150 + accXVal));
                                   // velXPoints.Add(new Point(count * 4, 150 + velXVal));
                                    accYPoints.Add(new Point(count, 150 + accYVal));
                                   // velYPoints.Add(new Point(count * 4, 150 + velYVal));
                                    accZPoints.Add(new Point(count, 150 + accZVal));
                                  //  velZPoints.Add(new Point(count * 4, 150 + velZVal));
                                }
                            });

                            CountTextBlock.Dispatcher.Invoke(() => 
                            {
                                if (totalInputReports > 0) {
                                    lossPercentage = (double)(lostPacketCount) * 100 / totalInputReports;
                                }
                                CountTextBlock.Text = totalInputReports.ToString();
                                LossTextBlock.Text = string.Format("{0:0.0}%", lossPercentage);
                                LossTextBlock.Foreground = (lossPercentage > prevLossPercentage) ? Brushes.Red : Brushes.Green;
                                prevLossPercentage = lossPercentage;
                            });

                        }
                    }
                }
                catch (Exception ex)
                {
                    DisplayException(Name, ex);
                }
            }
        }



        ///  <summary>
        ///  Request an Input report.
        ///  Assumes report ID = 0.
        ///  </summary>

        private async void RequestToGetInputReport()
        {
            const Int32 readTimeout = 1000;

            String byteValue = null;
            Byte[] inputReportBuffer = null;

            try
            {
                Boolean success = false;

                //  If the device hasn't been detected, was removed, or timed out on a previous attempt
                //  to access it, look for the device.
                if (!_deviceHandleObtained)
                {
                    _deviceHandleObtained = FindTheHid();
                }

                if (_deviceHandleObtained)
                {
                    //  Don't attempt to exchange reports if valid handles aren't available
                    //  (as for a mouse or keyboard under Windows 2000 and later.)

                    if (!_hidHandle.IsInvalid)
                    {
                        //  Read an Input report.

                        //  Don't attempt to send an Input report if the HID has no Input report.
                        //  (The HID spec requires all HIDs to have an interrupt IN endpoint,
                        //  which suggests that all HIDs must support Input reports.)

                        if (_myHid.Capabilities.InputReportByteLength > 0)
                        {
                            //  Set the size of the Input report buffer. 

                            inputReportBuffer = new Byte[_myHid.Capabilities.InputReportByteLength];
                            _transferInProgress = true;

                            //  Read a report using interrupt transfers. 
                            //  Timeout if no report available.
                            //  To enable reading a report without blocking the calling thread, uses Filestream's ReadAsync method.                                               

                            // Create a delegate to execute on a timeout.
                            Action onReadTimeoutAction = OnReadTimeout;
                            // The CancellationTokenSource specifies the timeout value and the action to take on a timeout.
                            var cts = new CancellationTokenSource();
                            // Cancel the read if it hasn't completed after a timeout.
                            cts.CancelAfter(readTimeout);
                            // Specify the function to call on a timeout.
                            cts.Token.Register(onReadTimeoutAction);

                            // Stops waiting when data is available or on timeout:
                            Int32 bytesRead = await _myHid.GetInputReportViaInterruptTransfer(_deviceData, inputReportBuffer, cts);

                            // Arrive here only if the operation completed.

                            // Dispose to stop the timeout timer. 
                            cts.Dispose();

                            _transferInProgress = false;
                            if (bytesRead > 0)
                            {
                                success = true;
                                ErrorBox.Dispatcher.Invoke(() => { ErrorBox.Text = "bytes read (includes report ID) = " + Convert.ToString(bytesRead); });
                                //for (int i = 0; i < inputReportBuffer.Length; i++)
                                //   InfoBox.Text += inputReportBuffer[i].ToString() + " ";
                                SensorValue = inputReportBuffer[1] + inputReportBuffer[2] * 256;
                                //ValueTextBlock.Text = SensorValue.ToString();
                                ValueTextBlock.Dispatcher.Invoke(() => { ValueTextBlock.Text = ((sbyte)inputReportBuffer[3]).ToString(); });
                                //AccValProgressBar.Dispatcher.Invoke(() => { AccValProgressBar.Value = (double)((sbyte)inputReportBuffer[3]); });
                            }

                        }
                        else
                        {
                            ErrorBox.Dispatcher.Invoke(() => { ErrorBox.Text = "No attempt to read an Input report was made."; });
                            ErrorBox.Dispatcher.Invoke(() => { ErrorBox.Text += "The HID doesn't have an Input report."; });
                        }
                    }
                    else
                    {
                        ErrorBox.Dispatcher.Invoke(() => { ErrorBox.Text = "Invalid handle."; });
                        ErrorBox.Dispatcher.Invoke(() => { ErrorBox.Text += "No attempt to write an Output report or read an Input report was made."; });
                    }

                    if (success)
                    {
                        totalInputReports++;
                        CountTextBlock.Dispatcher.Invoke(() => { CountTextBlock.Text = totalInputReports.ToString(); });
                        //ErrorBox.Text += string.Format("\nTotal: {0}", totalInputReports);
                    }
                    else
                    {
                        CloseCommunications();
                        InfoBox.Dispatcher.Invoke(() => { InfoBox.Text = "The attempt to read an Input report has failed."; });
                    }

                }
            }

            catch (Exception ex)
            {
                //DisplayException(Name, ex);
                throw new Exception(string.Format("Exception: {0}", ex.Message));
            }
        }


        /// <summary>
        /// Timeout if read via interrupt transfer doesn't return.
        /// </summary>

        private void OnReadTimeout()
        {
            try
            {
                ErrorBox.Dispatcher.Invoke(() => { ErrorBox.Text = "The attempt to read a report timed out."; });
                //CloseCommunications();
                _transferInProgress = false;
            }
            catch (Exception ex)
            {
                DisplayException(Name, ex);
                throw;
            }
        }

    }
}


