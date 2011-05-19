
namespace BackupReader
{

    enum EBlockType: uint
    {
        /// <summary>
        /// TAPE descriptor block
        /// </summary>
        MTF_TAPE = 0x45504154,
        /// <summary>
        /// Start of data SET descriptor block
        /// </summary>
        MTF_SSET = 0x54455353,
        /// <summary>
        /// VOLume descriptor Block
        /// </summary>
        MTF_VOLB = 0x424C4F56,
        /// <summary>
        /// DIRectory descriptor Block
        /// </summary>
        MTF_DIRB = 0x42524944,
        /// <summary>
        /// FILE descriptor block
        /// </summary>
        MTF_FILE = 0x454C4946,
        /// <summary>
        /// Corrupt object descriptor block
        /// </summary>
        MTF_CFIL = 0x4C494643,
        /// <summary>
        /// End of Set Pad descriptor Block
        /// </summary>
        MTF_ESPB = 0x42505345,
        /// <summary>
        /// End of SET descriptor block
        /// </summary>
        MTF_ESET = 0x54455345,
        /// <summary>
        /// End Of Tape Marker descriptor block
        /// </summary>
        MTF_EOTM = 0x4D544F45,
        /// <summary>
        /// Soft FileMark descriptor Block
        /// </summary>
        MTF_SFMB = 0x424D4653,
    }

    enum EBlockAttributes : uint
    {
        MTF_CONTINUATION = 0x1,        // Bit set if DBLK is a continuation from the previous tape. any BIT0
        MTF_COMPRESSION = 0x4,         // Bit set if compression may be active. any BIT2
        MTF_EOS_AT_EOM = 0x8,          // Bit set if the End Of Medium was hit during end of set processing. any BIT3
        MTF_SET_MAP_EXISTS = 0x10000,  // Bit set if an Media Based Catalog Set Map can be found on the tape. MTF_TAPE BIT16
        MTF_FDD_ALLOWED = 0x20000,     // Bit set if an attempt will be made to put a Media Based Catalog File/Directory Detail section on the tape. MTF_TAPE BIT17
        MTF_FDD_EXISTS = 0x10000,      // Bit set if a Media Based Catalog File/Directory Detail section has been successfully put on the tape for this Data Set. MTF_SSET BIT16
        MTF_ENCRYPTION = 0x20000,      // Bit set if encryption is active for the data streams within this Data Set. MTF_SSET BIT17
        MTF_FDD_ABORTED = 0x10000,     // Bit set if a Media Based Catalog File/Directory Detail section was aborted for any reason during the write operation. MTF_ESET BIT16
        MTF_END_OF_FAMILY = 0x20000,   // Bit set if the Media Based Catalog Set Map has been aborted. This condition means that additional Data Sets cannot be appended to the tape. MTF_ESET BIT17
        MTF_ABORTED_SET = 0x40000,     // Bit set if the Data Set was aborted while being written. This can happen if a fatal error occurs while writing data, or if the user terminates the data management operation. An MTF_ESET DBLK containing this flag is put at the end of the Data Set even if it was aborted. MTF_ESET BIT18
        MTF_NO_ESET_PBA = 0x10000,     // Bit set if no Data Set ends on this tape (i.e. continuation tape must follow this tape). MTF_EOTM BIT16
        MTF_INVALID_ESET_PBA = 0x20000,// Bit set if the Physical Block Address (PBA) of the MTF_ESET is invalid because the tape drive doesn't support physical block addressing. MTF_EOTM BIT17
    }

    enum EOSID : byte 
    {
        NetWare = 1,         // 0
        NetWare_SMS = 13,    // 1, 2
        Windows_NT = 14,     // 0
        DOS_Windows_3X = 24, // 0
        OS_2 = 25,           // 0
        Windows_95 = 26,     // 0
        Macintosh = 27,      // 0
        UNIX = 28,           // 0
        // To Be Assigned 33 - 127
        // Vendor Specific 128 - 255
    }

    enum EStringType : byte
    {
        None = 0,
        ANSI = 1,
        Unicode = 2,
    }

    /// <summary>
    /// Represents a descriptor block. Descriptor blocks define the types and 
    /// attributes of the data in the backup file.
    /// </summary>
    class CDescriptorBlock
    {
        public long StartPosition;

        public EBlockType BlockType;
        public EBlockAttributes Attributes;
        public ushort OffsetToFirstEvent; // Obsolete
        public EOSID OSID;
        public byte OSVersion;
        public ulong DisplayableSize;
        public ulong FormatLogicalAddress;
        public ushort ReservedMBC;
        public ushort Reserved1;
        public ushort Reserved2;
        public ushort Reserved3;
        public uint ControlBlock;
        public uint Reserved4;
        public COSSpecificData OsSpecificData;
        public EStringType StringType;
        public byte Reserved5;
        public ushort HeaderChecksum;

        public System.Collections.Generic.List<CDataStream> Streams; 

        /// <summary>
        /// Read block header.
        /// </summary>
        protected void ReadData(CBackupStream Reader)
        {
            StartPosition = Reader.BaseStream.Position;
            Streams = new System.Collections.Generic.List<CDataStream>();

            BlockType = (EBlockType)Reader.ReadUInt32();
            Attributes = (EBlockAttributes)Reader.ReadUInt32();
            OffsetToFirstEvent = Reader.ReadUInt16();
            OSID = (EOSID)Reader.ReadByte();
            OSVersion = Reader.ReadByte();
            DisplayableSize = Reader.ReadUInt64();
            FormatLogicalAddress = Reader.ReadUInt64();
            ReservedMBC = Reader.ReadUInt16();
            Reserved1 = Reader.ReadUInt16();
            Reserved2 = Reader.ReadUInt16();
            Reserved3 = Reader.ReadUInt16();
            ControlBlock = Reader.ReadUInt32();
            Reserved4 = Reader.ReadUInt32();
            OsSpecificData = Reader.ReadOsSpecificData(StartPosition, OSID, OSVersion, BlockType);
            StringType = (EStringType)Reader.ReadByte();
            Reserved5 = Reader.ReadByte();
            HeaderChecksum = Reader.ReadUInt16();
        }

        /// <summary>
        /// Read streams following this block.
        /// </summary>
        /// <param name="Reader"></param>
        protected void ReadStreams(CBackupStream Reader)
        {
            // Move to stream
            long off = OffsetToFirstEvent + StartPosition;
            // Make sure we are at a 4 byte boundary
            long nullbytecount = (4 - (off % 4)) % 4;

            Reader.BaseStream.Seek(off + nullbytecount, System.IO.SeekOrigin.Begin);
            string streamtype = "";

            do
            {
                // Read next stream
                CDataStream stream = new CDataStream(Reader);
                streamtype = stream.Header.StreamID;
                Streams.Add(stream);
            } while ((streamtype != "SPAD") && (streamtype != ""));
        }

        public CDescriptorBlock()
        {
        }

        public CDescriptorBlock(CBackupStream Reader)
        {
            ReadData(Reader);
        }
    }

    enum ETapeAttributes : uint 
    {
        TAPE_SOFT_FILEMARK_BIT = 1,
        TAPE_MEDIA_LABEL_BIT = 2,
        // Reserved 2-23
        // Vendor Specific 24-31
    }

    enum EMediaBasedCatalogType : ushort
    {
        No_MBC = 0,
        Type_1_MBC = 1,
        Type_2_MBC = 2,
    }

    class CTapeHeaderDescriptorBlock : CDescriptorBlock
    {
        public uint MediaFamilyID;
        public ETapeAttributes TapeAttributes;
        public ushort MediaSequenceNumber;
        public ushort PasswordEncryptionAlgorithm;
        public ushort SoftFilemarkBlockSize;
        public EMediaBasedCatalogType MediaBasedCatalogType;
        public string MediaName;
        public string MediaDescription;
        public string MediaPassword;
        public string SoftwareName;
        public ushort FormatLogicalBlockSize;
        public ushort SoftwareVendorID;
        public System.DateTime MediaDate;
        public byte MTFMajorVersion;

        public CTapeHeaderDescriptorBlock(CBackupStream Reader)
        {
            base.ReadData(Reader);
            MediaFamilyID = Reader.ReadUInt32();
            TapeAttributes = (ETapeAttributes)Reader.ReadUInt32();
            MediaSequenceNumber = Reader.ReadUInt16();
            PasswordEncryptionAlgorithm = Reader.ReadUInt16();
            SoftFilemarkBlockSize = Reader.ReadUInt16();
            MediaBasedCatalogType = (EMediaBasedCatalogType)Reader.ReadUInt16();
            MediaName = Reader.ReadString(StartPosition, StringType);
            MediaDescription = Reader.ReadString(StartPosition, StringType);
            MediaPassword = Reader.ReadString(StartPosition, StringType);
            SoftwareName = Reader.ReadString(StartPosition, StringType);
            FormatLogicalBlockSize = Reader.ReadUInt16();
            SoftwareVendorID = Reader.ReadUInt16();
            MediaDate = Reader.ReadDate();
            MTFMajorVersion = Reader.ReadByte();
            base.ReadStreams(Reader);
        }
}

    class CEndOfTapeMarkerDescriptorBlock : CDescriptorBlock 
    {
        public ulong LastESETPBA;

        public CEndOfTapeMarkerDescriptorBlock(CBackupStream Reader)
        {
            base.ReadData(Reader);
            LastESETPBA = Reader.ReadUInt64();
            base.ReadStreams(Reader);
        }
    }

    enum ESSETAttributes : uint 
    {
        SSET_TRANSFER_BIT = 0x1,     // This bit is set if the data management operation is a “transfer”. It indicates that the files in this Data Set were removed from the source media after the operation was completed. BIT0
        SSET_COPY_BIT = 0x2,         // This bit is set if the operation is a “copy”. The copy method copies all selected files from the primary storage to the media. The file’s “modified” flag IS NOT reset afterwards. BIT1
        SSET_NORMAL_BIT = 0x4,       // This bit is set if the backup type is “normal”. The normal backup method backs up all selected files. The file’s “modified” flag IS reset afterwards. BIT2
        SSET_DIFFERENTIAL_BIT = 0x8, // This bit is set if the backup type is “differential”. The differential backup method only backs up selected files having their “modified” flag set. The file’s “modified” flag IS NOT reset afterwards. BIT3
        SSET_INCREMENTAL_BIT = 0x10, // This bit is set if the backup type is “incremental”. The incremental backup method only backs up selected files having their “modified” flag set. The file’s “modified” flag IS reset afterwards. BIT4
        SSET_DAILY_BIT = 0x20,       // This bit is set if the backup type is “daily”. The daily backup method only backs up selected files created or modified with today’s date. The file’s “modified” flag IS NOT reset afterwards. BIT5
        // Reserved (set to zero) BIT6 - BIT23
        // Vendor Specific BIT24 - BIT31
    }

    class CStartOfDataSetDescriptorBlock : CDescriptorBlock 
    {
        public ESSETAttributes SSETAttributes;
        public ushort PasswordEncryptionAlgorithm;
        public ushort SoftwareCompressionAlgorithm;
        public ushort SoftwareVendorID;
        public ushort DataSetNumber;
        public string DataSetName;
        public string DataSetDescription;
        public string DataSetPassword;
        public string UserName;
        public ulong PhysicalBlockAddress;
        public System.DateTime MediaWriteDate;
        public byte SoftwareMajorVersion;
        public byte SoftwareMinorVersion;
        public sbyte MTFTimeZone;
        public byte MTFMinorVersion;
        public byte MediaCatalogVersion;

        public CStartOfDataSetDescriptorBlock(CBackupStream Reader)
        {
            base.ReadData(Reader);
            SSETAttributes = (ESSETAttributes)Reader.ReadUInt32();
            PasswordEncryptionAlgorithm = Reader.ReadUInt16();
            SoftwareCompressionAlgorithm = Reader.ReadUInt16();
            SoftwareVendorID = Reader.ReadUInt16();
            DataSetNumber = Reader.ReadUInt16();
            DataSetName = Reader.ReadString(StartPosition, StringType);
            DataSetDescription = Reader.ReadString(StartPosition, StringType);
            DataSetPassword = Reader.ReadString(StartPosition, StringType);
            UserName = Reader.ReadString(StartPosition, StringType);
            PhysicalBlockAddress = Reader.ReadUInt64();
            MediaWriteDate = Reader.ReadDate();
            SoftwareMajorVersion = Reader.ReadByte();
            SoftwareMinorVersion = Reader.ReadByte();
            MTFTimeZone = Reader.ReadSByte();
            MTFMinorVersion = Reader.ReadByte();
            MediaCatalogVersion = Reader.ReadByte();
            base.ReadStreams(Reader);
        }
    }

    class CEndOfDataSetDescriptorBlock : CDescriptorBlock 
    {
        public ESSETAttributes ESETAttributes;
        public uint NumberOfCorruptFiles;
        public ulong ReservedforMBC1;
        public ulong ReservedforMBC2;
        public ushort FDDMediaSequenceNumber;
        public ushort DataSetNumber;
        public System.DateTime MediaWriteDate;

        public CEndOfDataSetDescriptorBlock(CBackupStream Reader)
        {
            base.ReadData(Reader);
            ESETAttributes = (ESSETAttributes)Reader.ReadUInt32();
            NumberOfCorruptFiles = Reader.ReadUInt32();
            ReservedforMBC1 = Reader.ReadUInt64();
            ReservedforMBC2 = Reader.ReadUInt64();
            FDDMediaSequenceNumber = Reader.ReadUInt16();
            DataSetNumber = Reader.ReadUInt16();
            MediaWriteDate = Reader.ReadDate();
            base.ReadStreams(Reader);
        }
    }

    enum EVOLBAttributes : uint 
    {
        VOLB_NO_REDIRECT_RESTORE_BIT = 0x1, // Objects following this DBLK can only be restored to the device from which they were backed up. BIT0
        VOLB_NON_VOLUME_BIT = 0x2,          // Objects following this DBLK are not associated with a volume. BIT1
        VOLB_DEV_DRIVE_BIT = 0x4,           // Device name format is, “<drive letter>:”. BIT2
        VOLB_DEV_UNC_BIT = 0x8,             // Device name format is UNC. BIT3
        VOLB_DEV_OS_SPEC_BIT = 0x10,        // Device name format is OS specific (refer to Appendix C for details on a given OS). BIT4
        VOLB_DEV_VEND_SPEC_BIT = 0x20       // Device name format is vendor specific. BIT5
        // Reserved (set to zero) BIT6 - BIT23
        // Vendor Specific BIT24 - BIT31
    }

    class CVolumeDescriptorBlock : CDescriptorBlock 
    {
        public EVOLBAttributes VOLBAttributes;
        public string DeviceName;
        public string VolumeName;
        public string MachineName;
        public System.DateTime MediaWriteDate;

        public CVolumeDescriptorBlock(CBackupStream Reader)
        {
            base.ReadData(Reader);
            VOLBAttributes = (EVOLBAttributes)Reader.ReadUInt32();
            DeviceName = Reader.ReadString(StartPosition, StringType);
            VolumeName = Reader.ReadString(StartPosition, StringType);
            MachineName = Reader.ReadString(StartPosition, StringType);
            MediaWriteDate = Reader.ReadDate();
            base.ReadStreams(Reader);
        }
    }

    enum EDIRBAttributes : uint 
    {
        DIRB_READ_ONLY_BIT = 0x100,        // This bit is set if the directory is marked as read only. BIT8
        DIRB_HIDDEN_BIT = 0x200,           // This bit is set if the directory is hidden from the user. BIT9
        DIRB_SYSTEM_BIT = 0x400,           // This bit is set if the directory is a system directory. BIT10
        DIRB_MODIFIED_BIT = 0x800,         // This bit is set if the directory has been modified. This is also referred to as an “archive” flag. BIT11
        DIRB_EMPTY_BIT = 0x10000,          // This bit set if the directory contained no files or subdirectories. BIT16
        DIRB_PATH_IN_STREAM_BIT = 0x20000, // This bit set if the directory path is stored in a stream associated with this DBLK. BIT17
        DIRE_CORRUPT_BIT = 0x40000,        // This bit set if the data associated with the directory could not be read. BIT18
        // Reserved (set to zero) BIT0 - BIT7, BIT12 - BIT15, BIT19 - BIT23
        // Vendor Specific BIT24 - BIT31
    }

    class CDirectoryDescriptorBlock : CDescriptorBlock 
    {
        public EDIRBAttributes DIRBAttributes;
        public System.DateTime LastModificationDate;
        public System.DateTime CreationDate;
        public System.DateTime BackupDate;
        public System.DateTime LastAccessDate;
        public uint DirectoryID;
        public string DirectoryName;

        public CDirectoryDescriptorBlock(CBackupStream Reader)
        {
            base.ReadData(Reader);
            DIRBAttributes = (EDIRBAttributes)Reader.ReadUInt32();
            LastModificationDate = Reader.ReadDate();
            CreationDate = Reader.ReadDate();
            BackupDate = Reader.ReadDate();
            LastAccessDate = Reader.ReadDate();
            DirectoryID = Reader.ReadUInt32();
            // MTF uses '\0' as the path seperator. Replace them with '\\'
            DirectoryName = Reader.ReadString(StartPosition, StringType).Replace('\0','\\');
            base.ReadStreams(Reader);
        }
    }

    enum EFileAttributes : uint 
    {
        FILE_READ_ONLY_BIT = 0x100,        // This bit is set if the file is marked as read only. BIT8
        FILE_HIDDEN_BIT = 0x200,           // This bit is set if the file is hidden from the user. BIT9
        FILE_SYSTEM_BIT = 0x400,           // This bit is set if the file is a system file. BIT10
        FILE_MODIFIED_BIT = 0x800,         // This bit is set if the file has been modified. This is also referred to as an “archive” flag. BIT11
        FILE_IN_USE_BIT = 0x10000,         // This bit set if the file was in use at the time it was backed up. BIT16
        FILE_NAME_IN_STREAM_BIT = 0x20000, // This bit set if the file name is stored in a stream associated with this DBLK. BIT17
        FILE_CORRUPT_BIT = 0x40000,        // This bit set if the data associated with the file could not be read. BIT18
        // Reserved (set to zero) BIT0 - BIT7, BIT12 - BIT15, BIT19 - BIT23
        // Vendor Specific BIT24 - BIT31
    }

    class CFileDescriptorBlock : CDescriptorBlock 
    {
        public EFileAttributes FileAttributes;
        public System.DateTime LastModificationDate;
        public System.DateTime CreationDate;
        public System.DateTime BackupDate;
        public System.DateTime LastAccessDate;
        public uint DirectoryID;
        public uint FileID;
        public string FileName;

        public CFileDescriptorBlock(CBackupStream Reader)
        {
            base.ReadData(Reader);
            FileAttributes = (EFileAttributes)Reader.ReadUInt32();
            LastModificationDate = Reader.ReadDate();
            CreationDate = Reader.ReadDate();
            BackupDate = Reader.ReadDate();
            LastAccessDate = Reader.ReadDate();
            DirectoryID = Reader.ReadUInt32();
            FileID = Reader.ReadUInt32();
            FileName = Reader.ReadString(StartPosition, StringType);
            base.ReadStreams(Reader);
        }
    }

    enum ECFilAttributes : uint 
    {
        CFIL_LENGTH_CHANGE_BIT = 0x10000,  // This bit is set if the file size has changed since the file was opened for the write operation. BIT16
        CFIL_UNREADABLE_BLK_BIT = 0x20000, // This bit is set if a hard error was encountered reading the source media (hard disk). This usually indicates that the media itself is bad (i.e. bad sector). BIT17
        CFIL_DEADLOCK_BIT = 0x40000,       // This bit is set if the file was deadlocked. (i.e. On a system supporting record and file locking, it was not possible to get a region of a file unlocked within a watchdog time interval.) BIT18
       // Reserved (set to zero) BIT0 - BIT15, BIT19 - BIT23
       // Vendor Specific BIT24 - BIT31    
    }

    class CCorruptObjectDescriptorBlock : CDescriptorBlock 
    {
        public ECFilAttributes CFilAttributes;
        public ulong Reserved;
        public ulong StreamOffset;
        public ushort CorrupStreamNumber;

        public CCorruptObjectDescriptorBlock(CBackupStream Reader)
        {
            base.ReadData(Reader);
            CFilAttributes = (ECFilAttributes)Reader.ReadUInt32();
            Reserved = Reader.ReadUInt64();
            StreamOffset = Reader.ReadUInt64();
            CorrupStreamNumber = Reader.ReadUInt16();
            base.ReadStreams(Reader);
        }
    }

    class CEndOfPadSetDescriptorBlock : CDescriptorBlock 
    {
        public CEndOfPadSetDescriptorBlock(CBackupStream Reader)
        {
            base.ReadData(Reader);
            base.ReadStreams(Reader);
        }
    }

    class CSoftFilemarkDescriptorBlock : CDescriptorBlock 
    {
        public uint NumberOfFilemarkEntries;
        public uint FilemarkEntriesUsed;
        public uint[] PBAofPreviousFilemarksArray;

        public CSoftFilemarkDescriptorBlock(CBackupStream Reader)
        {
            base.ReadData(Reader);
            NumberOfFilemarkEntries = Reader.ReadUInt32();
            FilemarkEntriesUsed = Reader.ReadUInt32();
            PBAofPreviousFilemarksArray = new uint[FilemarkEntriesUsed];
            for (uint i = 0; i < NumberOfFilemarkEntries; i++)
            {
                uint val = Reader.ReadUInt32();
                if (i < FilemarkEntriesUsed)
                    PBAofPreviousFilemarksArray.SetValue(val, i);
            }
            //base.ReadStreams(Reader);
        }
    }

}
