
namespace BackupReader
{
    enum ENodeType : int 
    {
        Root = 0,
        Set = 1,
        Volume = 2,
        Folder = 3,
        File = 4,
    }

    /// <summary>
    /// Catalog nodes represents the blocks in the backup file.
    /// </summary>
    class CCatalogNode
    {
        private string mName;
        private ENodeType mType;
        private long mOffset;
        private CCatalogNode mParent;
        private System.Collections.Generic.List<CCatalogNode> mNodes;

        public string Name
        {
            get { return mName; }
        }

        public ENodeType Type
        {
            get { return mType; }
        }

        public long Offset
        {
            get { return mOffset; }
        }

        protected CCatalogNode Parent
        {
            get { return mParent; }
        }

        public System.Collections.Generic.List<CCatalogNode> Children
        {
            get { return mNodes; }
        }

        public CCatalogNode()
        {
            mName = "";
            mType = ENodeType.Root;
            mOffset = 0;
            mParent = null;
            mNodes = new System.Collections.Generic.List<CCatalogNode>();
        }

        public CCatalogNode(string nName, ENodeType nType, long nOffset)
        {
            mName = nName;
            mType = nType;
            mOffset = nOffset;
            mParent = null;
            mNodes = new System.Collections.Generic.List<CCatalogNode>();
        }

        public CCatalogNode AddSet(string nName, long nOffset)
        {
            CCatalogNode cnode = new CCatalogNode(nName, ENodeType.Set, nOffset);
            cnode.mParent = this;
            mNodes.Add(cnode);
            return cnode;
        }

        public CCatalogNode AddVolume(string nName, long nOffset)
        {
            CCatalogNode cnode = new CCatalogNode(nName, ENodeType.Volume, nOffset);
            cnode.mParent = this;
            mNodes.Add(cnode);
            return cnode;
        }

        public CCatalogNode AddFolder(string nName, long nOffset)
        {
            CCatalogNode cnode = new CCatalogNode(nName, ENodeType.Folder, nOffset);
            cnode.mParent = this;
            mNodes.Add(cnode);
            return cnode;
        }

        public CCatalogNode AddFile(string nName, long nOffset)
        {
            CCatalogNode cnode = new CCatalogNode(nName, ENodeType.File, nOffset);
            cnode.mParent = this;
            mNodes.Add(cnode);
            return cnode;
        }

        public bool ExtractTo(CBackupReader BackupFile, string TargetPath)
        {
            // Ensure that the target path has a trailing '\'
            if (TargetPath[TargetPath.Length - 1] != '\\')
                TargetPath += '\\';

            if ((mType == ENodeType.Root) || (mType == ENodeType.Set))
            {
                throw new System.Exception("Tape and set nodes can not be extracted. Only volume, folder or file nodes can be extracted.");
            }
            else if (mType == ENodeType.Volume)
            {
                System.IO.DirectoryInfo dirinfo = System.IO.Directory.CreateDirectory(TargetPath);
                foreach (CCatalogNode node in mNodes)
                    node.ExtractTo(BackupFile, TargetPath);
            }
            else if (mType == ENodeType.Folder)
            {
                System.IO.DirectoryInfo dirinfo = System.IO.Directory.CreateDirectory(TargetPath + mName);
                foreach (CCatalogNode node in mNodes)
                    node.ExtractTo(BackupFile, dirinfo.FullName);
            }
            else if (mType == ENodeType.File)
            {
                // Create the target directory if it does not exist
                System.IO.Directory.CreateDirectory(TargetPath);
                BackupFile.Stream.BaseStream.Seek(mOffset, System.IO.SeekOrigin.Begin);
                System.IO.FileStream file = new System.IO.FileStream(TargetPath + mName, System.IO.FileMode.Create);
                CFileDescriptorBlock fil = (CFileDescriptorBlock)BackupFile.Stream.ReadDBLK();
                foreach (CDataStream data in fil.Streams)
                {
                    if (data.Header.StreamID == "STAN")
                    {
                        file.Write(data.Data, 0, data.Data.Length);
                    }
                }
                file.Close();
            }

            return true;
        }

        /// <summary>
        /// Saves the catalog to the disk.
        /// </summary>
        public static void SaveCatalog(string Filename, CCatalogNode Node, string BackupFilename)
        {
            // Open the file
            System.IO.BinaryWriter file = new System.IO.BinaryWriter(new System.IO.FileStream(Filename, System.IO.FileMode.Create, System.IO.FileAccess.Write));

            // Write full path to backup file
            file.Write(BackupFilename);

            // Write nodes
            Node.SaveNode(file);

            // Close the file
            file.Close();
        }

        /// <summary>
        /// Reads the name of the backup file used to create the catalog.
        /// </summary>
        public static string ReadBackupFilename(string Filename)
        {
            // Open the file
            System.IO.BinaryReader file = new System.IO.BinaryReader(new System.IO.FileStream(Filename, System.IO.FileMode.Open, System.IO.FileAccess.Read));

            // Read backup file name
            string bkfname = file.ReadString();

            // Close the file
            file.Close();

            return bkfname;
        }

        /// <summary>
        /// Reads the catalog from the disk.
        /// </summary>
        public static CCatalogNode ReadCatalog(string Filename)
        {
            // Create the root node
            CCatalogNode Node = new CCatalogNode();

            // Open the file
            System.IO.BinaryReader file = new System.IO.BinaryReader(new System.IO.FileStream(Filename, System.IO.FileMode.Open, System.IO.FileAccess.Read));

            // Read backup file name
            file.ReadString();

            // Read nodes
            Node.ReadNode(file);

            // Close the file
            file.Close();

            return Node;
        }

        private void SaveNode(System.IO.BinaryWriter file)
        {
            // Write node info
            file.Write((int)mType);
            file.Write(mName);
            file.Write(mOffset);
            file.Write(mNodes.Count);

            // Recursively write child nodes
            foreach (CCatalogNode node in mNodes)
                node.SaveNode(file);
        }

        private void ReadNode(System.IO.BinaryReader file)
        {
            // Read node info
            mType = (ENodeType)file.ReadInt32();
            mName = file.ReadString();
            mOffset = file.ReadInt64();
            int count = file.ReadInt32();

            // Recursively read child nodes
            for (int i = 0; i < count; i++)
            {
                CCatalogNode node = new CCatalogNode();
                mNodes.Add(node);
                node.ReadNode(file);
            }
        }

    }

}
