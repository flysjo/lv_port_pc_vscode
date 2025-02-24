#!/usr/bin/bash

# Generate font files
# Usage: ./fontGen.sh <ttf file> <font name> <font size>

# Check if the number of arguments is correct
if [ $# -ne 3 ]; then
    echo "Usage: ./fontGen.sh <ttf file> <font name> <font size>"
    exit 1
fi

# Check if the ttf file exists
if [ ! -f $1 ]; then
    echo "File $1 not found!"
    exit 1
fi

# Check if the font size is a number
if ! [[ $3 =~ ^[0-9]+$ ]]; then
    echo "Font size must be a number!"
    exit 1
fi

# Generate the font files
echo "Generating font files..."

npx lv_font_conv --size $3 --bpp 2 --no-compress --format bin --font $1 --output $2.bin --force-fast-kern-format --range 0x20-0x7F
npx lv_font_conv --size $3 --bpp 2 --no-compress --format bin --font $1 --output $2_2.bin --force-fast-kern-format --range 0x20-0x1F470
