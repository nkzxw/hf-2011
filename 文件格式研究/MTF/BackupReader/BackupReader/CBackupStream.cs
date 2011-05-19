using System;
using System.Collections.Generic;
using System.Text;

namespace BackupReader
{
    /// <summary>
    /// Represents a binary stream for reading a backup file.
    /// This class is derived from System.IO.BinaryReader.
    /// </summary>
    class CBackupStream : System.IO.BinaryReader 
    {
        /// <summary>
        /// Reads a descriptor block from the stream.
        /// </summary>
        public CDescriptorBlock ReadDBLK()
        {
            // Read block type
            EBlockType bt = PeekNextBlockType();

            switch (bt)
            {
                case EBlockType.MTF_TAPE: // TAPE descriptor block
                    return new CTapeHeaderDescriptorBlock(this);
                case EBlockType.MTF_SSET: // Start of data SET descriptor block
                    return new CStartOfDataSetDescriptorBlock(this);
                case EBlockType.MTF_VOLB: // VOLume descriptor Block
                    return new CVolumeDescriptorBlock(this);
                case EBlockType.MTF_DIRB: // DIRectory descriptor Block
                    return new CDirectoryDescriptorBlock(this);
                case EBlockType.MTF_FILE: // FILE descriptor block
                    return new CFileDescriptorBlock(this);
                case EBlockType.MTF_CFIL: // Corrupt object descriptor block
                    return new CCorruptObjectDescriptorBlock(this);
                case EBlockType.MTF_ESPB: // End of Set Pad descriptor Block
                    return new CEndOfPadSetDescriptorBlock(this);
                case EBlockType.MTF_ESET: // End of SET descriptor block
                    return new CEndOfDataSetDescriptorBlock(this);
                case EBlockType.MTF_EOTM: // End Of Tape Marker descriptor block
                    return new CEndOfTapeMarkerDescriptorBlock(this);
                case EBlockType.MTF_SFMB: // Soft FileMark descriptor Block
                    return new CSoftFilemarkDescriptorBlock(this);
            }

            return null;
        }

        /// <summary>
        /// Reads a stream header from the stream.
        /// </summary>
        public CDataStream.CStreamHeader ReadStreamHeader()
        {
            return new CDataStream.CStreamHeader(this);
        }

        /// <summary>
        /// Reads the type of the next desciptor block. Does not advance stream position.
        /// </summary>
        public EBlockType PeekNextBlockType()
        {
            // Check for EOF
            if (BaseStream.Position + 4 >= BaseStream.Length)
                return 0;

            EBlockType et = (EBlockType)ReadUInt32();
            BaseStream.Seek(-4, System.IO.SeekOrigin.Current);
            return et;
        }

        /// <summary>
        /// Reads the type of the next data stream.
        /// </summary>
        public string GetNextDataStreamType()
        {
            // Check for EOF
            if (BaseStream.Position + 4 >= BaseStream.Length)
                return "";

            return ReadFixedSizeString(4, EStringType.ANSI);
        }

        /// <summary>
        /// Reads OS specific data. It is assumed that a MTF_MEDIA_ADDRESS structure is next in the stream.
        /// </summary>
        public COSSpecificData ReadOsSpecificData(long StartPosition, EOSID OSID, byte OSVersion, EBlockType Type)
        {
            long oldpos = BaseStream.Position;
            ushort sz = ReadUInt16();
            ushort off = ReadUInt16();
            BaseStream.Seek(StartPosition + off, System.IO.SeekOrigin.Begin);

            if ((OSID == EOSID.NetWare) && (OSVersion == 0))
            {
                if (Type == EBlockType.MTF_DIRB) return new CNetwareDirB(this);
                if (Type == EBlockType.MTF_FILE) return new CNetwareFile(this);
            }
            if ((OSID == EOSID.NetWare_SMS) && (OSVersion == 1))
            {
                if (Type == EBlockType.MTF_DIRB) return new CNetwareSMSDirB(this);
                if (Type == EBlockType.MTF_FILE) return new CNetwareSMSFile(this);
            }
            if ((OSID == EOSID.NetWare_SMS) && (OSVersion == 2))
            {
                if (Type == EBlockType.MTF_DIRB) return new CNetwareSMEDirB(this);
                if (Type == EBlockType.MTF_FILE) return new CNetwareSMEFile(this);
            }
            if ((OSID == EOSID.Windows_NT) && (OSVersion == 0))
            {
                if (Type == EBlockType.MTF_DIRB) return new CWindowsNT0DirB(this);
                if (Type == EBlockType.MTF_FILE) return new CWindowsNT0File(this);
            }
            if ((OSID == EOSID.Windows_NT) && (OSVersion == 1))
            {
                if (Type == EBlockType.MTF_VOLB) return new CWindowsNT1VolB(this);
                if (Type == EBlockType.MTF_DIRB) return new CWindowsNT1DirB(this);
                if (Type == EBlockType.MTF_FILE) return new CWindowsNT1File(this);
            }
            if ((OSID == EOSID.OS_2) && (OSVersion == 0))
            {
                if (Type == EBlockType.MTF_DIRB) return new COS2DirB(this);
                if (Type == EBlockType.MTF_FILE) return new COS2File(this);
            }
            if ((OSID == EOSID.Windows_95) && (OSVersion == 0))
            {
                if (Type == EBlockType.MTF_DIRB) return new CWindows95DirB(this);
                if (Type == EBlockType.MTF_FILE) return new CWindows95File(this);
            }
            if ((OSID == EOSID.Macintosh) && (OSVersion == 0))
            {
                if (Type == EBlockType.MTF_VOLB) return new CMacintoshVolB(this);
                if (Type == EBlockType.MTF_DIRB) return new CMacintoshDirB(this);
                if (Type == EBlockType.MTF_FILE) return new CMacintoshFile(this);
            }

            BaseStream.Seek(oldpos + 4, System.IO.SeekOrigin.Begin);

            return new COSSpecificData();
        }

        /// <summary>
        /// Reads a string. It is assumed that a MTF_MEDIA_ADDRESS structure is next in the stream.
        /// </summary>
        public string ReadString(long StartPosition, EStringType Type)
        {
            long oldpos = BaseStream.Position;

            ushort sz = ReadUInt16();
            long off = StartPosition + ReadUInt16();
            BaseStream.Seek(off, System.IO.SeekOrigin.Begin);
            string str;
            if (Type == EStringType.ANSI)
            {
                byte[] bytes = ReadBytes(sz);
                System.Text.ASCIIEncoding encoding = new System.Text.ASCIIEncoding();
                str = encoding.GetString(bytes);
            }
            else if (Type == EStringType.Unicode)
            {
                byte[] bytes = ReadBytes(sz);
                System.Text.UnicodeEncoding encoding = new System.Text.UnicodeEncoding();
                str = encoding.GetString(bytes);
            }
            else
            {
                str = "";
            }

            BaseStream.Seek(oldpos + 4, System.IO.SeekOrigin.Begin);

            return str;
        }

        /// <summary>
        /// Reads a string. It is assumed that string bytes is next in the stream.
        /// </summary>
        public string ReadFixedSizeString(int Size, EStringType Type)
        {
            string str;
            if (Type == EStringType.ANSI)
            {
                byte[] bytes = ReadBytes(Size);
                System.Text.ASCIIEncoding encoding = new System.Text.ASCIIEncoding();
                str = encoding.GetString(bytes);
            }
            else if (Type == EStringType.Unicode)
            {
                byte[] bytes = ReadBytes(Size);
                System.Text.UnicodeEncoding encoding = new System.Text.UnicodeEncoding();
                str = encoding.GetString(bytes);
            }
            else
            {
                str = "";
            }

            return str;
        }

        /// <summary>
        /// Reads a date time. It is assumed that a 5 byte MTF_MEDIA_DATE structure is next in the stream.
        /// </summary>
        public System.DateTime ReadDate()
        {
            byte byte1 = ReadByte();
            byte byte2 = ReadByte();
            byte byte3 = ReadByte();
            byte byte4 = ReadByte();
            byte byte5 = ReadByte();

            if ((byte1 == 0) && (byte2 == 0) && (byte3 == 0) && (byte4 == 0) && (byte5 == 0))
                return new System.DateTime();

            int year = (byte1 << 6) + (byte2 >> 2);
            int month = ((byte2 & 0x3) << 2) + (byte3 >> 6);
            int day = ((byte3 & 0x3E) >> 1);
            int hour = ((byte3 & 0x1) << 4) + (byte4 >> 4);
            int minute = ((byte4 & 0xF) << 2) + (byte5 >> 6);
            int second = (byte5 & 0x3F);
            return new System.DateTime(year, month, day, hour, minute, second);
        }

        public CBackupStream(string Filename)
            : base (new System.IO.FileStream(Filename, System.IO.FileMode.Open, System.IO.FileAccess.Read))
        {

        }

        ~CBackupStream()
        {
            Close();
        }

    }
}
