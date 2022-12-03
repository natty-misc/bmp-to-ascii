// Revision 2022-01-01

#define _CRT_SECURE_NO_WARNINGS 1

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>

char scale[] = "#Oo:*. ";

typedef struct
{
    char bfType[2];
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
} bmp_file_header_t;

typedef struct
{
    uint32_t biSize;
    uint32_t biWidth;
    uint32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    uint32_t biXPelsPerMeter;
    uint32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} bmp_image_header_t;

// BMP file header size
#define FILE_HEADER_SIZE 14

// BMP minimum image header size
// #define IMAGE_HEADER_SIZE 40

// Read a 2-byte integer from the given buffer at the given offset
// Note that this only works for little endian architectures, such as x86/AMD64!
#define GET_UINT16(BUF, X) ((uint16_t) (BUF)[(X)] | (uint16_t) (BUF)[(X) + 1] << 8)

// Read a 4-byte integer from the given buffer at the given offset
// Note that this only works for little endian architectures, such as x86/AMD64!
#define GET_UINT32(BUF, X) ((uint32_t) (BUF)[(X)] | (uint32_t) (BUF)[(X) + 1] << 8 | (uint32_t) (BUF)[(X) + 2] << 16 | (uint32_t) (BUF)[(X) + 3] << 24)

// The per-character aspect ratio to be applied, makes the image not stretched for non-square character fonts
#define ASPECT_RATIO_NOMINATOR 9
#define ASPECT_RATIO_DENOMINATOR 19

typedef struct
{
    size_t width;
    size_t height;
    unsigned char data[0];
} image;

unsigned char luminanceFromRGB(unsigned char r, unsigned char g, unsigned char b)
{
    return (unsigned char) (0.2126 * r + 0.7152 * g + 0.0722 * b);
}

image* loadImage(char* location)
{
    FILE* f = fopen(location, "rb");

    if (f == NULL)
    {
        puts("Opening failed...");
        return NULL;
    }

    unsigned char* result;
    // Move to the end of the file, so we can measure the file's size
    fseek(f, 0, SEEK_END);
    // ftell will return a negative value on error, here we assume it doesn't happen
    size_t size = ftell(f);

    if (size > SIZE_MAX)
    {
        puts("Size is too big!");
        fclose(f);
        return NULL;
    }

    // Return to position 0
    fseek(f, 0, SEEK_SET);
    result = (unsigned char*) malloc(size);

    if (result == NULL)
    {
        free(result);
        fputs("Memory allocation failed...", stderr);
        fclose(f);
        return NULL;
    }

    // Read the entire file into the buffer
    if (size != fread(result, sizeof(unsigned char), size, f))
    {
        free(result);
        fputs("Reading failed...", stderr);
        fclose(f);
        return NULL;
    }

    fclose(f);

    // The absolute minimum file size
    if (size < sizeof(bmp_file_header_t) + sizeof(bmp_image_header_t))
    {
        free(result);
        fputs("Invalid file...", stderr);
        return NULL;
    }

    const unsigned char* fileHeaderAddr = result;
    bmp_file_header_t fileHeaderData;
    memcpy(fileHeaderData.bfType, (char*) fileHeaderAddr, sizeof(fileHeaderData.bfType));
    fileHeaderData.bfSize = GET_UINT32(fileHeaderAddr, 2);
    fileHeaderData.bfReserved1 = GET_UINT16(fileHeaderAddr, 6);
    fileHeaderData.bfReserved2 = GET_UINT16(fileHeaderAddr, 8);
    fileHeaderData.bfOffBits = GET_UINT32(fileHeaderAddr, 10);
    const bmp_file_header_t* fileHeader = &fileHeaderData;

    fputs("File Header:", stderr);
    fprintf(stderr, "  fileHeader->bfSize      = %" SCNu32 "\n", fileHeader->bfSize);
    fprintf(stderr, "  fileHeader->bfReserved1 = %" SCNu16 "\n", fileHeader->bfReserved1);
    fprintf(stderr, "  fileHeader->bfReserved2 = %" SCNu16 "\n", fileHeader->bfReserved2);
    fprintf(stderr, "  fileHeader->bfOffBits   = %" SCNu32 "\n", fileHeader->bfOffBits);

    // Verify the BMP header to start with the "BM" characters
    const char magicNumber[] = "BM";
    if (strncmp(fileHeader->bfType, magicNumber, strlen(magicNumber)) != 0)
    {
        free(result);
        fputs("Incorrect file header...", stderr);
        return NULL;
    }

    size_t specifiedSize = fileHeader->bfSize;

    if (specifiedSize != size)
    {
        free(result);
        fputs("File sizes don't match...", stderr);
        return NULL;
    }

    const unsigned char* imageHeaderAddr = result + FILE_HEADER_SIZE;
    bmp_image_header_t imageHeaderData;
    imageHeaderData.biSize = GET_UINT32(imageHeaderAddr, 0);
    imageHeaderData.biWidth = GET_UINT32(imageHeaderAddr, 4);
    imageHeaderData.biHeight = GET_UINT32(imageHeaderAddr, 8);
    imageHeaderData.biPlanes = GET_UINT16(imageHeaderAddr, 12);
    imageHeaderData.biBitCount = GET_UINT16(imageHeaderAddr, 14);
    imageHeaderData.biCompression = GET_UINT32(imageHeaderAddr, 16);
    imageHeaderData.biSizeImage = GET_UINT32(imageHeaderAddr, 20);
    imageHeaderData.biXPelsPerMeter = GET_UINT32(imageHeaderAddr, 24);
    imageHeaderData.biYPelsPerMeter = GET_UINT32(imageHeaderAddr, 28);
    imageHeaderData.biClrUsed = GET_UINT32(imageHeaderAddr, 32);
    imageHeaderData.biClrImportant = GET_UINT32(imageHeaderAddr, 36);
    const bmp_image_header_t* imageHeader = &imageHeaderData;

    fputs("Image Header:", stderr);
    fprintf(stderr, "  imageHeader->biSize          = %" SCNu32 "\n", imageHeader->biSize);
    fprintf(stderr, "  imageHeader->biWidth         = %" SCNu32 "\n", imageHeader->biWidth);
    fprintf(stderr, "  imageHeader->biHeight        = %" SCNu32 "\n", imageHeader->biHeight);
    fprintf(stderr, "  imageHeader->biPlanes        = %" SCNu16 "\n", imageHeader->biPlanes);
    fprintf(stderr, "  imageHeader->biBitCount      = %" SCNu16 "\n", imageHeader->biBitCount);
    fprintf(stderr, "  imageHeader->biCompression   = %" SCNu32 "\n", imageHeader->biCompression);
    fprintf(stderr, "  imageHeader->biSizeImage     = %" SCNu32 "\n", imageHeader->biSizeImage);
    fprintf(stderr, "  imageHeader->biXPelsPerMeter = %" SCNu32 "\n", imageHeader->biXPelsPerMeter);
    fprintf(stderr, "  imageHeader->biYPelsPerMeter = %" SCNu32 "\n", imageHeader->biYPelsPerMeter);
    fprintf(stderr, "  imageHeader->biClrUsed       = %" SCNu32 "\n", imageHeader->biClrUsed);
    fprintf(stderr, "  imageHeader->biClrImportant  = %" SCNu32 "\n", imageHeader->biClrImportant);

    // Pixel data start
    const size_t pdOffset = fileHeader->bfOffBits;

    // Image width
    const uint32_t width = imageHeader->biWidth;
    const uint32_t height = imageHeader->biHeight;

    // Bits per pixel (bit depth)
    const uint16_t bpp = imageHeader->biBitCount;
    const bool noCompression = imageHeader->biCompression == 0;

    // Reject bit depths other than 24 bits (R8B8G8)
    if (bpp != 24 || !noCompression || width < 1 || height < 1 || width >= 16384 || height >= 16384)
    {
        free(result);
        fputs("Unsupported BMP format, only 24 bits per pixel images of resolution up to 16384x16384 are supported...", stderr);
        return NULL;
    }

    // Bytes per pixel, should be always 3
    const int bytesPerPixel = (int) (bpp / 8);

    // Size of one row (scanline) of pixels
    // This value is rounded UP to the nearest multiple of 4, since row starts are aligned to 4 bytes
    const size_t rowBytes = (width * bytesPerPixel + 3) / 4 * 4;

    fprintf(stderr, "Bytes per row: %zu\n", rowBytes);

    const size_t usedRowBytes = width * bytesPerPixel;

    // Padding so each row's size in bytes is divisible by 4
    const size_t rowPadding = rowBytes - usedRowBytes;

    fprintf(stderr, "Row padding: %zu\n", rowPadding);

    const size_t imageBytes = rowBytes * height;

    if (pdOffset + imageBytes > size)
    {
        free(result);
        fputs("Invalid offset specified...", stderr);
        return NULL;
    }

    // Total pixel count in image, also the amount of bytes needed to store 8-bit grayscale image data
    const size_t imgSize = width * height;

    // Allocate the main image struct
    image* outputImg = malloc(sizeof(image) + imgSize);
    if (outputImg == NULL)
    {
        free(result);
        fputs("Memory allocation failed...", stderr);
        return NULL;
    }

    outputImg->height = height;
    outputImg->width = width;

    // The address we write luminance image data to
    unsigned char* destPtr = outputImg->data;
    // The address we read color data from
    unsigned char* srcPtr = &result[pdOffset];

    for (size_t i = 0; i < imgSize; ++i)
    {
        // At this point srcPtr points to the red, green and blue values (in this order)
        // We simply need to read them
        unsigned char r = srcPtr[0]; // = *srcPtr
        unsigned char g = srcPtr[1]; // = *(srcPtr + 1)
        unsigned char b = srcPtr[2]; // = *(srcPtr + 2)

        // Write the luminance data and advance the pointer in the destination image
        *destPtr = luminanceFromRGB(r, g, b);
        destPtr++;

        // Offset the position by the color depth in bytes
        srcPtr += bytesPerPixel;

        // If we reach the start of a new line, apply extra padding to align the position to 4 bytes
        if (i % width == 0)
        {
            srcPtr += rowPadding;
        }
    }

    free(result);

    return outputImg;
}

void asciify(const image* img)
{
    size_t numScale = strlen(scale) - 1;

    for (size_t y = img->height * ASPECT_RATIO_NOMINATOR / ASPECT_RATIO_DENOMINATOR - 1; y > 0; --y)
    {
        for (size_t x = 0; x < img->width; ++x)
        {
            unsigned char c = img->data[x + img->width * (y * ASPECT_RATIO_DENOMINATOR / ASPECT_RATIO_NOMINATOR)];
            size_t rescaled = c * numScale / UCHAR_MAX;
            putchar(scale[numScale - rescaled]);
        }

        putchar('\n');
    }
}

void release(image* img)
{
    if (img)
        return;

    free(img);
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        fputs("Argument needed: filename.", stderr);
        return 1;
    }

    fputs(argv[1], stderr);

    fprintf(stderr, "ASCII Brightness Scale: %zu\n", strlen(scale) - 1);

    image* img = NULL;

    if ((img = loadImage(argv[1])) != NULL)
    {
        fprintf(stderr, "Image dimensions: %zux%zu\n", img->width, img->height);

        asciify(img);

        release(img);
    }

    return 0;
}
