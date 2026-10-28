// MSX PICOVERSE PROJECT
// (c) 2026 Cristiano Goncalves
// The Retro Hacker
//
// msxusbfdd.c - Windows console application to create the MSX PICOVERSE 2040 USB FDD UF2 file
//
// This program creates a UF2 file to program the Raspberry Pi Pico with the MSX PICOVERSE 2040 USB FDD firmware.
// The UF2 file is created with the combined PICO firmware binary and the 32KB MSX ROM (MSX-DOS 1 kernel + USB FDD
// BIOS), which the firmware finds right after its own binary in the flash memory.
//
// By default the ROM embedded in the tool is used. Another ROM file with the same layout can be used with -r,
// for BIOS development.
//
// This work is licensed  under a "Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License. 
// https://creativecommons.org/licenses/by-nc-sa/4.0/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "uf2format.h"
#include "msxusbfdd.h"
#include "msxusbfdd_rom.h"

#ifndef APP_VERSION
#define APP_VERSION "v1.01"
#endif

#define UF2FILENAME               "msxusbfdd.uf2"         // this is the UF2 file to program the Raspberry Pi Pico
#define UF2_FLAG_FAMILYID_PRESENT 0x00002000UL            // Signals that fileSize stores the RP2040 family ID
#define RP2040_FAMILY_ID          0xE48BFF56UL            // RP2040 family identifier expected by UF2 bootloader
#define FLASH_START               0x10000000              // Start of the flash memory on the Raspberry Pi Pico
#define ROM_SIZE                  32768                   // The BIOS ROM has two 16KB banks

#define FIRMWARE_DATA   ___pico_msxusbfdd_dist_msxusbfdd_bin
#define EMBEDDED_ROM    ___msx_dist_msxusbfdd_rom

static void print_usage(const char *prog_name)
{
    printf("Usage: %s [options]\n", prog_name);
    printf("Options:\n");
    printf("  -r <filename>, --rom <filename>     Use this 32KB BIOS ROM instead of the embedded one\n");
    printf("  -o <filename>, --output <filename>  Set UF2 output filename (default %s)\n", UF2FILENAME);
    printf("  -h, --help                          Show this help\n");
}

// Check that the ROM has the layout the firmware and the BIOS expect: 32KB and the "AB" ROM header
// at the beginning of both 16KB banks
static int check_rom(const uint8_t *rom, size_t size)
{
    if (size != ROM_SIZE)
    {
        printf("Invalid ROM size: %zu bytes (expected %d bytes).\n", size, ROM_SIZE);
        return 0;
    }

    if (rom[0] != 'A' || rom[1] != 'B' || rom[0x4000] != 'A' || rom[0x4001] != 'B')
    {
        printf("Invalid ROM: the \"AB\" header is missing in one of the 16KB banks.\n");
        return 0;
    }

    return 1;
}

static uint8_t *load_rom_file(const char *filename, size_t *size)
{
    FILE *f = fopen(filename, "rb");
    if (!f)
    {
        perror("Failed to open ROM file");
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (len <= 0)
    {
        printf("Invalid ROM file.\n");
        fclose(f);
        return NULL;
    }

    uint8_t *data = malloc((size_t)len);
    if (!data || fread(data, 1, (size_t)len, f) != (size_t)len)
    {
        printf("Failed to read the ROM file.\n");
        free(data);
        fclose(f);
        return NULL;
    }

    fclose(f);
    *size = (size_t)len;
    return data;
}

// create_uf2_file - Create the UF2 file
// This function streams the firmware binary and the ROM payload into UF2 blocks.
static int create_uf2_file(const uint8_t *rom, const char *uf2_filename)
{
    const uint8_t *firmware_data = FIRMWARE_DATA;
    const size_t firmware_size = sizeof(FIRMWARE_DATA);
    const size_t total_size = firmware_size + ROM_SIZE;

    FILE *uf2_file = fopen(uf2_filename, "wb");
    if (!uf2_file)
    {
        perror("Failed to create UF2 file");
        return 0;
    }

    UF2_Block bl;
    memset(&bl, 0, sizeof(bl));
    bl.magicStart0 = UF2_MAGIC_START0;
    bl.magicStart1 = UF2_MAGIC_START1;
    bl.flags = UF2_FLAG_FAMILYID_PRESENT;
    bl.magicEnd = UF2_MAGIC_END;
    bl.targetAddr = FLASH_START;
    bl.payloadSize = 256;
    bl.numBlocks = (uint32_t)((total_size + bl.payloadSize - 1) / bl.payloadSize);
    bl.fileSize = RP2040_FAMILY_ID;

    size_t offset = 0;
    uint32_t block_no = 0;
    while (offset < total_size)
    {
        memset(bl.data, 0, sizeof(bl.data));

        size_t chunk = total_size - offset;
        if (chunk > bl.payloadSize)
        {
            chunk = bl.payloadSize;
        }

        // The ROM starts right after the firmware, a block can have bytes of both
        for (size_t i = 0; i < chunk; i++)
        {
            size_t pos = offset + i;
            bl.data[i] = (pos < firmware_size) ? firmware_data[pos] : rom[pos - firmware_size];
        }

        bl.blockNo = block_no++;
        if (fwrite(&bl, 1, sizeof(bl), uf2_file) != sizeof(bl))
        {
            printf("Failed to write UF2 block %u.\n", block_no - 1);
            fclose(uf2_file);
            return 0;
        }
        bl.targetAddr += bl.payloadSize;
        offset += chunk;
    }

    fclose(uf2_file);
    printf("\nSuccessfully wrote %u blocks to %s.\n", block_no, uf2_filename);
    return 1;
}

int main(int argc, char *argv[])
{
    printf("MSX PICOVERSE 2040 USB FDD UF2 Creator %s\n", APP_VERSION);
    printf("(c) 2026 The Retro Hacker\n\n");

    const char *output_filename = UF2FILENAME;
    const char *rom_filename = NULL;

    for (int i = 1; i < argc; i++)
    {
        if ((strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) && i + 1 < argc)
        {
            output_filename = argv[++i];
        }
        else if ((strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--rom") == 0) && i + 1 < argc)
        {
            rom_filename = argv[++i];
        }
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
        {
            print_usage(argv[0]);
            return 0;
        }
        else
        {
            printf("Invalid option: %s\n\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    const uint8_t *rom = EMBEDDED_ROM;
    size_t rom_size = sizeof(EMBEDDED_ROM);
    uint8_t *rom_from_file = NULL;

    if (rom_filename)
    {
        rom_from_file = load_rom_file(rom_filename, &rom_size);
        if (!rom_from_file)
        {
            return 1;
        }
        rom = rom_from_file;
        printf("ROM: %s\n", rom_filename);
    }
    else
    {
        printf("ROM: embedded USB FDD BIOS\n");
    }

    if (!check_rom(rom, rom_size))
    {
        free(rom_from_file);
        return 1;
    }

    printf("Firmware Size: %zu bytes\n", sizeof(FIRMWARE_DATA));
    printf("ROM Size: %zu bytes\n", rom_size);
    printf("UF2 Output: %s\n", output_filename);

    int ok = create_uf2_file(rom, output_filename);
    free(rom_from_file);
    return ok ? 0 : 1;
}
