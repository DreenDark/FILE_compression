# FILE_compression
simple program to compress and decompress the given file 
# Huffman File Compressor/Decompressor

This is a simple **Huffman coding-based file compression and decompression program** written in C.  
It works on **any type of file**, including text, images, audio, and binary files.

---

## Features

- Compress any file using Huffman coding.
- Decompress Huffman-compressed files.
- Binary-safe: works with all 256 byte values.
- Stores frequency table in the compressed file to reconstruct the Huffman tree.
- Dynamically generates Huffman codes for symbols in the file.

---

## Files

- `main.c` → The main C source code containing both compression and decompression logic.  
- `README.md` → This file.  
- `explanation.md` → explain line by line.  

---

## How to Compile

git clone https://github.com/DreenDark/FILE_compression.git
cd FILE_compression
Use GCC to compile:


MENU: 

Huffman File Compressor/Decompressor
1. Compress a file
2. Decompress a file
Enter your choice (1 or 2):

what you see:

Enter your choice (1 or 2): 1
Enter the filename: x.txt
Compression done! Output file: x.txt.huff

OR

Enter your choice (1 or 2): 2
Enter the filename: x.txt.huff
Decompression done! Output file: x.txt.huff_dec.txt

How it Works
Compression

Counts frequency of each byte (0–255) in the input file.

Builds a Huffman tree based on frequencies.

Generates Huffman codes for each symbol.

Writes the frequency table (256 × 4 bytes) at the start of the output file.

Encodes the file using variable-length Huffman codes at the bit level.

Decompression

Reads the frequency table from the start of the compressed file.

Reconstructs the Huffman tree.

Reads the compressed bits and traverses the Huffman tree to decode symbols.

Writes original symbols to the output file.
