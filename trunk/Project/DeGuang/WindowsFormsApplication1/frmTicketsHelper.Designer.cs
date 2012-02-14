namespace DeGuangTicketsHelper
{
    partial class frmTicketsHelper
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmTicketsHelper));
            this.toolTip1 = new System.Windows.Forms.ToolTip(this.components);
            this.webBrowser1 = new System.Windows.Forms.WebBrowser();
            this.radDefaultWebBrowser = new System.Windows.Forms.RadioButton();
            this.radIE = new System.Windows.Forms.RadioButton();
            this.radTickerWebBrowser = new System.Windows.Forms.RadioButton();
            this.lstMsg = new System.Windows.Forms.ListBox();
            this.txtPassword = new System.Windows.Forms.TextBox();
            this.txtUserName = new System.Windows.Forms.TextBox();
            this.picValidImg = new System.Windows.Forms.PictureBox();
            this.txtVerificationCode = new System.Windows.Forms.TextBox();
            this.statusStrip1 = new System.Windows.Forms.StatusStrip();
            this.tssVersion = new System.Windows.Forms.ToolStripStatusLabel();
            this.tssAuthor = new System.Windows.Forms.ToolStripStatusLabel();
            this.btnExit = new System.Windows.Forms.Button();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.label3 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.btnLogin = new System.Windows.Forms.Button();
            this.numInterval = new System.Windows.Forms.NumericUpDown();
            this.label4 = new System.Windows.Forms.Label();
            this.labInterval = new System.Windows.Forms.Label();
            this.chkRadom = new System.Windows.Forms.CheckBox();
            ((System.ComponentModel.ISupportInitialize)(this.picValidImg)).BeginInit();
            this.statusStrip1.SuspendLayout();
            this.groupBox1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.numInterval)).BeginInit();
            this.SuspendLayout();
            // 
            // webBrowser1
            // 
            this.webBrowser1.AllowWebBrowserDrop = false;
            this.webBrowser1.Location = new System.Drawing.Point(0, 0);
            this.webBrowser1.Margin = new System.Windows.Forms.Padding(0);
            this.webBrowser1.MinimumSize = new System.Drawing.Size(20, 20);
            this.webBrowser1.Name = "webBrowser1";
            this.webBrowser1.ScrollBarsEnabled = false;
            this.webBrowser1.Size = new System.Drawing.Size(348, 86);
            this.webBrowser1.TabIndex = 36;
            this.toolTip1.SetToolTip(this.webBrowser1, "把您的使用心得分享给您的朋友吧!");
            this.webBrowser1.WebBrowserShortcutsEnabled = false;
            // 
            // radDefaultWebBrowser
            // 
            this.radDefaultWebBrowser.AutoSize = true;
            this.radDefaultWebBrowser.Enabled = false;
            this.radDefaultWebBrowser.Location = new System.Drawing.Point(6, 61);
            this.radDefaultWebBrowser.Name = "radDefaultWebBrowser";
            this.radDefaultWebBrowser.Size = new System.Drawing.Size(107, 16);
            this.radDefaultWebBrowser.TabIndex = 0;
            this.radDefaultWebBrowser.Tag = "";
            this.radDefaultWebBrowser.Text = "系统默认浏览器";
            this.toolTip1.SetToolTip(this.radDefaultWebBrowser, "[暂不支持]您设定的系统默认Web浏览器");
            this.radDefaultWebBrowser.UseVisualStyleBackColor = true;
            // 
            // radIE
            // 
            this.radIE.AutoSize = true;
            this.radIE.Location = new System.Drawing.Point(6, 39);
            this.radIE.Name = "radIE";
            this.radIE.Size = new System.Drawing.Size(71, 16);
            this.radIE.TabIndex = 0;
            this.radIE.Tag = "";
            this.radIE.Text = "IE浏览器";
            this.toolTip1.SetToolTip(this.radIE, "微软Internet Explorer浏览器");
            this.radIE.UseVisualStyleBackColor = true;
            // 
            // radTickerWebBrowser
            // 
            this.radTickerWebBrowser.AutoSize = true;
            this.radTickerWebBrowser.Checked = true;
            this.radTickerWebBrowser.Location = new System.Drawing.Point(6, 17);
            this.radTickerWebBrowser.Name = "radTickerWebBrowser";
            this.radTickerWebBrowser.Size = new System.Drawing.Size(119, 16);
            this.radTickerWebBrowser.TabIndex = 0;
            this.radTickerWebBrowser.TabStop = true;
            this.radTickerWebBrowser.Tag = "";
            this.radTickerWebBrowser.Text = "内置浏览器(推荐)";
            this.toolTip1.SetToolTip(this.radTickerWebBrowser, "德广火车票助手内置的浏览器");
            this.radTickerWebBrowser.UseVisualStyleBackColor = true;
            // 
            // lstMsg
            // 
            this.lstMsg.FormattingEnabled = true;
            this.lstMsg.HorizontalScrollbar = true;
            this.lstMsg.ItemHeight = 12;
            this.lstMsg.Location = new System.Drawing.Point(12, 234);
            this.lstMsg.Name = "lstMsg";
            this.lstMsg.Size = new System.Drawing.Size(324, 124);
            this.lstMsg.TabIndex = 42;
            this.toolTip1.SetToolTip(this.lstMsg, "执行结果");
            // 
            // txtPassword
            // 
            this.txtPassword.Font = new System.Drawing.Font("宋体", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(134)));
            this.txtPassword.Location = new System.Drawing.Point(73, 128);
            this.txtPassword.Name = "txtPassword";
            this.txtPassword.PasswordChar = '*';
            this.txtPassword.Size = new System.Drawing.Size(126, 26);
            this.txtPassword.TabIndex = 44;
            this.toolTip1.SetToolTip(this.txtPassword, "输入您在12306的密码");
            // 
            // txtUserName
            // 
            this.txtUserName.Font = new System.Drawing.Font("宋体", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(134)));
            this.txtUserName.Location = new System.Drawing.Point(73, 96);
            this.txtUserName.Name = "txtUserName";
            this.txtUserName.Size = new System.Drawing.Size(126, 26);
            this.txtUserName.TabIndex = 43;
            this.toolTip1.SetToolTip(this.txtUserName, "输入您在12306的登录名");
            // 
            // picValidImg
            // 
            this.picValidImg.Location = new System.Drawing.Point(121, 160);
            this.picValidImg.Name = "picValidImg";
            this.picValidImg.Size = new System.Drawing.Size(78, 26);
            this.picValidImg.TabIndex = 38;
            this.picValidImg.TabStop = false;
            this.toolTip1.SetToolTip(this.picValidImg, "看不清?点击更换一个验证码");
            this.picValidImg.Click += new System.EventHandler(this.picValidImg_Click);
            // 
            // txtVerificationCode
            // 
            this.txtVerificationCode.Font = new System.Drawing.Font("宋体", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(134)));
            this.txtVerificationCode.Location = new System.Drawing.Point(73, 160);
            this.txtVerificationCode.MaxLength = 4;
            this.txtVerificationCode.Name = "txtVerificationCode";
            this.txtVerificationCode.Size = new System.Drawing.Size(43, 26);
            this.txtVerificationCode.TabIndex = 45;
            this.toolTip1.SetToolTip(this.txtVerificationCode, "输入右图中的验证码");
            // 
            // statusStrip1
            // 
            this.statusStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.tssVersion,
            this.tssAuthor});
            this.statusStrip1.Location = new System.Drawing.Point(0, 400);
            this.statusStrip1.Name = "statusStrip1";
            this.statusStrip1.Size = new System.Drawing.Size(348, 22);
            this.statusStrip1.TabIndex = 35;
            this.statusStrip1.Text = "statusStrip1";
            // 
            // tssVersion
            // 
            this.tssVersion.Name = "tssVersion";
            this.tssVersion.Size = new System.Drawing.Size(35, 17);
            this.tssVersion.Text = "版本:";
            // 
            // tssAuthor
            // 
            this.tssAuthor.ForeColor = System.Drawing.Color.Blue;
            this.tssAuthor.Name = "tssAuthor";
            this.tssAuthor.Size = new System.Drawing.Size(298, 17);
            this.tssAuthor.Spring = true;
            this.tssAuthor.Text = "深圳市德广信息技术有限公司";
            this.tssAuthor.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.tssAuthor.Click += new System.EventHandler(this.tssAuthor_Click);
            // 
            // btnExit
            // 
            this.btnExit.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnExit.Font = new System.Drawing.Font("宋体", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(134)));
            this.btnExit.Location = new System.Drawing.Point(179, 364);
            this.btnExit.Name = "btnExit";
            this.btnExit.Size = new System.Drawing.Size(159, 33);
            this.btnExit.TabIndex = 47;
            this.btnExit.Text = "退出(&E)";
            this.btnExit.UseVisualStyleBackColor = true;
            this.btnExit.Click += new System.EventHandler(this.btnExit_Click);
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.radDefaultWebBrowser);
            this.groupBox1.Controls.Add(this.radIE);
            this.groupBox1.Controls.Add(this.radTickerWebBrowser);
            this.groupBox1.Location = new System.Drawing.Point(205, 96);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(133, 86);
            this.groupBox1.TabIndex = 46;
            this.groupBox1.TabStop = false;
            this.groupBox1.Tag = "设置登录成功后打开12306的浏览器";
            this.groupBox1.Text = "登录后使用的浏览器";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Font = new System.Drawing.Font("宋体", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(134)));
            this.label3.Location = new System.Drawing.Point(11, 163);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(56, 16);
            this.label3.TabIndex = 41;
            this.label3.Text = "验证码";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Font = new System.Drawing.Font("宋体", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(134)));
            this.label2.Location = new System.Drawing.Point(11, 131);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(40, 16);
            this.label2.TabIndex = 40;
            this.label2.Text = "密码";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Font = new System.Drawing.Font("宋体", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(134)));
            this.label1.Location = new System.Drawing.Point(11, 101);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(56, 16);
            this.label1.TabIndex = 39;
            this.label1.Text = "登录名";
            // 
            // btnLogin
            // 
            this.btnLogin.Font = new System.Drawing.Font("宋体", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(134)));
            this.btnLogin.Location = new System.Drawing.Point(12, 364);
            this.btnLogin.Name = "btnLogin";
            this.btnLogin.Size = new System.Drawing.Size(159, 33);
            this.btnLogin.TabIndex = 37;
            this.btnLogin.Text = "登录 ";
            this.btnLogin.UseVisualStyleBackColor = true;
            this.btnLogin.Click += new System.EventHandler(this.btnLogin_Click);
            // 
            // numInterval
            // 
            this.numInterval.Font = new System.Drawing.Font("宋体", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(134)));
            this.numInterval.Location = new System.Drawing.Point(120, 192);
            this.numInterval.Name = "numInterval";
            this.numInterval.Size = new System.Drawing.Size(77, 26);
            this.numInterval.TabIndex = 48;
            this.numInterval.Value = new decimal(new int[] {
            5,
            0,
            0,
            0});
            this.numInterval.Validating += new System.ComponentModel.CancelEventHandler(this.numInterval_Validating);
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Font = new System.Drawing.Font("宋体", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(134)));
            this.label4.Location = new System.Drawing.Point(208, 197);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(24, 16);
            this.label4.TabIndex = 49;
            this.label4.Text = "秒";
            // 
            // labInterval
            // 
            this.labInterval.AutoSize = true;
            this.labInterval.Font = new System.Drawing.Font("宋体", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(134)));
            this.labInterval.Location = new System.Drawing.Point(11, 197);
            this.labInterval.Name = "labInterval";
            this.labInterval.Size = new System.Drawing.Size(72, 16);
            this.labInterval.TabIndex = 41;
            this.labInterval.Text = "重试间隔";
            // 
            // chkRadom
            // 
            this.chkRadom.AutoSize = true;
            this.chkRadom.Font = new System.Drawing.Font("宋体", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(134)));
            this.chkRadom.Location = new System.Drawing.Point(242, 197);
            this.chkRadom.Name = "chkRadom";
            this.chkRadom.Size = new System.Drawing.Size(91, 20);
            this.chkRadom.TabIndex = 50;
            this.chkRadom.Text = "随机间隔";
            this.chkRadom.UseVisualStyleBackColor = true;
            this.chkRadom.CheckedChanged += new System.EventHandler(this.chkRadom_CheckedChanged);
            // 
            // frmTicketsHelper
            // 
            this.AcceptButton = this.btnLogin;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 12F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.btnExit;
            this.ClientSize = new System.Drawing.Size(348, 422);
            this.Controls.Add(this.chkRadom);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.numInterval);
            this.Controls.Add(this.btnExit);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.lstMsg);
            this.Controls.Add(this.labInterval);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.txtPassword);
            this.Controls.Add(this.txtUserName);
            this.Controls.Add(this.picValidImg);
            this.Controls.Add(this.btnLogin);
            this.Controls.Add(this.txtVerificationCode);
            this.Controls.Add(this.webBrowser1);
            this.Controls.Add(this.statusStrip1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.Name = "frmTicketsHelper";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "德广火车票助手";
            this.Load += new System.EventHandler(this.frmTicketsHelper_Load);
            ((System.ComponentModel.ISupportInitialize)(this.picValidImg)).EndInit();
            this.statusStrip1.ResumeLayout(false);
            this.statusStrip1.PerformLayout();
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.numInterval)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.ToolTip toolTip1;
        private System.Windows.Forms.StatusStrip statusStrip1;
        private System.Windows.Forms.ToolStripStatusLabel tssAuthor;
        private System.Windows.Forms.ToolStripStatusLabel tssVersion;
        private System.Windows.Forms.WebBrowser webBrowser1;
        private System.Windows.Forms.Button btnExit;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.RadioButton radDefaultWebBrowser;
        private System.Windows.Forms.RadioButton radIE;
        private System.Windows.Forms.RadioButton radTickerWebBrowser;
        private System.Windows.Forms.ListBox lstMsg;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.TextBox txtPassword;
        private System.Windows.Forms.TextBox txtUserName;
        private System.Windows.Forms.PictureBox picValidImg;
        private System.Windows.Forms.Button btnLogin;
        private System.Windows.Forms.TextBox txtVerificationCode;
        private System.Windows.Forms.NumericUpDown numInterval;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.Label labInterval;
        private System.Windows.Forms.CheckBox chkRadom;
    }
}