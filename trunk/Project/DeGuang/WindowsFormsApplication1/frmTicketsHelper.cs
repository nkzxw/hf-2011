using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Net;
using System.IO;
using System.Diagnostics;
using System.Security.Cryptography.X509Certificates;
using System.Net.Security;
using System.Security.Authentication;
using System.Runtime.InteropServices;
using System.Threading;
using System.Web;
using System.IO.Compression;

namespace DeGuangTicketsHelper
{
    /// <summary>
    /// 项目名称:德广火车票助手
    /// 公司:深圳市德广信息技术有限公司
    /// 作者:武广敬
    /// 此项目目前开源,所以请注意以下几点
    /// 请保留此版权信息.
    /// 自由再散布（Free Distribution）：允许获得源代码的人可自由再将此源代码散布。
    /// 源代码（Source Code）：程序的可执行文件在散布时，必需以随附完整源代码或是可让人方便的事后取得源代码。
    /// 派生著作（Derived Works）：让人可依此源代码修改后，在依照同一授权条款的情形下再散布。
    /// 原创作者程序源代码的完整性（Integrity of The Author’s Source Code）：意即修改后的版本，需以不同的版本号码以与原始的代码做分别，保障原始的代码完整性。
    /// 不得对任何人或团体有差别待遇（No Discrimination Against Persons or Groups）：开放源代码软件不得因性别、团体、国家、族群等设置限制，但若是因为法律规定的情形则为例外（如：美国政府限制高加密软件的出口）。
    /// 对程序在任何领域内的利用不得有差别待遇（No Discrimination Against Fields of Endeavor）：意即不得限制商业使用。
    /// 散布授权条款（Distribution of License）：若软件再散布，必需以同一条款散布之。
    /// 授权条款不得专属于特定产品（License Must Not Be Specific to a Product）：若多个程序组合成一套软件，则当某一开放源代码的程序单独散布时，也必需要符合开放源代码的条件。
    /// 授权条款不得限制其他软件（License Must Not Restrict Other Software）：当某一开放源代码软件与其他非开放源代码软件一起散布时（例如放在同一光盘），不得限制其他软件的授权条件也要遵照开放源代码的授权。
    /// 授权条款必须技术中立（License Must Be Technology-Neutral）：意即授权条款不得限制为电子格式才有效，若是纸本的授权条款也应视为有效。
    /// </summary>
    public partial class frmTicketsHelper : Form
    {


        [DllImport("wininet.dll", CharSet = CharSet.Auto, SetLastError = true)]
        public static extern bool InternetSetCookie(string lpszUrlName, string lbszCookieName, string lpszCookieData);

        /// <summary>
        /// 无参数方法委托
        /// </summary>
        public delegate void doWorkDelegate();
        /// <summary>
        /// UI显示消息委托
        /// </summary>
        /// <param name="msg"></param>
        public delegate void showMsgDelegate1(string msg);
        /// <summary>
        /// 得到焦点委托
        /// </summary>
        /// <returns></returns>
        public delegate void focusDelegate1(Control control);
        /// <summary>
        /// 修改控件文字委托
        /// </summary>
        /// <param name="con"></param>
        /// <param name="text"></param>
        /// <param name="enable"></param>
        public delegate void setControlTextDelegate1(Control con, string text,bool enable);

        public doWorkDelegate LoggedDelegate;
        public doWorkDelegate activateDelegate;
        public showMsgDelegate1 showMsgDelegate;
        public showMsgDelegate1 shareToWeiboDelegate;
        public setControlTextDelegate1 setControlTextDelegate;
        public focusDelegate1 focusDelegate;

        private static readonly string DefaultUserAgent = "Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.2; Trident/4.0; .NET CLR 1.1.4322; .NET4.0C; .NET4.0E; .NET CLR 2.0.50727; .NET CLR 3.0.04506.648; .NET CLR 3.5.21022; .NET CLR 3.0.4506.2152; .NET CLR 3.5.30729)";
        private static readonly string DefaultAccept = "image/gif, image/jpeg, image/pjpeg, image/pjpeg, application/xaml+xml, application/x-ms-xbap, application/x-ms-application, application/vnd.ms-excel, application/vnd.ms-powerpoint, application/msword, */*";
        private static readonly string DefaultContentType = "text/html; charset=GBK";
        /// <summary>
        /// cookie容器
        /// </summary>
        CookieContainer cookieContainer;
        /// <summary>
        /// cookie集合
        /// </summary>
        CookieCollection cookieCollection;
        /// <summary>
        /// 
        /// </summary>
        string cookieStr;
        /// <summary>
        /// 取得的HTML
        /// </summary>
        string html;
        /// <summary>
        /// 已经登录,当后台线程登录成功后,设为真
        /// </summary>
        private static bool logged = false;
        /// <summary>
        /// 正在运行中
        /// </summary>
        private static bool running = false;
        /// <summary>
        /// 手工停止
        /// </summary>
        private static bool stop = false;
        /// <summary>
        /// 尝试次数
        /// </summary>
        private static int count = 0;
        /// <summary>
        /// 便即登录时间
        /// </summary>
        private static DateTime beginTime;
        /// <summary>
        /// 登录成功时间
        /// </summary>
        private static DateTime endTime;
        /// <summary>
        /// 登录用时
        /// </summary>
        private TimeSpan timeSpan;
        /// <summary>
        /// 登录用时字符串
        /// </summary>
        private string timeSpanStr;
        /// <summary>
        /// 用于显示的时间与实际阻塞时间相同
        /// </summary>
        private int tryInterval;
        /// <summary>
        /// 德广火车票助手
        /// </summary>
        public frmTicketsHelper()
        {
            InitializeComponent();
            LoggedDelegate = new doWorkDelegate(openInWebBrowser);
            activateDelegate = new doWorkDelegate(Activate);
            showMsgDelegate = new showMsgDelegate1(showLogInfo);
            shareToWeiboDelegate = new showMsgDelegate1(shareToWeibo);
            setControlTextDelegate = new setControlTextDelegate1(changeControlTxt);
            focusDelegate = new focusDelegate1(setControlFocus);
            cookieContainer = new CookieContainer();
            cookieCollection = new CookieCollection();
            tssVersion.Text = "版本: " + System.Reflection.Assembly.GetExecutingAssembly().GetName().Version.ToString();
        }

        /// <summary>
        /// 得到验证码 外部调用
        /// </summary>
        /// <param name="obj"></param>
        private void getVerificationCode(object obj)
        {
            beginTime = DateTime.Now;
            System.Net.ServicePointManager.CertificatePolicy = new MyPolicy();
            //string url = "http://www.12306.cn/mormhweb/kyfw/";

            //HttpWebRequest request1 = HttpWebResponseUtility.CreateGetHttpResponse(url, cookieContainer) ;
            count = 0;
            this.Invoke(setControlTextDelegate, new object[] { btnLogin,"取得验证码中...",false});
            picValidImg.Image = null;
            getVerificationCode();
        }

        /// <summary>
        /// 得到验证码 内部重复调用 直到得到验证码
        /// </summary>
        private void getVerificationCode()
        {
            sleep();
            count++;
            string url = "https://dynamic.12306.cn/otsweb/passCodeAction.do?rand=lrand";
            HttpWebRequest request2 = HttpWebResponseUtility.CreateGetHttpResponse(url, cookieContainer, "https://dynamic.12306.cn/otsweb/loginAction.do?method=login");
            HttpWebResponse response = null;
            try
            {
                response = (HttpWebResponse)request2.GetResponse();
            }
            catch (Exception ex)
            {
                //MessageBox.Show("连接12306.cn网站出错!","异常", MessageBoxButtons.OK, MessageBoxIcon.Error, MessageBoxDefaultButton.Button1);
                this.Invoke(this.showMsgDelegate, "取得验证码失败:" + ex.Message);
                getVerificationCode();
            }
            if (response != null)
            {
                Stream responseStream = response.GetResponseStream();

                response.Cookies = request2.CookieContainer.GetCookies(new Uri(url));

                cookieCollection = response.Cookies;

                if (string.IsNullOrEmpty(cookieStr) == true)
                {
                    cookieStr = response.Headers.Get("Set-Cookie");
                }
                cookieContainer.SetCookies(new Uri(url), cookieStr);

                Image original = Image.FromStream(responseStream);

                picValidImg.Image = original;

                this.Invoke(this.showMsgDelegate, "取得验证码成功!");

                this.Invoke(setControlTextDelegate, new object[] { btnLogin, "登录" ,true});

                endTime = DateTime.Now;

                timeSpan = endTime.Subtract(beginTime);

                if (count > 1)
                {
                    MessageBox.Show("12306不给力啊!!!尝试了" + count + "次用了" + getTimeSpanString(timeSpan) + "后才得到验证码图片.", "德广火车票助手 温馨提示", MessageBoxButtons.OK, MessageBoxIcon.Information);
                    this.Invoke(activateDelegate);
                    this.Invoke(shareToWeiboDelegate, new object[] { "12306不给力啊,登录页面尝试了"+count+"次用了"+getTimeSpanString(timeSpan)+"后,才显示出来.还好有#德广火车票助手#帮助,不然就鼠标键盘就被我按报废啦!" });
                }
            }
        }

        /// <summary>
        /// 线程阻塞,重试间隔
        /// </summary>
        private void sleep()
        {
            if (count > 0)
            {
                tryInterval = TryInterval;
                if (tryInterval > 0)
                {
                    this.Invoke(new showMsgDelegate1(showTimeInfo), "休息" + tryInterval + "秒");
                }
                Thread.Sleep(TryInterval * 1000);
            }
        }

        /// <summary>
        /// 登录
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void btnLogin_Click(object sender, EventArgs e)
        {
            if (logged == true)
            {
                if (MessageBox.Show("您已经登录,您需要再次进入12306网站吗?需要您已经退出,就需要重新登录!", "德广火车票助手 温馨提示", MessageBoxButtons.YesNo, MessageBoxIcon.Information, MessageBoxDefaultButton.Button2) == DialogResult.Yes)
                {
                    this.Invoke(LoggedDelegate);
                    //openie();
                }
                return;
            }
            if (running == false)
            {
                //MessageBox.Show("目前是单线程版本,所以程序可能没有反应,此为正常现象,待登录成功后,即会自动弹出IE浏览器.");
                if (picValidImg.Image == null)
                {
                    getVerificationCode(this);
                }
                else
                {
                    if (validate() == true)
                    {
                        stop = false;
                        count = 0;
                        btnLogin.Text = "尝试登录中...";
                        beginTime = DateTime.Now;
                        ThreadPool.QueueUserWorkItem(new WaitCallback(login));
                    }
                }
            }
            else
            {
                //MessageBox.Show("目前正在登录中!","德广火车票助手 温馨提示", MessageBoxButtons.OK, MessageBoxIcon.Information, MessageBoxDefaultButton.Button1);
                if (MessageBox.Show("目前正在尝试登录中,中止登录吗?", "德广火车票助手 温馨提示", MessageBoxButtons.YesNo, MessageBoxIcon.Information, MessageBoxDefaultButton.Button2) == DialogResult.Yes)
                {
                    stop = true;
                }
            }
        }

        /// <summary>
        /// 验证数据
        /// </summary>
        /// <returns></returns>
        private bool validate()
        {
            if (string.IsNullOrEmpty(txtUserName.Text) == true)
            {
                messageBoxShowInfo("请输入登录名!");
                txtUserName.Focus();
                return false;
            }
            else if (string.IsNullOrEmpty(txtPassword.Text) == true)
            {
                messageBoxShowInfo("请输入密码!");
                txtPassword.Focus();
                return false;
            }
            return true;
        }

        /// <summary>
        /// 时间间隔
        /// </summary>
        private int TryInterval
        {
            get
            {
                if (chkRadom.Checked == true)
                {
                    return new Random((int)DateTime.Now.Ticks).Next(Convert.ToInt32(numInterval.Value));
                }
                else
                {
                    return Convert.ToInt32(numInterval.Value);
                }
            }
        }

        /// <summary>
        /// 登录
        /// </summary>
        private void login()
        {
            login(null);
        }
        /// <summary>
        /// 登录
        /// </summary>
        /// <param name="obj"></param>
        private void login(object obj)
        {
            try
            {
                running = true;
                sleep();
                if (stop == true)
                {
                    return;
                }
                if (obj != null)
                {
                    Thread.CurrentThread.Name = obj.ToString();
                }
                if (logged == true)
                {
                    if (MessageBox.Show("您已经登录,您需要再次进入12306网站吗?需要您已经退出,就需要重新登录!", "德广火车票助手 温馨提示", MessageBoxButtons.YesNo, MessageBoxIcon.Information, MessageBoxDefaultButton.Button2) == DialogResult.Yes)
                    {
                        this.Invoke(LoggedDelegate);
                        //openie();
                    }
                    running = false;
                    return;
                }

                Trace.WriteLine("login()");
                count++;
                System.Net.ServicePointManager.CertificatePolicy = new MyPolicy();

                // this is what we are sending
                string post_data = "loginUser.user_name=tony12306cn&nameErrorFocus=&user.password=tony1234&passwordErrorFocus=&randCode=" + txtVerificationCode.Text + "&randErrorFocus=focus";

                // this is where we will send it
                string uri = "https://dynamic.12306.cn/otsweb/loginAction.do?method=login";

                // create a request
                //HttpWebRequest request = (HttpWebRequest)WebRequest.Create(uri);
                //request.CookieContainer = cookieContainer;
                //request.KeepAlive = false;
                //request.ProtocolVersion = HttpVersion.Version10;
                //request.Method = "POST";

                //// turn our request string into a byte stream
                //byte[] postBytes = Encoding.ASCII.GetBytes(post_data);

                //// this is important - make sure you specify type this way
                //request.ContentType = "application/x-www-form-urlencoded";
                //request.ContentLength = postBytes.Length;
                //Stream requestStream = request.GetRequestStream();

                //// now send it
                //requestStream.Write(postBytes, 0, postBytes.Length);
                //requestStream.Close();

                Dictionary<string, string> param = new Dictionary<string, string>();
                param.Add("loginUser.user_name", txtUserName.Text);
                param.Add("nameErrorFocus", string.Empty);
                param.Add("user.password", txtPassword.Text);
                param.Add("passwordErrorFocus", string.Empty);
                param.Add("randCode", txtVerificationCode.Text);
                param.Add("randErrorFocus", "focus");

                HttpWebResponse response = null;
                try
                {
                    response = HttpWebResponseUtility.CreatePostHttpResponse(uri, param,null, DefaultUserAgent, Encoding.ASCII, cookieCollection, uri);
                }
                catch (Exception ex)
                {
                    this.Invoke(this.showMsgDelegate, ex.Message);
                    //showInfo(ex.Message);
                }

                if (response != null)
                {
                    // grab te response and print it out to the console along with the status code
                    //HttpWebResponse response = (HttpWebResponse)request.GetResponse();
                    Stream receiveStream = response.GetResponseStream();
                    if (response.ContentEncoding.ToLower().Contains("gzip"))
                    {
                        receiveStream = new GZipStream(receiveStream, CompressionMode.Decompress);
                    }
                    html = new StreamReader(receiveStream).ReadToEnd();
                    if (html.IndexOf("当前访问用户过多") > 0)
                    {
                        this.Invoke(this.showMsgDelegate, "当前访问用户过多");
                        login(null);
                    }
                    else if (html.IndexOf("请输入正确的验证码") > 0)
                    {
                        messageBoxShowInfo("请输入正确的验证码!");
                        this.Invoke(focusDelegate,new object[]{txtVerificationCode});
                        this.Invoke(setControlTextDelegate,new object[]{txtVerificationCode,string.Empty,true});
                        getVerificationCode(this);
                    }
                    else if (html.IndexOf("登录名不存在") > 0)
                    {
                        messageBoxShowInfo("登录名不存在!!");
                        this.Invoke(focusDelegate, new object[] { txtUserName });
                    }
                    else if (html.IndexOf("密码输入错误") > 0)
                    {
                        messageBoxShowInfo("密码输入错误,如果多次输入错误可能会被锁定帐户!");
                        this.Invoke(focusDelegate, new object[] { txtPassword });
                        this.Invoke(setControlTextDelegate, new object[] { txtPassword, string.Empty, true });
                    }
                    else if (html.IndexOf("已经被锁定") > 0)
                    {
                        messageBoxShowInfo("您的用户已经被锁定,请稍候再试!");
                    }
                    else if (html.IndexOf("系统维护中") > 0)
                    {
                        messageBoxShowInfo("系统维护中!");
                    }
                    else if (html.IndexOf("我的12306") > 0)
                    {
                        this.Invoke(activateDelegate);
                        endTime = DateTime.Now;
                        logged = true;
                        timeSpan = endTime.Subtract(beginTime);
                        timeSpanStr = getTimeSpanString(timeSpan);

                        MessageBox.Show("经过 " + timeSpanStr + ", " + count + " 次的尝试后,您已经登录成功!" + Environment.NewLine
                            + "点击确定打开12306网站,请忽略登录界面,直接点击\"车票预订\"就可以啦!" + Environment.NewLine
                            + Environment.NewLine
                            + "深圳市德广信息技术有限公司 祝您:" + Environment.NewLine
                            + "回家一路顺风!全家身体健康!幸福快乐!事事如意!", "德广火车票助手 恭喜您", MessageBoxButtons.OK, MessageBoxIcon.Information, MessageBoxDefaultButton.Button1);

                        this.Invoke(shareToWeiboDelegate, new object[] { "我用#德广火车票助手#经过" + timeSpanStr +"尝试登录"+ count + "次后,成功登录上了12306.cn!你用了多长时间才登录成功的呢?" });

                        this.Invoke(LoggedDelegate);
                        //openie();
                        this.Invoke(this.showMsgDelegate, "登录成功!");
                    }
                    else
                    {
                        Trace.WriteLine(html);
                        login(null);
                    }
                    
                    Trace.WriteLine(response.StatusCode);
                }
                else
                {
                    login(null);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "异常", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            finally
            {
                this.Invoke(setControlTextDelegate, new object[] { btnLogin, "登录", true });
                running = false;
            }
        }

        /// <summary>
        /// 得到时间间隔的中文字符串
        /// </summary>
        private string getTimeSpanString(TimeSpan timeSpan)
        {
            StringBuilder timeSpanStr=new StringBuilder();

            if (timeSpan.Days > 0)
            {
                timeSpanStr.Append(timeSpan.Days);
                timeSpanStr.Append("天");
            }
            if (timeSpan.Hours > 0)
            {
                timeSpanStr.Append(timeSpan.Hours);
                timeSpanStr.Append("小时");
            }
            if (timeSpan.Minutes > 0)
            {
                timeSpanStr.Append(timeSpan.Minutes);
                timeSpanStr.Append("分");
            }
            if (timeSpan.Seconds > 0)
            {
                timeSpanStr.Append(timeSpan.Seconds);
                timeSpanStr.Append("秒");
            }
            if (timeSpanStr.Length == 0)
            {
                timeSpanStr.Append(timeSpan.Milliseconds);
                timeSpanStr.Append("毫秒");
            }
            return timeSpanStr.ToString();
        }

        /// <summary>
        /// 显示信息
        /// </summary>
        /// <param name="message"></param>
        private void messageBoxShowInfo(string message)
        {
            this.Invoke(this.showMsgDelegate, message);
            MessageBox.Show(message, "德广火车票助手 温馨提示", MessageBoxButtons.OK, MessageBoxIcon.Information, MessageBoxDefaultButton.Button1);
        }

        /// <summary>
        /// 打开浏览器
        /// </summary>
        private void openInWebBrowser()
        {
            foreach (Cookie cookie in cookieContainer.GetCookies(new Uri("https://dynamic.12306.cn/otsweb/loginAction.do?method=login")))
            {
                InternetSetCookie(
                    "https://" + cookie.Domain.ToString(),
                    cookie.Name.ToString(),
                    cookie.Value.ToString() + ";expires=Sun,22-Feb-2099 00:00:00 GMT");
            }
            string url="https://dynamic.12306.cn/otsweb/";

            if (radTickerWebBrowser.Checked == true)
            {
                TickerWebBrowser tw = new TickerWebBrowser();
                tw.Url = url;
                tw.ShowDialog();
            }
            else if (radIE.Checked == true)
            {
                System.Diagnostics.Process.Start("IExplore.exe", url); 
            }
            else if (radDefaultWebBrowser.Checked == true)
            {
                try
                {
                    System.Diagnostics.Process.Start(url);
                }
                catch(System.ComponentModel.Win32Exception noBrowser)
                {
                    if (noBrowser.ErrorCode == -2147467259)
                    {
                        MessageBox.Show("未找到您的默认Web浏览器!", "错误", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                        radTickerWebBrowser.Checked = true;
                    }
                }
                catch (System.Exception other)
                {
                    MessageBox.Show(other.Message, "错误", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                }
            }
        }

        /// <summary>
        /// 显示信息
        /// </summary>
        /// <param name="info"></param>
        private void showLogInfo(string info)
        {
            info = "第"+count+"次尝试:" + info;
            showTimeInfo(info);
        }

        /// <summary>
        /// 显示信息-加时间
        /// </summary>
        /// <param name="info"></param>
        private void showTimeInfo(string info)
        {
            info = DateTime.Now.ToString("HH:mm:ss ") + info;
            showInfo(info);
        }

        /// <summary>
        /// 显示信息
        /// </summary>
        /// <param name="info"></param>
        private void showInfo(string info)
        {
            if (lstMsg.Items.Count > 100)
            {
                lstMsg.Items.RemoveAt(100);
            }
            lstMsg.Items.Insert(0, info);
        }

        /// <summary>
        /// 改变控件文字
        /// </summary>
        /// <param name="con">控件</param>
        /// <param name="text">文本</param>
        /// <param name="enable">控件是否可用</param>
        private void changeControlTxt(Control con, string text,bool enable)
        {
            if (con != null && string.IsNullOrEmpty(text)==false)
            {
                con.Text = text;
                con.Enabled = enable;
            }
        }

        /// <summary>
        /// 使控件得到焦点
        /// </summary>
        /// <param name="control"></param>
        private void setControlFocus(Control control)
        {
            if(control!=null)
            {
                control.Focus();
            }
        }

        /// <summary>
        /// 加载
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void frmTicketsHelper_Load(object sender, EventArgs e)
        {
            ThreadPool.QueueUserWorkItem(new WaitCallback(getVerificationCode), new object[] { this });
            this.Invoke(shareToWeiboDelegate, new object[] { "我正在使用#德广火车票助手#抢火车票!亲们祝我好运噢!" });
            chkRadom.Checked = true;
        }

        /// <summary>
        /// 分享至微博
        /// </summary>
        /// <param name="message"></param>
        private void shareToWeibo(string message)
        {
            #region 使用Web代码
            this.webBrowser1.Navigate("http://www.9inf.com/TicketsHelper1.03.php?title=" + HttpUtility.UrlEncode(message) + "&url="+HttpUtility.UrlEncode("http://www.9inf.com/content/%E5%BE%B7%E5%B9%BF%E7%81%AB%E8%BD%A6%E7%A5%A8%E5%8A%A9%E6%89%8B"));
            #endregion

            #region 使用本地代码
            //StringBuilder sb = new StringBuilder();
            //sb.Append("<script type=\"text/javascript\" charset=\"utf-8\">");
            //sb.Append(Environment.NewLine);
            //sb.Append("(function(){");
            //sb.Append(Environment.NewLine);
            //sb.Append("  var _w = 142 , _h = 32;");
            //sb.Append(Environment.NewLine);
            //sb.Append("  var param = {");
            //sb.Append(Environment.NewLine);
            //sb.Append("    url:location.href,");
            //sb.Append(Environment.NewLine);
            //sb.Append("    type:'4',");
            //sb.Append(Environment.NewLine);
            //sb.Append("    count:'', /**是否显示分享数，1显示(可选)*/");
            //sb.Append(Environment.NewLine);
            //sb.Append("    appkey:'1049229778', /**您申请的应用appkey,显示分享来源(可选)*/");
            //sb.Append(Environment.NewLine);
            //sb.Append("    title:'"+message+"', /**分享的文字内容(可选，默认为所在页面的title)*/");
            //sb.Append(Environment.NewLine);
            //sb.Append("    pic:'', /**分享图片的路径(可选)*/");
            //sb.Append(Environment.NewLine);
            //sb.Append("    ralateUid:'2244896670', /**关联用户的UID，分享微博会@该用户(可选)*/");
            //sb.Append(Environment.NewLine);
            //sb.Append("	language:'zh_cn', /**设置语言，zh_cn|zh_tw(可选)*/");
            //sb.Append(Environment.NewLine);
            //sb.Append("    rnd:new Date().valueOf()");
            //sb.Append(Environment.NewLine);
            //sb.Append("  }");
            //sb.Append(Environment.NewLine);
            //sb.Append("  var temp = [];");
            //sb.Append(Environment.NewLine);
            //sb.Append("  for( var p in param ){");
            //sb.Append(Environment.NewLine);
            //sb.Append("    temp.push(p + '=' + encodeURIComponent( param[p] || '' ) )");
            //sb.Append(Environment.NewLine);
            //sb.Append("  }");
            //sb.Append(Environment.NewLine);
            //sb.Append("  document.write('<iframe allowTransparency=\"true\" frameborder=\"0\" scrolling=\"no\" src=\"http://hits.sinajs.cn/A1/weiboshare.html?' + temp.join('&') + '\" width=\"'+ _w+'\" height=\"'+_h+'\"></iframe>')");
            //sb.Append(Environment.NewLine);
            //sb.Append("})()");
            //sb.Append(Environment.NewLine);
            //sb.Append("</script>");
            //sb.Append(Environment.NewLine);
            //DisplayHtml(sb.ToString());
            #endregion
        }

        /// <summary>
        /// 显示html内容至WebBrowser
        /// </summary>
        /// <param name="html"></param>
        private void DisplayHtml(string html)
        {
            webBrowser1.Navigate("about:blank");
            if (webBrowser1.Document != null)
            {
                webBrowser1.Document.Write(string.Empty);
            }
            webBrowser1.DocumentText = html;
        }

        /// <summary>
        /// 退出
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void btnExit_Click(object sender, EventArgs e)
        {
            if (MessageBox.Show("您确定要退出德广火车票助手吗?", "德广火车票助手友情 温馨提示", MessageBoxButtons.YesNo, MessageBoxIcon.Information, MessageBoxDefaultButton.Button2) == DialogResult.Yes)
            {
                this.Close();
            }
        }

        /// <summary>
        /// 点击得到新的验证码
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void picValidImg_Click(object sender, EventArgs e)
        {
            ThreadPool.QueueUserWorkItem(new WaitCallback(getVerificationCode), new object[] { this });
        }

        /// <summary>
        /// 点击公司
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void tssAuthor_Click(object sender, EventArgs e)
        {
            System.Diagnostics.Process.Start("http://www.9inf.com");
        }

        private void chkRadom_CheckedChanged(object sender, EventArgs e)
        {
            if (chkRadom.Checked == true)
            {
                labInterval.Text = "最大尝试间隔";
            }
            else
            {
                labInterval.Text = "尝试间隔";
            }
        }

        private void numInterval_Validating(object sender, CancelEventArgs e)
        {
            if (numInterval.Value < 5)
            {
                MessageBox.Show("如果尝试间隔小于5秒,有可能会被12306封锁,欲速则不达!", "德广火车票助手友情 温馨提示", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
        }
    }

    /// <summary>
    /// 处理证书提示
    /// </summary>
    public class MyPolicy : ICertificatePolicy
    {
        public bool CheckValidationResult(ServicePoint srvPoint,
          X509Certificate certificate, WebRequest request,
          int certificateProblem)
        {
            //Return True to force the certificate to be accepted.
            return true;
        }
    }
}
