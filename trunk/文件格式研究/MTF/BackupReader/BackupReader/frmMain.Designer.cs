namespace BackupReader
{
    partial class frmMain
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmMain));
            this.ofdBackup = new System.Windows.Forms.OpenFileDialog();
            this.toolStripContainer1 = new System.Windows.Forms.ToolStripContainer();
            this.statusStrip1 = new System.Windows.Forms.StatusStrip();
            this.tsStatus = new System.Windows.Forms.ToolStripStatusLabel();
            this.tvDirs = new System.Windows.Forms.TreeView();
            this.ImageList1 = new System.Windows.Forms.ImageList(this.components);
            this.toolStrip1 = new System.Windows.Forms.ToolStrip();
            this.openToolStripButton = new System.Windows.Forms.ToolStripButton();
            this.extractToolStripButton = new System.Windows.Forms.ToolStripButton();
            this.cancelToolStripButton = new System.Windows.Forms.ToolStripButton();
            this.fbdBackup = new System.Windows.Forms.FolderBrowserDialog();
            this.toolStripButton1 = new System.Windows.Forms.ToolStripSeparator();
            this.opencatalogToolStripButton = new System.Windows.Forms.ToolStripButton();
            this.savecatalogToolStripButton = new System.Windows.Forms.ToolStripButton();
            this.ofdCatalog = new System.Windows.Forms.OpenFileDialog();
            this.sfdCatalog = new System.Windows.Forms.SaveFileDialog();
            this.toolStripContainer1.BottomToolStripPanel.SuspendLayout();
            this.toolStripContainer1.ContentPanel.SuspendLayout();
            this.toolStripContainer1.TopToolStripPanel.SuspendLayout();
            this.toolStripContainer1.SuspendLayout();
            this.statusStrip1.SuspendLayout();
            this.toolStrip1.SuspendLayout();
            this.SuspendLayout();
            // 
            // ofdBackup
            // 
            this.ofdBackup.DefaultExt = "*.bkf";
            this.ofdBackup.Filter = "Backup Files (*.bkf)|*.bkf|All Files|*.*";
            // 
            // toolStripContainer1
            // 
            // 
            // toolStripContainer1.BottomToolStripPanel
            // 
            this.toolStripContainer1.BottomToolStripPanel.Controls.Add(this.statusStrip1);
            // 
            // toolStripContainer1.ContentPanel
            // 
            this.toolStripContainer1.ContentPanel.Controls.Add(this.tvDirs);
            this.toolStripContainer1.ContentPanel.Size = new System.Drawing.Size(527, 461);
            this.toolStripContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.toolStripContainer1.Location = new System.Drawing.Point(0, 0);
            this.toolStripContainer1.Name = "toolStripContainer1";
            this.toolStripContainer1.Size = new System.Drawing.Size(527, 508);
            this.toolStripContainer1.TabIndex = 0;
            this.toolStripContainer1.Text = "toolStripContainer1";
            // 
            // toolStripContainer1.TopToolStripPanel
            // 
            this.toolStripContainer1.TopToolStripPanel.Controls.Add(this.toolStrip1);
            // 
            // statusStrip1
            // 
            this.statusStrip1.Dock = System.Windows.Forms.DockStyle.None;
            this.statusStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.tsStatus});
            this.statusStrip1.Location = new System.Drawing.Point(0, 0);
            this.statusStrip1.Name = "statusStrip1";
            this.statusStrip1.Size = new System.Drawing.Size(527, 22);
            this.statusStrip1.TabIndex = 0;
            // 
            // tsStatus
            // 
            this.tsStatus.Name = "tsStatus";
            this.tsStatus.Size = new System.Drawing.Size(38, 17);
            this.tsStatus.Text = "Ready";
            // 
            // tvDirs
            // 
            this.tvDirs.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tvDirs.ImageIndex = 0;
            this.tvDirs.ImageList = this.ImageList1;
            this.tvDirs.Location = new System.Drawing.Point(0, 0);
            this.tvDirs.Name = "tvDirs";
            this.tvDirs.SelectedImageIndex = 0;
            this.tvDirs.Size = new System.Drawing.Size(527, 461);
            this.tvDirs.TabIndex = 1;
            this.tvDirs.NodeMouseDoubleClick += new System.Windows.Forms.TreeNodeMouseClickEventHandler(this.tvDirs_NodeMouseDoubleClick);
            this.tvDirs.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.tvDirs_AfterSelect);
            // 
            // ýmageList1
            // 
            this.ImageList1.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("ImageList1.ImageStream")));
            //this.ImageList1.ImageSize = new System.Drawing.Size(16, 16);
            this.ImageList1.TransparentColor = System.Drawing.Color.Transparent;
            this.ImageList1.Images.SetKeyName(0, "Disk");
            this.ImageList1.Images.SetKeyName(1, "Tape");
            this.ImageList1.Images.SetKeyName(2, "Volume");
            this.ImageList1.Images.SetKeyName(3, "Folder");
            this.ImageList1.Images.SetKeyName(4, "File");
            // 
            // toolStrip1
            // 
            this.toolStrip1.Dock = System.Windows.Forms.DockStyle.None;
            this.toolStrip1.GripStyle = System.Windows.Forms.ToolStripGripStyle.Hidden;
            this.toolStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.openToolStripButton,
            this.extractToolStripButton,
            this.cancelToolStripButton,
            this.toolStripButton1,
            this.opencatalogToolStripButton,
            this.savecatalogToolStripButton});
            this.toolStrip1.Location = new System.Drawing.Point(0, 0);
            this.toolStrip1.Name = "toolStrip1";
            this.toolStrip1.Size = new System.Drawing.Size(527, 25);
            this.toolStrip1.Stretch = true;
            this.toolStrip1.TabIndex = 0;
            // 
            // openToolStripButton
            // 
            this.openToolStripButton.Image = ((System.Drawing.Image)(resources.GetObject("openToolStripButton.Image")));
            this.openToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.openToolStripButton.Name = "openToolStripButton";
            this.openToolStripButton.Size = new System.Drawing.Size(120, 22);
            this.openToolStripButton.Text = "&Read Backup File...";
            this.openToolStripButton.Click += new System.EventHandler(this.openToolStripButton_Click);
            // 
            // extractToolStripButton
            // 
            this.extractToolStripButton.Enabled = false;
            this.extractToolStripButton.Image = ((System.Drawing.Image)(resources.GetObject("extractToolStripButton.Image")));
            this.extractToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.extractToolStripButton.Name = "extractToolStripButton";
            this.extractToolStripButton.Size = new System.Drawing.Size(89, 22);
            this.extractToolStripButton.Text = "&Extract To...";
            this.extractToolStripButton.Click += new System.EventHandler(this.extractToolStripButton_Click);
            // 
            // cancelToolStripButton
            // 
            this.cancelToolStripButton.Enabled = false;
            this.cancelToolStripButton.Image = ((System.Drawing.Image)(resources.GetObject("cancelToolStripButton.Image")));
            this.cancelToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.cancelToolStripButton.Name = "cancelToolStripButton";
            this.cancelToolStripButton.Size = new System.Drawing.Size(71, 22);
            this.cancelToolStripButton.Text = "&Cancel...";
            this.cancelToolStripButton.Click += new System.EventHandler(this.cancelToolStripButton_Click);
            // 
            // toolStripButton1
            // 
            this.toolStripButton1.Name = "toolStripButton1";
            this.toolStripButton1.Size = new System.Drawing.Size(6, 25);
            // 
            // opencatalogToolStripButton
            // 
            this.opencatalogToolStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
            this.opencatalogToolStripButton.Image = ((System.Drawing.Image)(resources.GetObject("opencatalogToolStripButton.Image")));
            this.opencatalogToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.opencatalogToolStripButton.Name = "opencatalogToolStripButton";
            this.opencatalogToolStripButton.Size = new System.Drawing.Size(89, 22);
            this.opencatalogToolStripButton.Text = "Open Catalog...";
            this.opencatalogToolStripButton.Click += new System.EventHandler(this.opencatalogToolStripButton_Click);
            // 
            // savecatalogToolStripButton
            // 
            this.savecatalogToolStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
            this.savecatalogToolStripButton.Enabled = false;
            this.savecatalogToolStripButton.Image = ((System.Drawing.Image)(resources.GetObject("savecatalogToolStripButton.Image")));
            this.savecatalogToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.savecatalogToolStripButton.Name = "savecatalogToolStripButton";
            this.savecatalogToolStripButton.Size = new System.Drawing.Size(87, 22);
            this.savecatalogToolStripButton.Text = "Save Catalog...";
            this.savecatalogToolStripButton.Click += new System.EventHandler(this.savecatalogToolStripButton_Click);
            // 
            // ofdCatalog
            // 
            this.ofdCatalog.DefaultExt = "*.bkf";
            this.ofdCatalog.Filter = "Catalog Files (*.cat)|*.cat|All Files|*.*";
            // 
            // sfdCatalog
            // 
            this.sfdCatalog.DefaultExt = "cat";
            this.sfdCatalog.Filter = "Catalog Files (*.cat)|*.cat|All Files|*.*";
            // 
            // frmMain
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(527, 508);
            this.Controls.Add(this.toolStripContainer1);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "frmMain";
            this.Text = "Backup Reader";
            this.FormClosed += new System.Windows.Forms.FormClosedEventHandler(this.frmMain_FormClosed);
            this.toolStripContainer1.BottomToolStripPanel.ResumeLayout(false);
            this.toolStripContainer1.BottomToolStripPanel.PerformLayout();
            this.toolStripContainer1.ContentPanel.ResumeLayout(false);
            this.toolStripContainer1.TopToolStripPanel.ResumeLayout(false);
            this.toolStripContainer1.TopToolStripPanel.PerformLayout();
            this.toolStripContainer1.ResumeLayout(false);
            this.toolStripContainer1.PerformLayout();
            this.statusStrip1.ResumeLayout(false);
            this.statusStrip1.PerformLayout();
            this.toolStrip1.ResumeLayout(false);
            this.toolStrip1.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.OpenFileDialog ofdBackup;
        private System.Windows.Forms.ToolStripContainer toolStripContainer1;
        private System.Windows.Forms.StatusStrip statusStrip1;
        private System.Windows.Forms.ToolStrip toolStrip1;
        private System.Windows.Forms.ToolStripButton openToolStripButton;
        private System.Windows.Forms.ToolStripStatusLabel tsStatus;
        private System.Windows.Forms.ImageList ImageList1;
        private System.Windows.Forms.TreeView tvDirs;
        private System.Windows.Forms.ToolStripButton extractToolStripButton;
        private System.Windows.Forms.FolderBrowserDialog fbdBackup;
        private System.Windows.Forms.ToolStripButton cancelToolStripButton;
        private System.Windows.Forms.ToolStripSeparator toolStripButton1;
        private System.Windows.Forms.ToolStripButton opencatalogToolStripButton;
        private System.Windows.Forms.ToolStripButton savecatalogToolStripButton;
        private System.Windows.Forms.OpenFileDialog ofdCatalog;
        private System.Windows.Forms.SaveFileDialog sfdCatalog;
    }
}

