#include "Loader.h"
#include "NativeStructs.h"

/*++

Copyright (c) Microsoft Corporation. All rights reserved. 

You may only use this code if you agree to the terms of the Windows Research Kernel Source Code License agreement (see License.txt).
If you do not agree to the terms, do not use the code.


Module Name:

   ldrreloc.c

Abstract:

    This module contains the code to relocate an image when
    the preferred base isn't available. This is called by the
    boot loader, device driver loader, and system loader.

--*/


//
// Mark a HIGHADJ entry as needing an increment if reprocessing.
//
#define LDRP_RELOCATION_INCREMENT   0x1

//
// Mark a HIGHADJ entry as not suitable for reprocessing.
//
#define LDRP_RELOCATION_FINAL       0x2


/*
#pragma alloc_text(PAGE,LdrRelocateImage)
#pragma alloc_text(PAGE,LdrRelocateImageWithBias)
#pragma alloc_text(PAGE,LdrProcessRelocationBlock)
#pragma alloc_text(PAGE,LdrProcessRelocationBlockLongLong)*/

typedef NTSTATUS LDR_RELOCATE_IMAGE_RETURN_TYPE;

LDR_RELOCATE_IMAGE_RETURN_TYPE
LdrRelocateImage (
    __in PVOID NewBase,
    __in LDR_RELOCATE_IMAGE_RETURN_TYPE Success,
    __in LDR_RELOCATE_IMAGE_RETURN_TYPE Conflict,
    __in LDR_RELOCATE_IMAGE_RETURN_TYPE Invalid
    )
/*++

Routine Description:

    This routine relocates an image file that was not loaded into memory
    at the preferred address.

Arguments:

    NewBase - Supplies a pointer to the image base.

    Success - Value to return if relocation successful.

    Conflict - Value to return if can't relocate.

    Invalid - Value to return if relocations are invalid.

Return Value:

    Success if image is relocated.
    Conflict if image can't be relocated.
    Invalid if image contains invalid fixups.

--*/

{
    //
    // Just call LdrRelocateImageWithBias() with a zero bias.
    //
    return LdrRelocateImageWithBias( NewBase, 0, Success, Conflict, Invalid );
}


LDR_RELOCATE_IMAGE_RETURN_TYPE
LdrRelocateImageWithBias (
    __in PVOID NewBase,
    __in LONGLONG AdditionalBias,
    __in LDR_RELOCATE_IMAGE_RETURN_TYPE Success,
    __in LDR_RELOCATE_IMAGE_RETURN_TYPE Conflict,
    __in LDR_RELOCATE_IMAGE_RETURN_TYPE Invalid
    )
/*++

Routine Description:

    This routine relocates an image file that was not loaded into memory
    at the preferred address.

Arguments:

    NewBase - Supplies a pointer to the image base.

    AdditionalBias - An additional quantity to add to all fixups.  The
                     32-bit X86 loader uses this when loading 64-bit images
                     to specify a NewBase that is actually a 64-bit value.

    Success - Value to return if relocation successful.

    Conflict - Value to return if can't relocate.

    Invalid - Value to return if relocations are invalid.

Return Value:

    Success if image is relocated.
    Conflict if image can't be relocated.
    Invalid if image contains invalid fixups.

--*/

{
    LONGLONG Diff;
    ULONG TotalCountBytes = 0;
    ULONG_PTR VA;
    ULONGLONG OldBase;
    ULONG SizeOfBlock;
    PUSHORT NextOffset = NULL;
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_BASE_RELOCATION NextBlock;
    LDR_RELOCATE_IMAGE_RETURN_TYPE Status;

    NtHeaders = RtlImageNtHeader( NewBase );
    if (NtHeaders == NULL) {
        Status = Invalid;
        goto Exit;
    }

    switch (NtHeaders->OptionalHeader.Magic) {
       
        case IMAGE_NT_OPTIONAL_HDR32_MAGIC:

            OldBase =
                ((PIMAGE_NT_HEADERS32)NtHeaders)->OptionalHeader.ImageBase;
            break;

        case IMAGE_NT_OPTIONAL_HDR64_MAGIC:

            OldBase =
                ((PIMAGE_NT_HEADERS64)NtHeaders)->OptionalHeader.ImageBase;
            break;

        default:

            Status = Invalid;
            goto Exit;
    }

    //
    // Locate the relocation section.
    //

    NextBlock = (PIMAGE_BASE_RELOCATION)RtlImageDirectoryEntryToData(
            NewBase, TRUE, IMAGE_DIRECTORY_ENTRY_BASERELOC, &TotalCountBytes);

    //
    // It is possible for a file to have no relocations, but the relocations
    // must not have been stripped.
    //

    if (!NextBlock || !TotalCountBytes) 
    {
        Status = (NtHeaders->FileHeader.Characteristics & IMAGE_FILE_RELOCS_STRIPPED) ? Conflict : Success;
        goto Exit;
    }

    //
    // If the image has a relocation table, then apply the specified fixup
    // information to the image.
    //
    Diff = (ULONG_PTR)NewBase - OldBase + AdditionalBias;
    while (TotalCountBytes) 
    {
        SizeOfBlock = NextBlock->SizeOfBlock;

        // Prevent crash
        if (SizeOfBlock == 0)
        {
            Status = Invalid;
            goto Exit;
        }

        TotalCountBytes -= SizeOfBlock;
        SizeOfBlock -= sizeof(IMAGE_BASE_RELOCATION);
        SizeOfBlock /= sizeof(USHORT);
        NextOffset = (PUSHORT)((PCHAR)NextBlock + sizeof(IMAGE_BASE_RELOCATION));

        VA = (ULONG_PTR)NewBase + NextBlock->VirtualAddress;
        NextBlock = LdrProcessRelocationBlockLongLong( VA, SizeOfBlock, NextOffset, Diff );

        if (!NextBlock)
        {
            Status = Invalid;
            goto Exit;
        }
    }

    Status = Success;
Exit:
    return Status;
}

PIMAGE_BASE_RELOCATION
LdrProcessRelocationBlock(
    IN ULONG_PTR VA,
    IN ULONG SizeOfBlock,
    IN PUSHORT NextOffset,
    IN LONG_PTR Diff
    )
{
    PIMAGE_BASE_RELOCATION baseRelocation;

    baseRelocation = LdrProcessRelocationBlockLongLong( VA, SizeOfBlock, NextOffset, (LONGLONG)Diff );

    return baseRelocation;
}

// begin_rebase
PIMAGE_BASE_RELOCATION
LdrProcessRelocationBlockLongLong(
    IN ULONG_PTR VA,
    IN ULONG SizeOfBlock,
    IN PUSHORT NextOffset,
    IN LONGLONG Diff
    )
{
    PUCHAR FixupVA;
    USHORT Offset;
    LONG Temp;
    //ULONG Temp32;
    ULONGLONG Value64;
    //LONGLONG Temp64;

    while (SizeOfBlock--) {

       Offset = *NextOffset & (USHORT)0xfff;
       FixupVA = (PUCHAR)(VA + Offset);

       //
       // Apply the fixups.
       //

       switch ((*NextOffset) >> 12) {

            case IMAGE_REL_BASED_HIGHLOW :
                //
                // HighLow - (32-bits) relocate the high and low half
                //      of an address.
                //
                *(LONG UNALIGNED *)FixupVA += (ULONG) Diff;
                break;

            case IMAGE_REL_BASED_HIGH :
                //
                // High - (16-bits) relocate the high half of an address.
                //
                Temp = *(PUSHORT)FixupVA << 16;
                Temp += (ULONG) Diff;
                *(PUSHORT)FixupVA = (USHORT)(Temp >> 16);
                break;

            case IMAGE_REL_BASED_HIGHADJ :
                //
                // Adjust high - (16-bits) relocate the high half of an
                //      address and adjust for sign extension of low half.
                //

                //
                // If the address has already been relocated then don't
                // process it again now or information will be lost.
                //
                if (Offset & LDRP_RELOCATION_FINAL) {
                    ++NextOffset;
                    --SizeOfBlock;
                    break;
                }

                Temp = *(PUSHORT)FixupVA << 16;
                ++NextOffset;
                --SizeOfBlock;
                Temp += (LONG)(*(PSHORT)NextOffset);
                Temp += (ULONG) Diff;
                Temp += 0x8000;
                *(PUSHORT)FixupVA = (USHORT)(Temp >> 16);

                break;

            case IMAGE_REL_BASED_LOW :
                //
                // Low - (16-bit) relocate the low half of an address.
                //
                Temp = *(PSHORT)FixupVA;
                Temp += (ULONG) Diff;
                *(PUSHORT)FixupVA = (USHORT)Temp;
                break;

            case IMAGE_REL_BASED_IA64_IMM64:

                //
                // Align it to bundle address before fixing up the
                // 64-bit immediate value of the movl instruction.
                //

                FixupVA = (PUCHAR)((ULONG_PTR)FixupVA & ~(15));
                Value64 = (ULONGLONG)0;

                //
                // Extract the lower 32 bits of IMM64 from bundle
                //

                /*
                EXT_IMM64(Value64,
                        (PULONG)FixupVA + EMARCH_ENC_I17_IMM7B_INST_WORD_X,
                        EMARCH_ENC_I17_IMM7B_SIZE_X,
                        EMARCH_ENC_I17_IMM7B_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IMM7B_VAL_POS_X);
                EXT_IMM64(Value64,
                        (PULONG)FixupVA + EMARCH_ENC_I17_IMM9D_INST_WORD_X,
                        EMARCH_ENC_I17_IMM9D_SIZE_X,
                        EMARCH_ENC_I17_IMM9D_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IMM9D_VAL_POS_X);
                EXT_IMM64(Value64,
                        (PULONG)FixupVA + EMARCH_ENC_I17_IMM5C_INST_WORD_X,
                        EMARCH_ENC_I17_IMM5C_SIZE_X,
                        EMARCH_ENC_I17_IMM5C_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IMM5C_VAL_POS_X);
                EXT_IMM64(Value64,
                        (PULONG)FixupVA + EMARCH_ENC_I17_IC_INST_WORD_X,
                        EMARCH_ENC_I17_IC_SIZE_X,
                        EMARCH_ENC_I17_IC_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IC_VAL_POS_X);
                EXT_IMM64(Value64,
                        (PULONG)FixupVA + EMARCH_ENC_I17_IMM41a_INST_WORD_X,
                        EMARCH_ENC_I17_IMM41a_SIZE_X,
                        EMARCH_ENC_I17_IMM41a_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IMM41a_VAL_POS_X);

                EXT_IMM64(Value64,
                        ((PULONG)FixupVA + EMARCH_ENC_I17_IMM41b_INST_WORD_X),
                        EMARCH_ENC_I17_IMM41b_SIZE_X,
                        EMARCH_ENC_I17_IMM41b_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IMM41b_VAL_POS_X);
                EXT_IMM64(Value64,
                        ((PULONG)FixupVA + EMARCH_ENC_I17_IMM41c_INST_WORD_X),
                        EMARCH_ENC_I17_IMM41c_SIZE_X,
                        EMARCH_ENC_I17_IMM41c_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IMM41c_VAL_POS_X);
                EXT_IMM64(Value64,
                        ((PULONG)FixupVA + EMARCH_ENC_I17_SIGN_INST_WORD_X),
                        EMARCH_ENC_I17_SIGN_SIZE_X,
                        EMARCH_ENC_I17_SIGN_INST_WORD_POS_X,
                        EMARCH_ENC_I17_SIGN_VAL_POS_X);
                //
                // Update 64-bit address
                //

                Value64+=Diff;

                //
                // Insert IMM64 into bundle
                //

                INS_IMM64(Value64,
                        ((PULONG)FixupVA + EMARCH_ENC_I17_IMM7B_INST_WORD_X),
                        EMARCH_ENC_I17_IMM7B_SIZE_X,
                        EMARCH_ENC_I17_IMM7B_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IMM7B_VAL_POS_X);
                INS_IMM64(Value64,
                        ((PULONG)FixupVA + EMARCH_ENC_I17_IMM9D_INST_WORD_X),
                        EMARCH_ENC_I17_IMM9D_SIZE_X,
                        EMARCH_ENC_I17_IMM9D_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IMM9D_VAL_POS_X);
                INS_IMM64(Value64,
                        ((PULONG)FixupVA + EMARCH_ENC_I17_IMM5C_INST_WORD_X),
                        EMARCH_ENC_I17_IMM5C_SIZE_X,
                        EMARCH_ENC_I17_IMM5C_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IMM5C_VAL_POS_X);
                INS_IMM64(Value64,
                        ((PULONG)FixupVA + EMARCH_ENC_I17_IC_INST_WORD_X),
                        EMARCH_ENC_I17_IC_SIZE_X,
                        EMARCH_ENC_I17_IC_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IC_VAL_POS_X);
                INS_IMM64(Value64,
                        ((PULONG)FixupVA + EMARCH_ENC_I17_IMM41a_INST_WORD_X),
                        EMARCH_ENC_I17_IMM41a_SIZE_X,
                        EMARCH_ENC_I17_IMM41a_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IMM41a_VAL_POS_X);
                INS_IMM64(Value64,
                        ((PULONG)FixupVA + EMARCH_ENC_I17_IMM41b_INST_WORD_X),
                        EMARCH_ENC_I17_IMM41b_SIZE_X,
                        EMARCH_ENC_I17_IMM41b_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IMM41b_VAL_POS_X);
                INS_IMM64(Value64,
                        ((PULONG)FixupVA + EMARCH_ENC_I17_IMM41c_INST_WORD_X),
                        EMARCH_ENC_I17_IMM41c_SIZE_X,
                        EMARCH_ENC_I17_IMM41c_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IMM41c_VAL_POS_X);
                INS_IMM64(Value64,
                        ((PULONG)FixupVA + EMARCH_ENC_I17_SIGN_INST_WORD_X),
                        EMARCH_ENC_I17_SIGN_SIZE_X,
                        EMARCH_ENC_I17_SIGN_INST_WORD_POS_X,
                        EMARCH_ENC_I17_SIGN_VAL_POS_X);
                        */
                break;

            case IMAGE_REL_BASED_DIR64:

                *(ULONGLONG UNALIGNED *)FixupVA += Diff;

                break;

            case IMAGE_REL_BASED_MIPS_JMPADDR :
                //
                // JumpAddress - (32-bits) relocate a MIPS jump address.
                //
                Temp = (*(PULONG)FixupVA & 0x3ffffff) << 2;
                Temp += (ULONG) Diff;
                *(PULONG)FixupVA = (*(PULONG)FixupVA & ~0x3ffffff) |
                                                ((Temp >> 2) & 0x3ffffff);

                break;

            case IMAGE_REL_BASED_ABSOLUTE :
                //
                // Absolute - no fixup required.
                //
                break;

            case IMAGE_REL_BASED_SECTION :
                //
                // Section Relative reloc.  Ignore for now.
                //
                break;

            case IMAGE_REL_BASED_REL32 :
                //
                // Relative intrasection. Ignore for now.
                //
                break;

            default :
                //
                // Illegal - illegal relocation type.
                //

                return (PIMAGE_BASE_RELOCATION)NULL;
       }
       ++NextOffset;
    }
    return (PIMAGE_BASE_RELOCATION)NextOffset;
}

// end_rebase

