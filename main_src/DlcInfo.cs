using LibOrbisPkg.PKG;
using LibOrbisPkg.Rif;
using System.IO.MemoryMappedFiles;
using System.Security.Cryptography;
using System.Text;

namespace ps4_eboot_dlc_patcher;
public class DlcInfo
{
    public enum DlcType
    {
        PSAL = 0, // additional license? (without extra data)
        PSAC = 4  // additional content  (with extra data)
    }
    public string EntitlementLabel { get; set; }
    public DlcType Type { get; set; }
    public byte[]? EntitlementKey { get; set; }

    public DlcInfo(string entitlementLabel, DlcType type, byte[]? entitlementKey)
    {
        if (string.IsNullOrWhiteSpace(entitlementLabel))
        {
            throw new ArgumentException("Value cannot be null or whitespace.", nameof(entitlementLabel));
        }

        if (entitlementLabel.AsSpan().Trim().Length != 16)
        {
            throw new ArgumentException("Entitlement label must be 16 characters long", nameof(entitlementLabel));
        }

        EntitlementLabel = entitlementLabel;
        Type = type;

        if (entitlementKey is not null && entitlementKey.Length != 16)
        {
            throw new ArgumentException("Entitlement key must be 16 bytes long", nameof(entitlementKey));
        }

        EntitlementKey = entitlementKey ?? new byte[16];
    }

    public string ToEncodedString()
    {
        return $"{EntitlementLabel}-{((byte)Type):X2}-{(EntitlementKey is not null ? BitConverter.ToString(EntitlementKey).Replace("-", "") : "")}";
    }

    /// <summary>
    /// Accepts either label+type+key or label+key
    /// </summary>
    /// <param name="encodedString"></param>
    /// <returns></returns>
    public static DlcInfo FromEncodedString(ReadOnlySpan<char> encodedString)
    {
        if (encodedString.Trim().Length != 16 + 1 + 2 && encodedString.Trim().Length != 16 + 1 + 2 + 1 + 32)
        {
            throw new ArgumentException("Invalid dlc info format");
        }

        var dashCount = encodedString.Count('-');

        if (dashCount != 1 && dashCount != 2)
        {
            throw new ArgumentException("Invalid dlc info format");
        }

        Span<Range> parts = stackalloc Range[3];

        encodedString.Split(parts, '-');

        var label = encodedString[parts[0]].Trim();
        if (label.Length != 16)
        {
            throw new ArgumentException("Invalid entitlement label length");
        }

        foreach (var c in label)
        {
            if (!char.IsLetterOrDigit(c))
            {
                throw new ArgumentException("Invalid character in entitlement label");
            }
        }

        var typeAsInt = int.Parse(encodedString[parts[1]].Trim(), System.Globalization.NumberStyles.HexNumber);

        if (!Enum.IsDefined(typeof(DlcType), typeAsInt))
        {
            throw new ArgumentException("Invalid dlc type");
        }

        DlcType type = (DlcType)typeAsInt;

        if (dashCount == 1)
        {
            return new DlcInfo(label.ToString(), type, null);
        }

        var keyHex = encodedString[parts[2]].Trim();

        if (keyHex.Length != 32)
        {
            throw new ArgumentException("Invalid entitlement key length");
        }

        var key = new byte[16];

        for (int i = 0; i < 16; i++)
        {
            key[i] = byte.Parse(keyHex.Slice(i * 2, 2), System.Globalization.NumberStyles.HexNumber);
        }

        return new DlcInfo(label.ToString(), type, key);
    }

    public static DlcInfo FromDlcPkg(string path)
    {
        if (!File.Exists(path))
        {
            throw new FileNotFoundException("DLC package not found", path);
        }

        using var file = MemoryMappedFile.CreateFromFile(path, FileMode.Open, null, 0, MemoryMappedFileAccess.Read);
        using var fs = file.CreateViewStream(0, 0, MemoryMappedFileAccess.Read);

        var pkg = new PkgReader(fs).ReadPkg();

        if (pkg.Header.content_type != ContentType.AC && pkg.Header.content_type != ContentType.AL)
        {
            throw new Exception("Not dlc pkg");
        }

        var licenseDat = GetLicenseDatByteArrayFromPkg(pkg, file);

        var secret = GetDecryptedSecretFromLicenseDat(licenseDat);
        var entitlementKey = secret[0x70..0x80];

        var contentId = pkg.Header.content_id.Substring(pkg.Header.content_id.LastIndexOf('-') + 1);

        if (contentId.Length != 16)
        {
            throw new Exception("Invalid content id length");
        }

        var type = pkg.Header.content_type == ContentType.AC ? DlcType.PSAC : DlcType.PSAL;

        return new DlcInfo(contentId, type, entitlementKey);
    }

    static byte[] GetLicenseDatByteArrayFromPkg(LibOrbisPkg.PKG.Pkg pkg, MemoryMappedFile pkgMemoryMappedFile)
    {
        var licenseDatMetaEntry = pkg.Metas.Metas.FirstOrDefault(x => x.id == EntryId.LICENSE_DAT);

        if (licenseDatMetaEntry is null)
        {
            throw new Exception("LICENSE.DAT not found in PKG");
        }

        var name = licenseDatMetaEntry.NameTableOffset != 0 ? pkg.EntryNames.GetName(licenseDatMetaEntry.NameTableOffset) : licenseDatMetaEntry.id.ToString();

        var totalEntrySize = licenseDatMetaEntry.Encrypted ? (licenseDatMetaEntry.DataSize + 15) & ~15 : licenseDatMetaEntry.DataSize;

        using var entryStream = pkgMemoryMappedFile.CreateViewStream(licenseDatMetaEntry.DataOffset, totalEntrySize, MemoryMappedFileAccess.Read);

        byte[] tmp = new byte[totalEntrySize];
        entryStream.Read(tmp, 0, tmp.Length);

        if (licenseDatMetaEntry.KeyIndex != 3)
        {
            throw new Exception("LICENSE.DAT key index is not 3");
        }

        tmp = Entry.Decrypt(tmp, pkg, licenseDatMetaEntry);
        return tmp;
    }

    static byte[] GetDecryptedSecretFromLicenseDat(byte[] licenseDatBytes)
    {
        using var ms = new MemoryStream(licenseDatBytes);
        var ldr = new LicenseDatReader(ms);
        var licenseDat = ldr.Read();

        var contentIdBuf = new byte[48];
        Buffer.BlockCopy(Encoding.ASCII.GetBytes(licenseDat.ContentId), 0, contentIdBuf, 0, 36);
        var tmp = SHA256.Create().ComputeHash(contentIdBuf);

        // check if already decrypted
        if (Enumerable.SequenceEqual(licenseDat.Secret.Take(16), tmp[16..32]))
        {
            return licenseDat.Secret;
        }

        licenseDat.DecryptSecretWithDebugKey();

        // check if decryption was successful
        if (!Enumerable.SequenceEqual(licenseDat.Secret.Take(16), tmp[16..32]))
        {
#if DEBUG
            Console.WriteLine(BitConverter.ToString(licenseDat.Secret.Take(16).ToArray()).Replace("-", "").ToUpper());
            Console.WriteLine(BitConverter.ToString(tmp[16..32]).Replace("-", "").ToUpper());
#endif
            throw new Exception("Decrypted secret does not match expected hash");
        }

        return licenseDat.Secret;
    }

}
