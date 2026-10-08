using System;
using System.IO;
public static class LegacyDeltaPatch {
    public static byte[] Apply(byte[] original, byte[] patch) {
        using (BinaryReader reader = new BinaryReader(new MemoryStream(patch, false))) {
            if (patch.Length < 16 || reader.ReadUInt32() != 0x3150534c) throw new InvalidDataException("Invalid patch header");
            uint originalSize = reader.ReadUInt32(), targetSize = reader.ReadUInt32(), count = reader.ReadUInt32();
            if (original.LongLength != originalSize || targetSize > 100 * 1024 * 1024) throw new InvalidDataException("Invalid file size");
            byte[] target = new byte[targetSize];
            Array.Copy(original, target, Math.Min(original.Length, target.Length));
            ulong previousEnd = 0;
            for (uint i = 0; i < count; ++i) {
                if (reader.BaseStream.Length - reader.BaseStream.Position < 8) throw new InvalidDataException("Truncated patch header");
                uint offset = reader.ReadUInt32(), length = reader.ReadUInt32();
                if (offset < previousEnd || (ulong)offset + length > targetSize || length > reader.BaseStream.Length - reader.BaseStream.Position) throw new InvalidDataException("Invalid patch chunk");
                byte[] data = reader.ReadBytes((int)length);
                Array.Copy(data, 0, target, offset, length);
                previousEnd = (ulong)offset + length;
            }
            if (reader.BaseStream.Position != reader.BaseStream.Length) throw new InvalidDataException("Trailing patch data");
            return target;
        }
    }
}
