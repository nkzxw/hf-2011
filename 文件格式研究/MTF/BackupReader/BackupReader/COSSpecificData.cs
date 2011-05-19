using System;

namespace BackupReader
{
    /// <summary>
    /// Represents data specific to an operating system.
    /// </summary>
    class COSSpecificData
    {
    }

    class CNetwareDirB : COSSpecificData
    {
        UInt32 OwnerID;
        UInt32 DirectoryAttributes;
        UInt32 MaximumSpace;
        UInt16 InheritedRights;

        public CNetwareDirB(CBackupStream Reader)
        {
            OwnerID = Reader.ReadUInt32();
            DirectoryAttributes = Reader.ReadUInt32();
            MaximumSpace = Reader.ReadUInt32();
            InheritedRights = Reader.ReadUInt16();
        }
    }

    class CNetwareFile : COSSpecificData
    {
        UInt32 OwnerID;
        UInt32 FileAttributes;
        UInt32 LastModiferID;
        UInt32 ArchiverID;
        UInt16 InheritedRights;

        public CNetwareFile(CBackupStream Reader)
        {
            OwnerID = Reader.ReadUInt32();
            FileAttributes = Reader.ReadUInt32();
            LastModiferID = Reader.ReadUInt32();
            ArchiverID = Reader.ReadUInt32();
            InheritedRights = Reader.ReadUInt16();
        }
    }

    class CNetwareSMSDirB : COSSpecificData
    {
        UInt32 DirectoryAttributes;
        Int16 Modified;
        UInt32 CreatorNameSpace;
        byte[] Volume; // 17 bytes

        public CNetwareSMSDirB(CBackupStream Reader)
        {
            DirectoryAttributes = Reader.ReadUInt32();
            Modified = Reader.ReadInt16();
            CreatorNameSpace = Reader.ReadUInt32();
            Volume = Reader.ReadBytes(17);
        }
    }

    class CNetwareSMSFile : COSSpecificData
    {
        UInt32 FileAttributes;
        Int16 Modified;
        UInt32 CreatorNameSpace;
        byte[] Volume; // 17 bytes

        public CNetwareSMSFile(CBackupStream Reader)
        {
            FileAttributes = Reader.ReadUInt32();
            Modified = Reader.ReadInt16();
            CreatorNameSpace = Reader.ReadUInt32();
            Volume = Reader.ReadBytes(17);
        }
    }

    class CNetwareSMEDirB : COSSpecificData
    {
        UInt32 DirectoryAttributes;
        UInt32 CreatorNameSpace;
        byte[] Volume; // 18 bytes
        Int16 Modified;

        public CNetwareSMEDirB(CBackupStream Reader)
        {
            DirectoryAttributes = Reader.ReadUInt32();
            CreatorNameSpace = Reader.ReadUInt32();
            Volume = Reader.ReadBytes(18);
            Modified = Reader.ReadInt16();
        }
    }

    class CNetwareSMEFile : COSSpecificData
    {
        UInt32 FileAttributes;
        UInt32 CreatorNameSpace;
        byte[] Volume; // 18 bytes
        Int16 Modified;

        public CNetwareSMEFile(CBackupStream Reader)
        {
            FileAttributes = Reader.ReadUInt32();
            CreatorNameSpace = Reader.ReadUInt32();
            Volume = Reader.ReadBytes(18);
            Modified = Reader.ReadInt16();
        }
    }

    class CWindowsNT0DirB : COSSpecificData
    {
        UInt32 DirectoryAttributes;

        public CWindowsNT0DirB(CBackupStream Reader)
        {
            DirectoryAttributes = Reader.ReadUInt32();
        }
    }

    class CWindowsNT0File : COSSpecificData
    {
        UInt32 FileAttributes;
        UInt16 ShortNameOffset;
        UInt16 ShortNameSize;
        Int16 IsLink;
        UInt16 Reserved;

        public CWindowsNT0File(CBackupStream Reader)
        {
            FileAttributes = Reader.ReadUInt32();
            ShortNameOffset = Reader.ReadUInt16();
            ShortNameSize = Reader.ReadUInt16();
            IsLink = Reader.ReadInt16();
            Reserved = Reader.ReadUInt16();
        }
    }

    class CWindowsNT1VolB : COSSpecificData
    {
        UInt32 FileSystemFlags;
        UInt32 NTBackupSetAttributes;

        public CWindowsNT1VolB(CBackupStream Reader)
        {
            FileSystemFlags = Reader.ReadUInt32();
            NTBackupSetAttributes = Reader.ReadUInt32();
        }
    }

    class CWindowsNT1DirB : COSSpecificData
    {
        UInt32 DirectoryAttributes;
        UInt16 ShortNameOffset;
        UInt16 ShortNameSize;

        public CWindowsNT1DirB(CBackupStream Reader)
        {
            DirectoryAttributes = Reader.ReadUInt32();
            ShortNameOffset = Reader.ReadUInt16();
            ShortNameSize = Reader.ReadUInt16();
        }
    }

    class CWindowsNT1File : COSSpecificData
    {
        UInt32 FileAttributes;
        UInt16 ShortNameOffset;
        UInt16 ShortNameSize;
        UInt32 NTFileFlags;

        public CWindowsNT1File(CBackupStream Reader)
        {
            FileAttributes = Reader.ReadUInt32();
            ShortNameOffset = Reader.ReadUInt16();
            ShortNameSize = Reader.ReadUInt16();
            NTFileFlags = Reader.ReadUInt32();
        }
    }

    class COS2DirB : COSSpecificData
    {
        UInt32 DirectoryAttributes;

        public COS2DirB(CBackupStream Reader)
        {
            DirectoryAttributes = Reader.ReadUInt32();
        }
    }

    class COS2File : COSSpecificData
    {
        UInt32 FileAttributes;

        public COS2File(CBackupStream Reader)
        {
            FileAttributes = Reader.ReadUInt32();
        }
    }

    class CWindows95DirB : COSSpecificData
    {
        UInt32 DirectoryAttributes;
        UInt16 ShortNameOffset;
        UInt16 ShortNameSize;

        public CWindows95DirB(CBackupStream Reader)
        {
            DirectoryAttributes = Reader.ReadUInt32();
            ShortNameOffset = Reader.ReadUInt16();
            ShortNameSize = Reader.ReadUInt16();
        }
    }

    class CWindows95File : COSSpecificData
    {
        UInt32 FileAttributes;
        UInt16 ShortNameOffset;
        UInt16 ShortNameSize;

        public CWindows95File(CBackupStream Reader)
        {
            FileAttributes = Reader.ReadUInt32();
            ShortNameOffset = Reader.ReadUInt16();
            ShortNameSize = Reader.ReadUInt16();
        }
    }

    class CMacintoshVolB : COSSpecificData
    {
        UInt32 VolumeParmsAttributes;
        UInt16 VolumeAttributes;
        UInt16 VolumeSignature;
        UInt16 DriveNumber;
        UInt16 DriverRefNumber;
        UInt16 FileSystemID;
        System.DateTime CreatorDate; // MTF_DATE_TIME 5 bytes
        System.DateTime ModificationDate; // MTF_DATE_TIME 5 bytes
        byte [] VolumeFinderInfo; // 32 bytes

        public CMacintoshVolB(CBackupStream Reader)
        {
            VolumeParmsAttributes = Reader.ReadUInt32();
            VolumeAttributes = Reader.ReadUInt16();
            VolumeSignature = Reader.ReadUInt16();
            DriveNumber = Reader.ReadUInt16();
            DriverRefNumber = Reader.ReadUInt16();
            FileSystemID = Reader.ReadUInt16();
            CreatorDate = Reader.ReadDate();
            ModificationDate = Reader.ReadDate();
            VolumeFinderInfo = Reader.ReadBytes(32);
        }
    }

    class CMacintoshDirB : COSSpecificData
    {
        byte[] FinderInfo; // 16 bytes
        byte [] AdditionalFinderInfo; // 16 bytes
        UInt32 DirectoryID;
        UInt16 DirectoryInfo;
        byte DirectoryXInfo;
        byte DirectoryAttributes;

        public CMacintoshDirB(CBackupStream Reader)
        {
            FinderInfo = Reader.ReadBytes(16);
            AdditionalFinderInfo = Reader.ReadBytes(16);
            DirectoryID = Reader.ReadUInt32();
            DirectoryInfo = Reader.ReadUInt16();
            DirectoryXInfo = Reader.ReadByte();
            DirectoryAttributes = Reader.ReadByte();
        }
    }

    class CMacintoshFile : COSSpecificData
    {
        byte[] FinderInfo; // 16 bytes
        byte[] AdditionalFinderInfo; // 16 bytes
        UInt32 FileID;
        UInt32 FileType;
        UInt32 FileCreator;
        UInt16 FileInfo;
        byte FileXInfo;
        byte FileAttributes;

        public CMacintoshFile(CBackupStream Reader)
        {
            FinderInfo = Reader.ReadBytes(16);
            AdditionalFinderInfo = Reader.ReadBytes(16);
            FileID = Reader.ReadUInt32();
            FileType = Reader.ReadUInt32();
            FileCreator = Reader.ReadUInt32();
            FileInfo = Reader.ReadUInt16();
            FileXInfo = Reader.ReadByte();
            FileAttributes = Reader.ReadByte();
        }
    }
}
