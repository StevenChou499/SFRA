using ScottPlot;
using ScottPlot.Plottables;
using System.IO;
using System.IO.Ports;
using System.Text;
using System.Threading;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace SFRA_Host
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        enum SFRA_Cmd
        {
            NO_CMD,
            SFRA_RESET,
            START_SWEEP,
            GET_STATUS,
            GET_BODE,
            SET_START_FREQ,
            SET_STEP_FREQ,
            SET_SAMP_FREQ,
            SET_INPUT_AMP,
        }

        enum SFRA_STATUS
        {
            IDLE, 
            SFRA_INIT, 
            SWEEP_INIT, 
            SWEEPING, 
            SWEEP_DONE, 
            SFRA_DONE, 
        }

        struct CmdItem
        {
            public SFRA_Cmd cmd;
            public float arg;
        }

        SerialPort? mySerialPort = null;
        readonly Thread? bg_thread = null;
        bool connected = false;
        byte[] rx_buffer = new byte[1000];
        byte[] tx_buffer = new byte[10];
        float start_freq_value = 0.0f;
        float step_freq_value = 0.0f;
        float samp_freq_vaule = 0.0f;
        float input_amp_value = 0.0f; 
        Mutex mutex_lock = new Mutex();
        Queue<CmdItem> Cmd_Queue = new Queue<CmdItem>();
        Crosshair mag_crosshair;
        Crosshair pha_crosshair;

        public MainWindow()
        {
            InitializeComponent();
            
            Dictionary<SFRA_Cmd, Action<float>> cmdTable = new Dictionary<SFRA_Cmd, Action<float>>
            {
                [SFRA_Cmd.NO_CMD] = null_handler,
                [SFRA_Cmd.SFRA_RESET] = sfra_reset,
                [SFRA_Cmd.START_SWEEP] = sfra_start,
                [SFRA_Cmd.GET_STATUS] = sfra_get_status,
                [SFRA_Cmd.GET_BODE] = sfra_get_bode,
                [SFRA_Cmd.SET_START_FREQ] = sfra_set_start_freq,
                [SFRA_Cmd.SET_STEP_FREQ] = sfra_set_step_freq,
                [SFRA_Cmd.SET_SAMP_FREQ] = sfra_set_samp_freq,
                [SFRA_Cmd.SET_INPUT_AMP] = sfra_set_input_amp,
            };
            Mag_Plot.MouseMove += Mag_MouseMove;
            Pha_Plot.MouseMove += Pha_MouseMove;
            mag_crosshair = Mag_Plot.Plot.Add.Crosshair(0, 0);
            pha_crosshair = Pha_Plot.Plot.Add.Crosshair(0, 0);
            mag_crosshair.IsVisible = true;
            pha_crosshair.IsVisible = true;

            bg_thread = new Thread(Background_Comm_Thread);
            bg_thread.Start();
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            if (mySerialPort is not null && mySerialPort.IsOpen)
            {
                mySerialPort.Close();
                mySerialPort = null;
            }
        }

        private void ShowComportList(object sender, EventArgs e)
        {
            string[] comport_list = SerialPort.GetPortNames();
            ComportList.ItemsSource = comport_list;
        }

        private void Connect_Btn_Clicked(object sender, RoutedEventArgs e)
        {
            if (connected == false)
            {
                mySerialPort = new SerialPort(ComportList.Text, 115200);
                try
                {
                    mySerialPort.Open();
                    mySerialPort.ReadTimeout = 2000;
                    connected = true;
                    Connect_Btn.Content = "Disconnect";
                    ComportList.IsEnabled = false;
                }
                catch (UnauthorizedAccessException ex)
                {
                    Console.WriteLine($"Error: {ex.Message}");
                }
                catch (IOException ex)
                {
                    Console.WriteLine($"Error: {ex.Message}");
                }
            }
            else
            {
                if (mySerialPort is not null && mySerialPort.IsOpen)
                {
                    mySerialPort.Close();
                    connected = false;
                    Connect_Btn.Content = "Connect";
                    ComportList.IsEnabled = true;
                }
            }
        }

        private void Set_Start_Frequency(object sender, RoutedEventArgs e)
        {
            try
            {
                float start_freq = Convert.ToSingle(StartFreqBox.Text);
                Cmd_Queue.Enqueue(new CmdItem { cmd = SFRA_Cmd.SET_START_FREQ, arg = start_freq });
            }
            catch (FormatException ex)
            {
                Console.WriteLine($"Error: {ex.Message}");
            }            
        }

        private void Set_Step_Frequency(object sender, RoutedEventArgs e)
        {
            try
            {
                float step_freq = Convert.ToSingle(StepFreqBox.Text);
                Cmd_Queue.Enqueue(new CmdItem { cmd = SFRA_Cmd.SET_STEP_FREQ, arg = step_freq });
            }
            catch (FormatException ex)
            {
                Console.WriteLine($"Error: {ex.Message}");
            }
        }

        private void Set_Samp_Frequency(object sender, RoutedEventArgs e)
        {
            try
            {
                float samp_freq = Convert.ToSingle(SampFreqBox.Text);
                Cmd_Queue.Enqueue(new CmdItem { cmd = SFRA_Cmd.SET_SAMP_FREQ, arg = samp_freq });
            }
            catch (FormatException ex)
            {
                Console.WriteLine($"Error: {ex.Message}");
            }
        }

        private void Set_Inject_Amplitude(object sender, RoutedEventArgs e)
        {
            try
            {
                float inject_amp = Convert.ToSingle(InjAmpBox.Text);
                Cmd_Queue.Enqueue(new CmdItem { cmd = SFRA_Cmd.SET_INPUT_AMP, arg = inject_amp });
            }
            catch (FormatException ex)
            {
                Console.WriteLine($"Error: {ex.Message}");
            }
        }

        private void Start_SFRA(object sender, RoutedEventArgs e)
        {
            Cmd_Queue.Enqueue(new CmdItem { cmd = SFRA_Cmd.START_SWEEP, arg = 0.0f });
        }

        private void Reset_SFRA(object sender, RoutedEventArgs e)
        {
            Cmd_Queue.Enqueue(new CmdItem { cmd = SFRA_Cmd.SFRA_RESET, arg = 0.0f });
        }

        private void Get_Bode(object sender, RoutedEventArgs e)
        {
            Cmd_Queue.Enqueue(new CmdItem { cmd = SFRA_Cmd.GET_BODE, arg = 0.0f });
        }

        private async void Background_Comm_Thread()
        {
            while (true)
            {
                if (mySerialPort is not null && connected)
                {
                    while (Cmd_Queue.Count > 0)
                    {
                        CmdItem popped = Cmd_Queue.Dequeue();
                        mutex_lock.WaitOne();
                        switch (popped.cmd)
                        {
                            case SFRA_Cmd.SFRA_RESET:
                                sfra_reset(popped.arg);
                                break;
                            case SFRA_Cmd.START_SWEEP:
                                sfra_start(popped.arg);
                                break;
                            case SFRA_Cmd.GET_STATUS:
                                sfra_get_status(popped.arg);
                                break;
                            case SFRA_Cmd.GET_BODE:
                                sfra_get_bode(popped.arg);
                                break;
                            case SFRA_Cmd.SET_START_FREQ:
                                sfra_set_start_freq(popped.arg);
                                break;
                            case SFRA_Cmd.SET_STEP_FREQ:
                                sfra_set_step_freq(popped.arg);
                                break;
                            case SFRA_Cmd.SET_SAMP_FREQ:
                                sfra_set_samp_freq(popped.arg);
                                break;
                            case SFRA_Cmd.SET_INPUT_AMP:
                                sfra_set_input_amp(popped.arg);
                                break;
                            case SFRA_Cmd.NO_CMD:
                            default:
                                null_handler(popped.arg);
                                break;
                        }
                        mutex_lock.ReleaseMutex();
                    }
                }
                Cmd_Queue.Enqueue(new CmdItem { cmd = SFRA_Cmd.GET_STATUS, arg = 0.0f });
                await Task.Delay(1000);
            }
        }

        void Safe_Read(ref byte[] input_buffer, int offset, int length)
        {
            int read_length = 0;
            while (read_length < length)
            {
                try
                {
                    read_length += mySerialPort.Read(input_buffer, offset + read_length, length - read_length);
                }
                catch (TimeoutException ex)
                {
                    Console.WriteLine($"Error: {ex.Message}");
                }

            }
        }

        void null_handler(float arg)
        {
            return;
        }

        void sfra_reset(float arg)
        {
            tx_buffer = new byte[] { 0xAA, 0x01, 0x01, 0xAA };
            mySerialPort.Write(tx_buffer, 0, 4);
            Safe_Read(ref rx_buffer, 0, 6);
            return;
        }

        void sfra_start(float arg)
        {
            tx_buffer = new byte[] { 0xAA, 0x02, 0x01, 0xA8 };
            mySerialPort.Write(tx_buffer, 0, 4);
            Safe_Read(ref rx_buffer, 0, 6);
            return;
        }

        void sfra_get_status(float arg)
        {
            tx_buffer = new byte[] { 0xAA, 0x03, 0x01, 0xA8 };
            mySerialPort.Write(tx_buffer, 0, 4);
            Safe_Read(ref rx_buffer, 0, 6);
            string hexString = BitConverter.ToString(rx_buffer, 0, 6);
            Console.WriteLine(hexString);
            SFRA_STATUS sfra_status = (SFRA_STATUS)rx_buffer[4];
            Application.Current.Dispatcher.BeginInvoke(() =>
            { 
                switch (sfra_status)
                {
                    case SFRA_STATUS.IDLE:
                        SFRAStatusLabel.Content = "IDLE";
                        SFRAStatusLabel.Foreground = Brushes.Black;
                        break;
                    case SFRA_STATUS.SFRA_INIT:
                        SFRAStatusLabel.Content = "SFRA INIT";
                        SFRAStatusLabel.Foreground = Brushes.Black;
                        break;
                    case SFRA_STATUS.SWEEP_INIT:
                        SFRAStatusLabel.Content = "SWEEP INIT";
                        SFRAStatusLabel.Foreground = Brushes.Black;
                        break;
                    case SFRA_STATUS.SWEEPING:
                        SFRAStatusLabel.Content = "SWEEPING";
                        SFRAStatusLabel.Foreground = Brushes.Red;
                        break;
                    case SFRA_STATUS.SWEEP_DONE:
                        SFRAStatusLabel.Content = "SWEEP DONE";
                        SFRAStatusLabel.Foreground = Brushes.Black;
                        break;
                    case SFRA_STATUS.SFRA_DONE:
                        SFRAStatusLabel.Content = "SWEEP DONE";
                        SFRAStatusLabel.Foreground = Brushes.Blue;
                        break;
                    default:
                        SFRAStatusLabel.Content = "COMM ERROR";
                        SFRAStatusLabel.Foreground = Brushes.Red;
                        break;
                }
            });
            return;
        }

        void sfra_get_bode(float arg)
        {
            byte[] float_byte = BitConverter.GetBytes(arg);
            tx_buffer = new byte[] { 0xAA, 0x04, 0x01, 0xAF };
            mySerialPort.Write(tx_buffer, 0, 4);
            Safe_Read(ref rx_buffer, 0, 4);
            short payload_len = BitConverter.ToInt16(rx_buffer, 2);
            Console.WriteLine($"The payload length is {payload_len} bytes");
            Safe_Read(ref rx_buffer, 4, payload_len);
            int bode_length = payload_len - 1;
            int points = bode_length / 12;
            float[] freq_points = new float[points];
            float[] mag_points = new float[points];
            float[] pha_points = new float[points];
            for (int i = 0; i < points; i++)
            {
                freq_points[i] = BitConverter.ToSingle(rx_buffer, 4 + 4 * i);
                mag_points[i] = BitConverter.ToSingle(rx_buffer, 4 + 4 * points + 4 * i);
                pha_points[i] = BitConverter.ToSingle(rx_buffer, 4 + 8 * points + 4 * i);
            }
            float[] freq_log = freq_points.Select(f => MathF.Log10(f)).ToArray();

            Mag_Plot.Plot.Add.Scatter(freq_log, mag_points);
            Mag_Plot.Plot.XLabel("X Axis 1");
            Mag_Plot.Plot.YLabel("Y Axis 1 (x^2)");
            Mag_Plot.Plot.Title("Plot 1: Quadratic");
            Mag_Plot.Plot.Axes.AutoScale();

            Pha_Plot.Plot.Add.Scatter(freq_log, pha_points);
            Pha_Plot.Plot.XLabel("X Axis 2");
            Pha_Plot.Plot.YLabel("Y Axis 2 (1/x)");
            Pha_Plot.Plot.Title("Plot 2: Reciprocal");
            Pha_Plot.Plot.Axes.AutoScale();

            Mag_Plot.Refresh();
            Pha_Plot.Refresh();
            return;
        }

        void sfra_set_start_freq(float arg)
        {
            byte[] float_byte = BitConverter.GetBytes(arg);
            tx_buffer = new byte[] { 0xAA, 0x05, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00 };
            for (int i = 0; i < 4; i++)
            {
                tx_buffer[i + 3] = float_byte[i];
            }
            mySerialPort.Write(tx_buffer, 0, 8);
            Safe_Read(ref rx_buffer, 0, 6);
            return;
        }

        void sfra_set_step_freq(float arg)
        {
            byte[] float_byte = BitConverter.GetBytes(arg);
            tx_buffer = new byte[] { 0xAA, 0x06, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00 };
            for (int i = 0; i < 4; i++)
            {
                tx_buffer[i + 3] = float_byte[i];
            }
            mySerialPort.Write(tx_buffer, 0, 8);
            Safe_Read(ref rx_buffer, 0, 6);
            return;
        }

        void sfra_set_samp_freq(float arg)
        {
            byte[] float_byte = BitConverter.GetBytes(arg);
            tx_buffer = new byte[] { 0xAA, 0x07, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00 };
            for (int i = 0; i < 4; i++)
            {
                tx_buffer[i + 3] = float_byte[i];
            }
            mySerialPort.Write(tx_buffer, 0, 8);
            Safe_Read(ref rx_buffer, 0, 6);
            return;
        }

         void sfra_set_input_amp(float arg)
        {
            byte[] float_byte = BitConverter.GetBytes(arg);
            tx_buffer = new byte[] { 0xAA, 0x08, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00 };
            for (int i = 0; i < 4; i++)
            {
                tx_buffer[i + 3] = float_byte[i];
            }
            mySerialPort.Write(tx_buffer, 0, 8);
            Safe_Read(ref rx_buffer, 0, 6);
            return;
        }

        void Mag_MouseMove(object sender, MouseEventArgs e)
        {
            Pixel p = Mag_Plot.GetPlotPixelPosition(e);
            Coordinates cc = Mag_Plot.Plot.GetCoordinates(p.X, p.Y);
            Cursor_Label.Content = "(" + cc.X.ToString("F2") + ", " + cc.Y.ToString("F2") + ")";
            Mag_Plot.Refresh();
        }

        void Pha_MouseMove(object sender, MouseEventArgs e)
        {
            Pixel p = Pha_Plot.GetPlotPixelPosition(e);
            Coordinates cc = Pha_Plot.Plot.GetCoordinates(p.X, p.Y);
            Cursor_Label.Content = "(" + cc.X.ToString("F2") + ", " + cc.Y.ToString("F2") + ")";
            Pha_Plot.Refresh();
        }
    }
}