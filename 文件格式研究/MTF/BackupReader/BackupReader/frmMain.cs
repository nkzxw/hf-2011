using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace BackupReader
{
    public partial class frmMain : Form
    {
        private string mFileName;
        private CBackupReader mFile;

        public frmMain()
        {
            InitializeComponent();
        }

        private void openToolStripButton_Click(object sender, EventArgs e)
        {
            if (ofdBackup.ShowDialog() == DialogResult.OK)
            {
                // Open the backup file
                mFileName = ofdBackup.FileName;
                tsStatus.Text = "Reading " + mFileName;

                // UI cues
                openToolStripButton.Enabled = false;
                extractToolStripButton.Enabled = false;
                cancelToolStripButton.Enabled = true;
                opencatalogToolStripButton.Enabled = false;
                savecatalogToolStripButton.Enabled = false;

                // Open and read the catalog
                if (mFile != null) mFile.Close();
                mFile = new CBackupReader(mFileName);
                mFile.OnProgressChange += new CBackupReader.ProgressChange(mFile_OnProgressChange);
                CCatalogNode node = mFile.ReadCatalog();
                
                // Populate tree view
                tvDirs.Nodes.Clear();
                tvDirs.Nodes.Add("root", node.Name, 0);
                tvDirs.Nodes[0].Tag = node;
                PopulateTreeView(tvDirs.Nodes[0], node);
                tsStatus.Text = "Select a single volume, folder or file to extract.";

                // UI cues
                openToolStripButton.Enabled = true;
                extractToolStripButton.Enabled = false;
                cancelToolStripButton.Enabled = false;
                savecatalogToolStripButton.Enabled = true;
                opencatalogToolStripButton.Enabled = true;
                savecatalogToolStripButton.Enabled = true;
            }
        }

        private void PopulateTreeView(TreeNode TNode, CCatalogNode CNode)
        {
            foreach (CCatalogNode node in CNode.Children)
            {
                TreeNode snode = new TreeNode(node.Name);
                if (node.Type == ENodeType.Set)
                {
                    snode.ImageIndex = 1;
                    snode.SelectedImageIndex = 1;
                    snode.Tag = node;
                    TNode.Nodes.Add(snode);
                }
                else if (node.Type == ENodeType.Volume)
                {
                    snode.ImageIndex = 2;
                    snode.SelectedImageIndex = 2;
                    snode.Tag = node;
                    TNode.Nodes.Add(snode);
                }
                else if (node.Type == ENodeType.Folder)
                {
                    snode.ImageIndex = 3;
                    snode.SelectedImageIndex = 3;
                    snode.Tag = node;
                    TNode.Nodes.Add(snode);
                }
                else if (node.Type == ENodeType.File)
                {
                    snode.ImageIndex = 4;
                    snode.SelectedImageIndex = 4;
                    snode.Tag = node;
                    TNode.Nodes.Add(snode);
                }
                PopulateTreeView(snode, node);
            }
        }

        private void extractToolStripButton_Click(object sender, EventArgs e)
        {
            if (tvDirs.SelectedNode == null) return;

            // Get the selected catalog node from tree node tag
            CCatalogNode node = (CCatalogNode)tvDirs.SelectedNode.Tag;
            if (node == null) return;
            if ((node.Type == ENodeType.Root) || (node.Type == ENodeType.Set)) return;

            // Get extraction path
            if (fbdBackup.ShowDialog() != DialogResult.OK) return;
            string TargetPath = fbdBackup.SelectedPath;

            // Extract the selected node and child nodes
            node.ExtractTo(mFile, TargetPath);
        }

        private void tvDirs_NodeMouseDoubleClick(object sender, TreeNodeMouseClickEventArgs e)
        {
            if (e.Button != MouseButtons.Left) return;

            // Get the selected catalog node from tree node tag
            CCatalogNode node = (CCatalogNode)tvDirs.SelectedNode.Tag;
            if (node == null) return;
            if (node.Type != ENodeType.File) return;

            // Extract the selected node to a temporary folder
            string TargetPath = System.IO.Path.GetTempPath();
            node.ExtractTo(mFile, TargetPath);

            // Open the file
            System.Diagnostics.ProcessStartInfo psi = new System.Diagnostics.ProcessStartInfo(TargetPath + node.Name);
            psi.UseShellExecute = true;
            psi.ErrorDialog = true;
            psi.ErrorDialogParentHandle = this.Handle;
            try
            {
                System.Diagnostics.Process.Start(psi);
            }
            catch(Win32Exception ex)
            {
                MessageBox.Show(this, "Could not open the file '" + node.Name + "'." + ex.ToString(), "Backup Reader", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
        }

        private void frmMain_FormClosed(object sender, FormClosedEventArgs e)
        {
            if (mFile != null) mFile.Close();
        }

        void mFile_OnProgressChange(int Progress)
        {
            tsStatus.Text = "Reading backup file. %" + Progress.ToString() + " completed.";
            System.Windows.Forms.Application.DoEvents();
        }

        private void cancelToolStripButton_Click(object sender, EventArgs e)
        {
            if (MessageBox.Show(this, "Are you sure you want to cancel?", "Backup Reader", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
                mFile.CancelRead();
        }

        private void tvDirs_AfterSelect(object sender, TreeViewEventArgs e)
        {
            extractToolStripButton.Enabled = false;

            if (tvDirs.SelectedNode == null) return;
            
            // Get the selected catalog node from tree node tag
            CCatalogNode node = (CCatalogNode)tvDirs.SelectedNode.Tag;
            if (node == null) return;
            if ((node.Type == ENodeType.Root) || (node.Type == ENodeType.Set)) return;

            extractToolStripButton.Enabled = true;
        }

        private void opencatalogToolStripButton_Click(object sender, EventArgs e)
        {
            if (ofdCatalog.ShowDialog() == DialogResult.Cancel) return;

            // Read the catalog from the file
            tsStatus.Text = "Reading catalog...";
            mFileName = CCatalogNode.ReadBackupFilename(ofdCatalog.FileName);
            if (mFile != null) mFile.Close();
            mFile = new CBackupReader(mFileName);
            CCatalogNode node = CCatalogNode.ReadCatalog(ofdCatalog.FileName);

            // Populate tree view
            tvDirs.Nodes.Clear();
            tvDirs.Nodes.Add("root", node.Name, 0);
            tvDirs.Nodes[0].Tag = node;
            PopulateTreeView(tvDirs.Nodes[0], node);
            tsStatus.Text = "Select a single volume, folder or file to extract.";

            // UI cues
            extractToolStripButton.Enabled = false;
            cancelToolStripButton.Enabled = false;
            savecatalogToolStripButton.Enabled = true;
        }

        private void savecatalogToolStripButton_Click(object sender, EventArgs e)
        {
            if (tvDirs.Nodes.Count == 0) return;
            if (tvDirs.Nodes[0].Tag == null) return;
            if (sfdCatalog.ShowDialog() == DialogResult.Cancel) return;

            // Save the catalog to the file
            CCatalogNode.SaveCatalog(sfdCatalog.FileName, (CCatalogNode)tvDirs.Nodes[0].Tag, mFileName);
        }
    }
}