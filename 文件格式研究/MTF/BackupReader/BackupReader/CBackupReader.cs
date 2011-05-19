
namespace BackupReader
{
    /// <summary>
    /// Represents a backup file reader.
    /// </summary>
    class CBackupReader
    {
        private long mLastPos;
        private long mIncrement;
        private bool mCancel;
        private CBackupStream mStream;

        /// <summary>
        /// Provides an event handler for the OnProgressChange event. 
        /// Progress is an integer between 1-100, representing the progress of
        /// the catalog read operation.
        /// </summary>
        public delegate void ProgressChange(int Progress);
        /// <summary>
        /// Occurs when the catalog read progress changes by 1%.
        /// </summary>
        public event ProgressChange OnProgressChange;

        /// <summary>
        /// Returns the underlying stream.
        /// </summary>
        public CBackupStream Stream
        {
            get { return mStream; }
        }
	 
        /// <summary>
        /// Reads the entire backup file and returns a root catalog node.
        /// The root node contains backup sets/volumes/directories/files
        /// as child nodes.
        /// </summary>
        public CCatalogNode ReadCatalog()
        {
            // Set to true to cancel reading
            mCancel = false;

            // Read the media header
            CTapeHeaderDescriptorBlock tape = (CTapeHeaderDescriptorBlock)mStream.ReadDBLK();
            // Read soft file mark
            CSoftFilemarkDescriptorBlock file = (CSoftFilemarkDescriptorBlock)mStream.ReadDBLK();

            // Create the root catalog node
            CCatalogNode node = new CCatalogNode(tape.MediaName, ENodeType.Root, 0);
            CCatalogNode nLastSet = null;
            CCatalogNode nLastVolume = null;
            CCatalogNode nLastDir = null;

            // Get next block type
            EBlockType bt = mStream.PeekNextBlockType();
            while ((bt != EBlockType.MTF_EOTM) && (bt != 0) && (mCancel == false))
            {
                // Read next block
                CDescriptorBlock block = mStream.ReadDBLK();

                // Add to catalog
                if (bt == EBlockType.MTF_SSET)
                {
                    CStartOfDataSetDescriptorBlock sset = (CStartOfDataSetDescriptorBlock)block;
                    CCatalogNode cnode = node.AddSet("Set: " + sset.DataSetNumber + " - " + sset.DataSetName, block.StartPosition);
                    nLastSet = cnode;
                }
                else if (bt == EBlockType.MTF_VOLB)
                {
                    CVolumeDescriptorBlock vol = (CVolumeDescriptorBlock)block;
                    CCatalogNode cnode = nLastSet.AddVolume(vol.DeviceName, block.StartPosition);
                    nLastVolume = cnode;
                }
                else if (bt == EBlockType.MTF_DIRB)
                {
                    CDirectoryDescriptorBlock dir = (CDirectoryDescriptorBlock)block;
                    // Check if the directory name is contained in a data stream
                    CCatalogNode cnode = null;
                    if ((dir.DIRBAttributes & EDIRBAttributes.DIRB_PATH_IN_STREAM_BIT) != 0)
                    {
                        foreach (CDataStream data in dir.Streams)
                        {
                            if (data.Header.StreamID == "PNAM")
                            {
                                if (dir.StringType == EStringType.ANSI)
                                {
                                    System.Text.ASCIIEncoding encoding = new System.Text.ASCIIEncoding();
                                    string str = encoding.GetString(data.Data);
                                    str = str.Substring(0, str.Length - 1);
                                    cnode = nLastVolume.AddFolder(str, block.StartPosition);
                                }
                                else if (dir.StringType == EStringType.Unicode)
                                {
                                    System.Text.UnicodeEncoding encoding = new System.Text.UnicodeEncoding();
                                    string str = encoding.GetString(data.Data);
                                    str = str.Substring(0, str.Length - 1);
                                    cnode = nLastVolume.AddFolder(str, block.StartPosition);
                                }

                            }
                        }
                    }
                    else
                        cnode = nLastVolume.AddFolder(dir.DirectoryName.Substring(0, dir.DirectoryName.Length - 1), block.StartPosition);

                    if (cnode != null) nLastDir = cnode;
                }
                else if (bt == EBlockType.MTF_FILE)
                {
                    CFileDescriptorBlock fil = (CFileDescriptorBlock)block;
                    // Check if the file name is contained in a data stream
                    CCatalogNode cnode = null;
                    if ((fil.FileAttributes & EFileAttributes.FILE_NAME_IN_STREAM_BIT) != 0)
                    {
                        foreach (CDataStream data in fil.Streams)
                        {
                            if (data.Header.StreamID == "FNAM")
                            {
                                if (fil.StringType == EStringType.ANSI)
                                {
                                    System.Text.ASCIIEncoding encoding = new System.Text.ASCIIEncoding();
                                    string str = encoding.GetString(data.Data);
                                    cnode = nLastDir.AddFile(str, block.StartPosition);
                                }
                                else if (fil.StringType == EStringType.Unicode)
                                {
                                    System.Text.UnicodeEncoding encoding = new System.Text.UnicodeEncoding();
                                    string str = encoding.GetString(data.Data);
                                    cnode = nLastDir.AddFile(str, block.StartPosition);
                                }

                            }
                        }
                    }
                    else
                        cnode = nLastDir.AddFile(fil.FileName, block.StartPosition);
                }


                // Get next block type
                bt = mStream.PeekNextBlockType();

                // Check progress
                if (mStream.BaseStream.Position > mLastPos + mIncrement)
                {
                    mLastPos = mStream.BaseStream.Position;
                    OnProgressChange((int)((float)mLastPos / (float)mStream.BaseStream.Length * 100.0f));
                }
            }

            return node;
        }

        /// <summary>
        /// Stops reading the catalog. The nodes that has already been read will still be available.
        /// </summary>
        public void CancelRead()
        {
            mCancel = true;
        }

        /// <summary>
        /// Opens a backup file.
        /// </summary>
        public void Open(string Filename)
        {
            mStream = new CBackupStream(Filename);
            mIncrement = mStream.BaseStream.Length / 100;
            mLastPos = 0;
            mCancel = false;
        }

        /// <summary>
        /// Closes the backup file.
        /// </summary>
        public void Close()
        {
            mStream.Close();
        }

        public CBackupReader()
        {

        }

        public CBackupReader(string Filename)
        {
            Open(Filename);
        }

        ~CBackupReader()
        {
            Close();
        }
    }

}
