/* $XFree86: xc/programs/Xserver/hw/xfree86/xaa/xaaTables.c,v 1.1.2.1 1998/07/12 09:43:04 dawes Exp $ */

/*
 * This is a table of 24-bit values, indexed with an 8-bit byte value, then
 * expands each bit to three consecutive bits. This is required for color
 * expansion in 24bpp mode with the coprocessor in 8bpp mode, with LSB-first
 * bit order within a byte.
 */

unsigned int byte_expand3[256] =
{
    0x000000, 0x000007, 0x000038, 0x00003F, 0x0001C0, 0x0001C7, 0x0001F8, 0x0001FF,
    0x000E00, 0x000E07, 0x000E38, 0x000E3F, 0x000FC0, 0x000FC7, 0x000FF8, 0x000FFF,
    0x007000, 0x007007, 0x007038, 0x00703F, 0x0071C0, 0x0071C7, 0x0071F8, 0x0071FF,
    0x007E00, 0x007E07, 0x007E38, 0x007E3F, 0x007FC0, 0x007FC7, 0x007FF8, 0x007FFF,
    0x038000, 0x038007, 0x038038, 0x03803F, 0x0381C0, 0x0381C7, 0x0381F8, 0x0381FF,
    0x038E00, 0x038E07, 0x038E38, 0x038E3F, 0x038FC0, 0x038FC7, 0x038FF8, 0x038FFF,
    0x03F000, 0x03F007, 0x03F038, 0x03F03F, 0x03F1C0, 0x03F1C7, 0x03F1F8, 0x03F1FF,
    0x03FE00, 0x03FE07, 0x03FE38, 0x03FE3F, 0x03FFC0, 0x03FFC7, 0x03FFF8, 0x03FFFF,
    0x1C0000, 0x1C0007, 0x1C0038, 0x1C003F, 0x1C01C0, 0x1C01C7, 0x1C01F8, 0x1C01FF,
    0x1C0E00, 0x1C0E07, 0x1C0E38, 0x1C0E3F, 0x1C0FC0, 0x1C0FC7, 0x1C0FF8, 0x1C0FFF,
    0x1C7000, 0x1C7007, 0x1C7038, 0x1C703F, 0x1C71C0, 0x1C71C7, 0x1C71F8, 0x1C71FF,
    0x1C7E00, 0x1C7E07, 0x1C7E38, 0x1C7E3F, 0x1C7FC0, 0x1C7FC7, 0x1C7FF8, 0x1C7FFF,
    0x1F8000, 0x1F8007, 0x1F8038, 0x1F803F, 0x1F81C0, 0x1F81C7, 0x1F81F8, 0x1F81FF,
    0x1F8E00, 0x1F8E07, 0x1F8E38, 0x1F8E3F, 0x1F8FC0, 0x1F8FC7, 0x1F8FF8, 0x1F8FFF,
    0x1FF000, 0x1FF007, 0x1FF038, 0x1FF03F, 0x1FF1C0, 0x1FF1C7, 0x1FF1F8, 0x1FF1FF,
    0x1FFE00, 0x1FFE07, 0x1FFE38, 0x1FFE3F, 0x1FFFC0, 0x1FFFC7, 0x1FFFF8, 0x1FFFFF,
    0xE00000, 0xE00007, 0xE00038, 0xE0003F, 0xE001C0, 0xE001C7, 0xE001F8, 0xE001FF,
    0xE00E00, 0xE00E07, 0xE00E38, 0xE00E3F, 0xE00FC0, 0xE00FC7, 0xE00FF8, 0xE00FFF,
    0xE07000, 0xE07007, 0xE07038, 0xE0703F, 0xE071C0, 0xE071C7, 0xE071F8, 0xE071FF,
    0xE07E00, 0xE07E07, 0xE07E38, 0xE07E3F, 0xE07FC0, 0xE07FC7, 0xE07FF8, 0xE07FFF,
    0xE38000, 0xE38007, 0xE38038, 0xE3803F, 0xE381C0, 0xE381C7, 0xE381F8, 0xE381FF,
    0xE38E00, 0xE38E07, 0xE38E38, 0xE38E3F, 0xE38FC0, 0xE38FC7, 0xE38FF8, 0xE38FFF,
    0xE3F000, 0xE3F007, 0xE3F038, 0xE3F03F, 0xE3F1C0, 0xE3F1C7, 0xE3F1F8, 0xE3F1FF,
    0xE3FE00, 0xE3FE07, 0xE3FE38, 0xE3FE3F, 0xE3FFC0, 0xE3FFC7, 0xE3FFF8, 0xE3FFFF,
    0xFC0000, 0xFC0007, 0xFC0038, 0xFC003F, 0xFC01C0, 0xFC01C7, 0xFC01F8, 0xFC01FF,
    0xFC0E00, 0xFC0E07, 0xFC0E38, 0xFC0E3F, 0xFC0FC0, 0xFC0FC7, 0xFC0FF8, 0xFC0FFF,
    0xFC7000, 0xFC7007, 0xFC7038, 0xFC703F, 0xFC71C0, 0xFC71C7, 0xFC71F8, 0xFC71FF,
    0xFC7E00, 0xFC7E07, 0xFC7E38, 0xFC7E3F, 0xFC7FC0, 0xFC7FC7, 0xFC7FF8, 0xFC7FFF,
    0xFF8000, 0xFF8007, 0xFF8038, 0xFF803F, 0xFF81C0, 0xFF81C7, 0xFF81F8, 0xFF81FF,
    0xFF8E00, 0xFF8E07, 0xFF8E38, 0xFF8E3F, 0xFF8FC0, 0xFF8FC7, 0xFF8FF8, 0xFF8FFF,
    0xFFF000, 0xFFF007, 0xFFF038, 0xFFF03F, 0xFFF1C0, 0xFFF1C7, 0xFFF1F8, 0xFFF1FF,
    0xFFFE00, 0xFFFE07, 0xFFFE38, 0xFFFE3F, 0xFFFFC0, 0xFFFFC7, 0xFFFFF8, 0xFFFFFF
};

/*
 * This is a table of 24-bit values, indexed with an 8-bit byte value,
 * that reverses the bit order of a byte and then expands each bit to three
 * consecutive bits. This is required for color expansion in 24bpp mode
 * with the coprocessor in 8bpp mode, with MSB-first bit order within a
 * byte.
 */

unsigned int byte_reversed_expand3[256] =
{
    0x000000, 0x0000E0, 0x00001C, 0x0000FC, 0x008003, 0x0080E3, 0x00801F, 0x0080FF,
    0x007000, 0x0070E0, 0x00701C, 0x0070FC, 0x00F003, 0x00F0E3, 0x00F01F, 0x00F0FF,
    0x000E00, 0x000EE0, 0x000E1C, 0x000EFC, 0x008E03, 0x008EE3, 0x008E1F, 0x008EFF,
    0x007E00, 0x007EE0, 0x007E1C, 0x007EFC, 0x00FE03, 0x00FEE3, 0x00FE1F, 0x00FEFF,
    0xC00100, 0xC001E0, 0xC0011C, 0xC001FC, 0xC08103, 0xC081E3, 0xC0811F, 0xC081FF,
    0xC07100, 0xC071E0, 0xC0711C, 0xC071FC, 0xC0F103, 0xC0F1E3, 0xC0F11F, 0xC0F1FF,
    0xC00F00, 0xC00FE0, 0xC00F1C, 0xC00FFC, 0xC08F03, 0xC08FE3, 0xC08F1F, 0xC08FFF,
    0xC07F00, 0xC07FE0, 0xC07F1C, 0xC07FFC, 0xC0FF03, 0xC0FFE3, 0xC0FF1F, 0xC0FFFF,
    0x380000, 0x3800E0, 0x38001C, 0x3800FC, 0x388003, 0x3880E3, 0x38801F, 0x3880FF,
    0x387000, 0x3870E0, 0x38701C, 0x3870FC, 0x38F003, 0x38F0E3, 0x38F01F, 0x38F0FF,
    0x380E00, 0x380EE0, 0x380E1C, 0x380EFC, 0x388E03, 0x388EE3, 0x388E1F, 0x388EFF,
    0x387E00, 0x387EE0, 0x387E1C, 0x387EFC, 0x38FE03, 0x38FEE3, 0x38FE1F, 0x38FEFF,
    0xF80100, 0xF801E0, 0xF8011C, 0xF801FC, 0xF88103, 0xF881E3, 0xF8811F, 0xF881FF,
    0xF87100, 0xF871E0, 0xF8711C, 0xF871FC, 0xF8F103, 0xF8F1E3, 0xF8F11F, 0xF8F1FF,
    0xF80F00, 0xF80FE0, 0xF80F1C, 0xF80FFC, 0xF88F03, 0xF88FE3, 0xF88F1F, 0xF88FFF,
    0xF87F00, 0xF87FE0, 0xF87F1C, 0xF87FFC, 0xF8FF03, 0xF8FFE3, 0xF8FF1F, 0xF8FFFF,
    0x070000, 0x0700E0, 0x07001C, 0x0700FC, 0x078003, 0x0780E3, 0x07801F, 0x0780FF,
    0x077000, 0x0770E0, 0x07701C, 0x0770FC, 0x07F003, 0x07F0E3, 0x07F01F, 0x07F0FF,
    0x070E00, 0x070EE0, 0x070E1C, 0x070EFC, 0x078E03, 0x078EE3, 0x078E1F, 0x078EFF,
    0x077E00, 0x077EE0, 0x077E1C, 0x077EFC, 0x07FE03, 0x07FEE3, 0x07FE1F, 0x07FEFF,
    0xC70100, 0xC701E0, 0xC7011C, 0xC701FC, 0xC78103, 0xC781E3, 0xC7811F, 0xC781FF,
    0xC77100, 0xC771E0, 0xC7711C, 0xC771FC, 0xC7F103, 0xC7F1E3, 0xC7F11F, 0xC7F1FF,
    0xC70F00, 0xC70FE0, 0xC70F1C, 0xC70FFC, 0xC78F03, 0xC78FE3, 0xC78F1F, 0xC78FFF,
    0xC77F00, 0xC77FE0, 0xC77F1C, 0xC77FFC, 0xC7FF03, 0xC7FFE3, 0xC7FF1F, 0xC7FFFF,
    0x3F0000, 0x3F00E0, 0x3F001C, 0x3F00FC, 0x3F8003, 0x3F80E3, 0x3F801F, 0x3F80FF,
    0x3F7000, 0x3F70E0, 0x3F701C, 0x3F70FC, 0x3FF003, 0x3FF0E3, 0x3FF01F, 0x3FF0FF,
    0x3F0E00, 0x3F0EE0, 0x3F0E1C, 0x3F0EFC, 0x3F8E03, 0x3F8EE3, 0x3F8E1F, 0x3F8EFF,
    0x3F7E00, 0x3F7EE0, 0x3F7E1C, 0x3F7EFC, 0x3FFE03, 0x3FFEE3, 0x3FFE1F, 0x3FFEFF,
    0xFF0100, 0xFF01E0, 0xFF011C, 0xFF01FC, 0xFF8103, 0xFF81E3, 0xFF811F, 0xFF81FF,
    0xFF7100, 0xFF71E0, 0xFF711C, 0xFF71FC, 0xFFF103, 0xFFF1E3, 0xFFF11F, 0xFFF1FF,
    0xFF0F00, 0xFF0FE0, 0xFF0F1C, 0xFF0FFC, 0xFF8F03, 0xFF8FE3, 0xFF8F1F, 0xFF8FFF,
    0xFF7F00, 0xFF7FE0, 0xFF7F1C, 0xFF7FFC, 0xFFFF03, 0xFFFFE3, 0xFFFF1F, 0xFFFFFF, 
};
