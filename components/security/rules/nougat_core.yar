// Elderred Softworks / Nougat Media Suite test-and-sanity rule set.
// These rules are intentionally conservative. Additional user rules may be
// placed in ~/.config/nougat-media-suite/security/rules/.
rule Nougat_EICAR_Antivirus_Test_File
{
    meta:
        description = "Detect the harmless industry-standard EICAR AV test string"
        author = "Elderred Softworks"
        severity = "test"
    strings:
        $eicar = "X5O!P%@AP[4\\PZX54(P^)7CC)7}$EICAR-STANDARD-ANTIVIRUS-TEST-FILE!$H+H*" ascii
    condition:
        $eicar
}
