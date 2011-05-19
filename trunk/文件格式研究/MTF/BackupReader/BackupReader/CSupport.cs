
namespace BackupReader
{
    /// <summary>
    /// Media adresses in the backup file are used as pointers to variable length
    /// data, such as strings.
    /// </summary>
    class CMediaAddress
    {
        public ushort Size;
        public ushort Offset;

        public CMediaAddress()
        {
            Size = 0;
            Offset = 0;
        }
    }

    /// <summary>
    /// Dates are saved as 40 bit packed values in the backup file. An undefined date
    /// is specified by using 0 for all bits. Bits are ordered from most significant to least
    /// significant as follows:
    /// Bits 0-13: Year, 14-17: Month, 18-22: Day, 23-27: Hour, 28-33: Minute, 34-39: Second
    /// </summary>
    class CMediaDate
    {
        public byte byte1;
        public byte byte2;
        public byte byte3;
        public byte byte4;
        public byte byte5;

        public CMediaDate()
        {
            byte1 = 0;
            byte2 = 0;
            byte3 = 0;
            byte4 = 0;
            byte5 = 0;
        }

        public System.DateTime ToDateTime()
        {
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
    }

}
